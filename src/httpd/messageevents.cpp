/* 
 * Message events triggering infrastructure.
 * Copyright (C) 2010 Petr Kubanek <petr@kubanek.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "httpd.h"

#include "messageevents.h"
#include "connection/fork.h"

using namespace rts2xmlrpc;

MessageEvent::MessageEvent (HttpD *_master, std::string _deviceName, int _type)
{
	master = _master;
	deviceName = ci_string (_deviceName.c_str ());

	type = _type;
}

void MessageCommand::run (rts2core::Message *message)
{
	int ret;
	rts2core::ConnFork *cf = new rts2core::ConnFork (master, commandName.c_str (), true, false, 100);
	cf->addArg (message->getType ());
	ret = cf->init ();
	if (ret)
	{
		delete cf;
		return;
	}

	master->addConnection (cf);
}

void MessageEmail::run (rts2core::Message *message)
{
	EmailAction::run (master);
}
