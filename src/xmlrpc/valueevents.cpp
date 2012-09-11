/* 
 * Value changes triggering infrastructure. 
 * Copyright (C) 2009 Petr Kubanek <petr@kubanek.net>
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

#include "xmlrpcd.h"

#include "rts2script/connexe.h"
#include "timestamp.h"

using namespace rts2xmlrpc;

ValueChange::ValueChange (XmlRpcd *_master, std::string _deviceName, std::string _valueName, float _cadency, Expression *_test):Object ()
{
	master = _master;
	deviceName = ci_string (_deviceName.c_str ());
	valueName = ci_string (_valueName.c_str ());
			
	lastTime = 0;
	cadency = _cadency;
	test = _test;

	if (cadency > 0)
		master->addTimer (cadency, new Event (EVENT_XMLRPC_VALUE_TIMER, this));
}

void ValueChange::postEvent (Event *event)
{
	switch (event->getType ())
	{
		case EVENT_XMLRPC_VALUE_TIMER:
			if (lastTime + cadency <= time(NULL))
			{
				rts2core::Connection *conn = master->getOpenConnection (deviceName.c_str ());
				if (conn)
					conn->queCommand (new rts2core::CommandInfo (master));
			}
			if (cadency > 0)
				master->addTimer (cadency, new Event (EVENT_XMLRPC_VALUE_TIMER, this));
			break;
	}
	Object::postEvent (event);
}

#ifndef RTS2_HAVE_PGSQL

void ValueChangeRecord::run (rts2core::Value *val, double validTime)
{
	std::cout << Timestamp (validTime) << " value: " << deviceName.c_str () << " " << valueName.c_str () << val->getDisplayValue () << std::endl;
}

#endif /* ! RTS2_HAVE_PGSQL */

void ValueChangeCommand::run (rts2core::Value *val, double validTime)
{
	int ret;
	rts2script::ConnExe *cf = new rts2script::ConnExe (master, commandName.c_str (), true, 100);
	cf->addArg (val->getName ());
	cf->addArg (validTime);
	ret = cf->init ();
	if (ret)
	{
		delete cf;
		return;
	}

	master->addConnection (cf);
}

void ValueChangeEmail::run (rts2core::Value *val, double validTime)
{
	EmailAction::run (master);
}
