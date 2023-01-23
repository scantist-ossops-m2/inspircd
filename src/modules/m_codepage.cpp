/*
 * InspIRCd -- Internet Relay Chat Daemon
 *
 *   Copyright (C) 2020-2021 Sadie Powell <sadie@witchery.services>
 *
 * This file is part of InspIRCd.  InspIRCd is free software: you can
 * redistribute it and/or modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation, version 2.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include "inspircd.h"
#include "modules/isupport.h"

class Codepage
{
public:
	enum class AllowCharacterResult
		: uint8_t
	{
		// The character is allowed in a nick.
		OKAY,

		// The character is never valid in a nick.
		NOT_VALID,

		// The character is not valid at the front of a nick.
		NOT_VALID_AT_FRONT,
	};

	// The mapping of lower case characters to upper case characters.
	unsigned char casemap[UCHAR_MAX + 1];

	// Initialises the Codepage class.
	Codepage()
	{
		for (size_t i = 0; i <= UCHAR_MAX; ++i)
			casemap[i] = i;
	}

	// Destroys the Codepage class.
	virtual ~Codepage() = default;

	// Specifies that a character is allowed.
	virtual AllowCharacterResult AllowCharacter(uint32_t character, bool front)
	{
		if (front)
		{
			// Nicknames can not begin with a number as that would collide with
			// a user identifier.
			if (character >= '0' && character <= '9')
				return AllowCharacterResult::NOT_VALID_AT_FRONT;

			// Nicknames can not begin with a : as it has special meaning within
			// the IRC message format.
			if (character == ':')
				return AllowCharacterResult::NOT_VALID_AT_FRONT;
		}

		// Nicknames can never contain NUL, CR, LF, or SPACE as they are either
		// banned within an IRC message or have special meaning within the IRC
		// message format.
		if (!character || character == '\n' || character == '\r' || character == ' ')
			return AllowCharacterResult::NOT_VALID;

		// The character is probably okay?
		return AllowCharacterResult::OKAY;
	}

	// Determines whether a nickname is valid.
	virtual bool IsValidNick(const std::string_view& nick) = 0;

	// Retrieves the link data for this codepage.
	virtual void GetLinkData(Module::LinkData& data, std::string& compatdata) const = 0;

	// Maps an upper case character to a lower case character.
	virtual bool Map(unsigned long upper, unsigned long lower) = 0;
};

class SingleByteCodepage final
	: public Codepage
{
private:
	// The characters which are allowed in nicknames.
	CharState allowedchars;

	// The characters which are allowed at the front of a nickname.
	CharState allowedfrontchars;

public:
	AllowCharacterResult AllowCharacter(uint32_t character, bool front) override
	{
		// Single byte codepage can, as their name suggests, only be one byte in size.
		if (character > UCHAR_MAX)
			return AllowCharacterResult::NOT_VALID;

		// Check the common allowed character rules.
		AllowCharacterResult result = Codepage::AllowCharacter(character, front);
		if (result != AllowCharacterResult::OKAY)
			return result;

		// The character is okay.
		allowedchars.set(character);
		allowedfrontchars.set(character, front);
		return AllowCharacterResult::OKAY;
	}

	bool IsValidNick(const std::string_view& nick) override
	{
		if (nick.empty() || nick.length() > ServerInstance->Config->Limits.MaxNick)
			return false;

		for (std::string_view::const_iterator iter = nick.begin(); iter != nick.end(); ++iter)
		{
			unsigned char chr = static_cast<unsigned char>(*iter);

			// Check that the character is allowed at the front of the nick.
			if (iter == nick.begin() && !allowedfrontchars[chr])
				return false;

			// Check that the character is allowed in the nick.
			if (!allowedchars[chr])
				return false;
		}

		return true;
	}

	void GetLinkData(Module::LinkData& data, std::string& compatdata) const override
	{
		for (size_t i = 0; i < allowedfrontchars.size(); ++i)
			if (allowedfrontchars[i])
				data["front"].push_back(static_cast<unsigned char>(i));

		for (size_t i = 0; i < allowedchars.size(); ++i)
			if (allowedchars[i])
				data["middle"].push_back(static_cast<unsigned char>(i));

		for (size_t i = 0; i < sizeof(casemap); ++i)
		{
			if (casemap[i] == i)
				continue;

			data["map"].push_back(static_cast<unsigned char>(i));
			data["map"].push_back(casemap[i]);
			data["map"].push_back(',');
		}

		compatdata = INSP_FORMAT("front={}&middle={}&map={}", data["front"], data["middle"], data["map"]);
	}

	bool Map(unsigned long upper, unsigned long lower) override
	{
		if (upper > UCHAR_MAX || lower > UCHAR_MAX)
			return false;

		casemap[upper] = lower;
		return true;
	}
};

class ModuleCodepage final
	: public Module
	, public ISupport::EventListener
{
private:
	// The currently active codepage.
	std::unique_ptr<Codepage> codepage = nullptr;

	// The character map which was set before this module was loaded.
	const unsigned char* origcasemap;

	// The name of the character map which was set before this module was loaded.
	const std::string origcasemapname;

	// The IsNick handler which was set before this module was loaded.
	const std::function<bool(const std::string_view&)> origisnick;

	// The character set used for the codepage.
	std::string charset;

	template <typename T>
	void RehashHashmap(T& hashmap)
	{
		T newhash(hashmap.bucket_count());
		for (const auto& [key, value] : hashmap)
			newhash.emplace(key, value);
		hashmap.swap(newhash);
	}

	static void CheckDuplicateNick()
	{
		insp::flat_set<std::string, irc::insensitive_swo> duplicates;
		for (auto* user : ServerInstance->Users.GetLocalUsers())
		{
			if (user->nick != user->uuid && !duplicates.insert(user->nick).second)
			{
				user->WriteNumeric(RPL_SAVENICK, user->uuid, "Your nickname is no longer available.");
				user->ChangeNick(user->uuid);
			}
		}
	}

	static void CheckInvalidNick()
	{
		for (auto* user : ServerInstance->Users.GetLocalUsers())
		{
			if (user->nick != user->uuid && !ServerInstance->IsNick(user->nick))
			{
				user->WriteNumeric(RPL_SAVENICK, user->uuid, "Your nickname is no longer valid.");
				user->ChangeNick(user->uuid);
			}
		}
	}

	void CheckRehash(unsigned char* prevmap)
	{
		if (!memcmp(prevmap, national_case_insensitive_map, UCHAR_MAX))
			return;

		RehashHashmap(ServerInstance->Users.clientlist);
		RehashHashmap(ServerInstance->Users.uuidlist);
		RehashHashmap(ServerInstance->Channels.GetChans());
	}

public:
	ModuleCodepage()
		: Module(VF_VENDOR | VF_COMMON, "Allows the server administrator to define what characters are allowed in nicknames and how characters should be compared in a case insensitive way.")
		, ISupport::EventListener(this)
		, origcasemap(national_case_insensitive_map)
		, origcasemapname(ServerInstance->Config->CaseMapping)
		, origisnick(ServerInstance->IsNick)
	{
	}

	~ModuleCodepage() override
	{
		ServerInstance->IsNick = origisnick;
		CheckInvalidNick();

		ServerInstance->Config->CaseMapping = origcasemapname;
		national_case_insensitive_map = origcasemap;
		CheckDuplicateNick();
		if (codepage) // nullptr if ReadConfig throws on load.
			CheckRehash(codepage->casemap);
	}

	void ReadConfig(ConfigStatus& status) override
	{
		const auto& codepagetag = ServerInstance->Config->ConfValue("codepage");

		const std::string name = codepagetag->getString("name");
		if (name.empty())
			throw ModuleException(this, "<codepage:name> is a required field!");

		std::unique_ptr<Codepage> newcodepage = std::make_unique<SingleByteCodepage>();
		for (const auto& [_, tag] : ServerInstance->Config->ConfTags("cpchars"))
		{
			unsigned long begin = tag->getUInt("begin", tag->getUInt("index", 0));
			if (!begin)
				throw ModuleException(this, "<cpchars> tag without index or begin specified at " + tag->source.str());

			unsigned long end = tag->getUInt("end", begin);
			if (begin > end)
				throw ModuleException(this, "<cpchars:begin> must be lower than <cpchars:end> at " + tag->source.str());

			bool front = tag->getBool("front", false);
			for (unsigned long pos = begin; pos <= end; ++pos)
			{
				// This cast could be unsafe but it's not obvious how to make it safe.
				switch (newcodepage->AllowCharacter(static_cast<uint32_t>(pos), front))
				{
					case Codepage::AllowCharacterResult::OKAY:
						ServerInstance->Logs.Debug(MODNAME, "Marked %lu (%.4s) as allowed (front: %s)",
							pos, fmt::ptr(&pos), front ? "yes" : "no");
						break;

					case Codepage::AllowCharacterResult::NOT_VALID:
						throw ModuleException(this, INSP_FORMAT("<cpchars> tag contains a forbidden character: {} at {}",
							pos, tag->source.str()));

					case Codepage::AllowCharacterResult::NOT_VALID_AT_FRONT:
						throw ModuleException(this, INSP_FORMAT("<cpchars> tag contains a forbidden front character: {} at {}",
							pos, tag->source.str()));
				}
			}
		}

		for (const auto& [_, tag] : ServerInstance->Config->ConfTags("cpcase"))
		{
			unsigned long lower = tag->getUInt("lower", 0);
			if (!lower)
				throw ModuleException(this, "<cpcase:lower> is required at " + tag->source.str());

			unsigned long upper = tag->getUInt("upper", 0);
			if (!upper)
				throw ModuleException(this, "<cpcase:upper> is required at " + tag->source.str());

			if (!newcodepage->Map(upper, lower))
				throw ModuleException(this, "Malformed <cpcase> tag at " + tag->source.str());

			ServerInstance->Logs.Debug(MODNAME, "Marked %lu (%.4s) as the lower case version of %lu (%.4s)",
				lower, fmt::ptr(&lower), upper, fmt::ptr(&upper));
		}

		charset = codepagetag->getString("charset");
		std::swap(codepage, newcodepage);
		ServerInstance->IsNick = [this](const std::string_view& nick) { return codepage->IsValidNick(nick); };
		CheckInvalidNick();

		ServerInstance->Config->CaseMapping = name;
		national_case_insensitive_map = codepage->casemap;
		if (newcodepage) // nullptr on first read.
			CheckRehash(newcodepage->casemap);
	}

	void OnBuildISupport(ISupport::TokenMap& tokens) override
	{
		if (!charset.empty())
			tokens["CHARSET"] = charset;
	}

	void GetLinkData(LinkData& data, std::string& compatdata) override
	{
		codepage->GetLinkData(data, compatdata);
	}
};

MODULE_INIT(ModuleCodepage)
