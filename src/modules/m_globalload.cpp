/*
 * InspIRCd -- Internet Relay Chat Daemon
 *
 *   Copyright (C) 2013, 2017-2018, 2021 Sadie Powell <sadie@witchery.services>
 *   Copyright (C) 2012, 2019 Robby <robby@chatbelgie.be>
 *   Copyright (C) 2012, 2014-2016 Attila Molnar <attilamolnar@hush.com>
 *   Copyright (C) 2009-2010 Daniel De Graaf <danieldg@inspircd.org>
 *   Copyright (C) 2009 Robin Burchell <robin+git@viroteck.net>
 *   Copyright (C) 2007-2008 Dennis Friis <peavey@inspircd.org>
 *   Copyright (C) 2007 John Brooks <special@inspircd.org>
 *   Copyright (C) 2006-2008, 2010 Craig Edwards <brain@inspircd.org>
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

class CommandGLoadModule final
	: public Command
{
public:
	CommandGLoadModule(Module* Creator)
		: Command(Creator,"GLOADMODULE", 1)
	{
		access_needed = CmdAccess::OPERATOR;
		allow_empty_last_param = false;
		syntax = { "<modulename> [<servermask>]" };
	}

	CmdResult Handle(User* user, const Params& parameters) override
	{
		std::string servername = parameters.size() > 1 ? parameters[1] : "*";

		if (InspIRCd::Match(ServerInstance->Config->ServerName.c_str(), servername))
		{
			if (ServerInstance->Modules.Load(parameters[0].c_str()))
			{
				ServerInstance->SNO.WriteToSnoMask('a', "NEW MODULE '%s' GLOBALLY LOADED BY '%s'",parameters[0].c_str(), user->nick.c_str());
				user->WriteRemoteNumeric(RPL_LOADEDMODULE, parameters[0], "Module successfully loaded.");
			}
			else
			{
				user->WriteRemoteNumeric(ERR_CANTLOADMODULE, parameters[0], ServerInstance->Modules.LastError());
			}
		}
		else
			ServerInstance->SNO.WriteToSnoMask('a', "MODULE '%s' GLOBAL LOAD BY '%s' (not loaded here)",parameters[0].c_str(), user->nick.c_str());

		return CmdResult::SUCCESS;
	}

	RouteDescriptor GetRouting(User* user, const Params& parameters) override
	{
		return ROUTE_BROADCAST;
	}
};

class CommandGUnloadModule final
	: public Command
{
public:
	CommandGUnloadModule(Module* Creator)
		: Command(Creator,"GUNLOADMODULE", 1)
	{
		access_needed = CmdAccess::OPERATOR;
		allow_empty_last_param = false;
		syntax = { "<modulename> [<servermask>]" };
	}

	CmdResult Handle(User* user, const Params& parameters) override
	{
		if (InspIRCd::Match(parameters[0], "core_*", ascii_case_insensitive_map))
		{
			user->WriteRemoteNumeric(ERR_CANTUNLOADMODULE, parameters[0], "You cannot unload core commands!");
			return CmdResult::FAILURE;
		}

		std::string servername = parameters.size() > 1 ? parameters[1] : "*";

		if (InspIRCd::Match(ServerInstance->Config->ServerName.c_str(), servername))
		{
			Module* m = ServerInstance->Modules.Find(parameters[0]);
			if (m)
			{
				if (ServerInstance->Modules.Unload(m))
				{
					ServerInstance->SNO.WriteToSnoMask('a', "MODULE '%s' GLOBALLY UNLOADED BY '%s'",parameters[0].c_str(), user->nick.c_str());
					user->WriteRemoteNumeric(RPL_UNLOADEDMODULE, parameters[0], "Module successfully unloaded.");
				}
				else
				{
					user->WriteRemoteNumeric(ERR_CANTUNLOADMODULE, parameters[0], ServerInstance->Modules.LastError());
				}
			}
			else
				user->WriteRemoteNumeric(ERR_CANTUNLOADMODULE, parameters[0], "No such module");
		}
		else
			ServerInstance->SNO.WriteToSnoMask('a', "MODULE '%s' GLOBAL UNLOAD BY '%s' (not unloaded here)",parameters[0].c_str(), user->nick.c_str());

		return CmdResult::SUCCESS;
	}

	RouteDescriptor GetRouting(User* user, const Params& parameters) override
	{
		return ROUTE_BROADCAST;
	}
};

class CommandGReloadModule final
	: public Command
{
public:
	CommandGReloadModule(Module* Creator)
		: Command(Creator, "GRELOADMODULE", 1)
	{
		access_needed = CmdAccess::OPERATOR;
		allow_empty_last_param = false;
		syntax = { "<modulename> [<servermask>]" };
	}

	CmdResult Handle(User* user, const Params& parameters) override
	{
		std::string servername = parameters.size() > 1 ? parameters[1] : "*";

		if (InspIRCd::Match(ServerInstance->Config->ServerName.c_str(), servername))
		{
			Module* m = ServerInstance->Modules.Find(parameters[0]);
			if (m)
			{
				ServerInstance->SNO.WriteToSnoMask('a', "MODULE '%s' GLOBALLY RELOADED BY '%s'", parameters[0].c_str(), user->nick.c_str());
				ServerInstance->Parser.CallHandler("RELOADMODULE", parameters, user);
			}
			else
			{
				user->WriteRemoteNumeric(ERR_CANTUNLOADMODULE, parameters[0], "Could not find a loaded module by that name");
				return CmdResult::FAILURE;
			}
		}
		else
			ServerInstance->SNO.WriteToSnoMask('a', "MODULE '%s' GLOBAL RELOAD BY '%s' (not reloaded here)",parameters[0].c_str(), user->nick.c_str());

		return CmdResult::SUCCESS;
	}

	RouteDescriptor GetRouting(User* user, const Params& parameters) override
	{
		return ROUTE_BROADCAST;
	}
};

class ModuleGlobalLoad final
	: public Module
{
private:
	CommandGLoadModule cmdgloadmodule;
	CommandGUnloadModule cmdgunloadmodule;
	CommandGReloadModule cmdgreloadmodule;

public:
	ModuleGlobalLoad()
		: Module(VF_VENDOR | VF_COMMON, "Adds the /GLOADMODULE, /GRELOADMODULE, and /GUNLOADMODULE commands which allows server operators to load, reload, and unload modules on remote servers.")
		, cmdgloadmodule(this)
		, cmdgunloadmodule(this)
		, cmdgreloadmodule(this)
	{
	}
};

MODULE_INIT(ModuleGlobalLoad)
