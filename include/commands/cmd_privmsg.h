/*       +------------------------------------+
 *       | Inspire Internet Relay Chat Daemon |
 *       +------------------------------------+
 *
 *  InspIRCd is copyright (C) 2002-2007 ChatSpike-Dev.
 *		       E-mail:
 *		<brain@chatspike.net>
 *		<Craig@chatspike.net>
 *
 * Written by Craig Edwards, Craig McLure, and others.
 * This program is free but copyrighted software; see
 *	    the file COPYING for details.
 *
 * ---------------------------------------------------
 */

#ifndef __CMD_PRIVMSG_H__
#define __CMD_PRIVMSG_H__

// include the common header files

#include "users.h"
#include "channels.h"

/** Handle /PRIVMSG
 */
class cmd_privmsg : public command_t
{
 public:
	cmd_privmsg (InspIRCd* Instance) : command_t(Instance,"PRIVMSG",0,2) { syntax = "<target>{,<target>} <message>"; }
	CmdResult Handle(const char** parameters, int pcnt, userrec *user);
};

#endif