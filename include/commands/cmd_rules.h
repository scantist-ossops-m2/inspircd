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

#ifndef __CMD_RULES_H__
#define __CMD_RULES_H__

// include the common header files

#include <string>
#include <vector>
#include "inspircd.h"
#include "users.h"
#include "channels.h"

/** Handle /RULES
 */
class cmd_rules : public command_t
{
 public:
	cmd_rules (InspIRCd* Instance) : command_t(Instance,"RULES",0,0) { syntax = "[<servername>]"; }
	CmdResult Handle(const char** parameters, int pcnt, userrec *user);
};

#endif