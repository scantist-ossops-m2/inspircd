/*       +------------------------------------+
 *       | Inspire Internet Relay Chat Daemon |
 *       +------------------------------------+
 *
 *  InspIRCd is copyright (C) 2002-2006 ChatSpike-Dev.
 *                       E-mail:
 *                <brain@chatspike.net>
 *                <Craig@chatspike.net>
 *
 * Written by Craig Edwards, Craig McLure, and others.
 * This program is free but copyrighted software; see
 *            the file COPYING for details.
 *
 * ---------------------------------------------------
 */

#include "inspircd.h"
#include "users.h"
#include "commands/cmd_part.h"



extern "C" command_t* init_command(InspIRCd* Instance)
{
	return new cmd_part(Instance);
}

CmdResult cmd_part::Handle (const char** parameters, int pcnt, userrec *user)
{
	if (ServerInstance->Parser->LoopCall(user, this, parameters, pcnt, 0))
		return CMD_SUCCESS;

	chanrec* c = ServerInstance->FindChan(parameters[0]);
	
	if (c)
	{
		if (!c->PartUser(user, pcnt > 1 ? parameters[1] : NULL))
			/* Arse, who stole our channel! :/ */
			delete c;
	}
	else
	{
		user->WriteServ( "401 %s %s :No such channel", user->nick, parameters[0]);
		return CMD_FAILURE;
	}

	return CMD_SUCCESS;
}
