#include <stdio.h>
#include <string>
#include "users.h"
#include "channels.h"
#include "modules.h"

/* $ModDesc: Provides support for /KNOCK and mode +K */

Server *Srv;
	 
void handle_knock(char **parameters, int pcnt, userrec *user)
{
	chanrec* c = Srv->FindChannel(parameters[0]);
	std::string line = "";

	for (int i = 1; i < pcnt - 1; i++)
	{
		line = line + std::string(parameters[i]) + " ";
	}
	line = line + std::string(parameters[pcnt-1]);

	if (c->IsCustomModeSet('K'))
	{
		WriteServ(user->fd,"480 %s :Can't KNOCK on %s, +K is set.",user->nick, c->name);
		return;
	}
	if (c->inviteonly)
	{
		WriteChannelWithServ((char*)Srv->GetServerName().c_str(),c,user,"NOTICE %s :User %s is KNOCKing on %s (%s)",c->name,user->nick,c->name,line.c_str());
		WriteServ(user->fd,"NOTICE %s :KNOCKing on %s",user->nick,c->name);
		return;
	}
	else
	{
		WriteServ(user->fd,"480 %s :Can't KNOCK on %s, channel is not invite only so knocking is pointless!",user->nick, c->name);
		return;
	}
}


class ModuleKnock : public Module
{
 public:
	ModuleKnock()
	{
		Srv = new Server;
		
		Srv->AddExtendedMode('K',MT_CHANNEL,false,0,0);
		Srv->AddCommand("KNOCK",handle_knock,0,2);
	}
	
	virtual ~ModuleKnock()
	{
		delete Srv;
	}
	
	virtual Version GetVersion()
	{
		return Version(1,0,0,1);
	}
	
	virtual int OnExtendedMode(userrec* user, void* target, char modechar, int type, bool mode_on, string_list &params)
	{
		// check if this is our mode character...
		if ((modechar == 'K') && (type == MT_CHANNEL))
  		{
			return 1;
		}
		else
		{
			return 0;
		}
	}

};

// stuff down here is the module-factory stuff. For basic modules you can ignore this.

class ModuleKnockFactory : public ModuleFactory
{
 public:
	ModuleKnockFactory()
	{
	}
	
	~ModuleKnockFactory()
	{
	}
	
	virtual Module * CreateModule()
	{
		return new ModuleKnock;
	}
	
};


extern "C" void * init_module( void )
{
	return new ModuleKnockFactory;
}

