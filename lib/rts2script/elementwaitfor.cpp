/*
 * Wait for some event or for a number of seconds.
 * Copyright (C) 2007-2009 Petr Kubanek <petr@kubanek.net>
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

#include "elementwaitfor.h"

using namespace rts2script;

ElementWaitFor::ElementWaitFor (Script * _script, const char *new_device, char *_valueName, double _tarval, double _range):Element (_script)
{
	valueName = std::string (_valueName);
	deviceName = new char[strlen (new_device) + 1];
	strcpy (deviceName, new_device);
	// if value contain device..
	tarval = _tarval;
	range = _range;
	setIdleTimeout (1);
}

int ElementWaitFor::defnextCommand (rts2core::DevClient * client, rts2core::Command ** new_command, char new_device[DEVICE_NAME_SIZE])
{
	return idle ();
}

int ElementWaitFor::idle ()
{
	rts2core::Value *val = script->getMaster ()->getValue (deviceName, valueName.c_str ());
	if (!val)
	{
		NetworkAddress *add = script->getMaster ()->findAddress (deviceName);
		// we will get device..
		if (add != NULL)
			return NEXT_COMMAND_KEEP;
		logStream (MESSAGE_ERROR) << "Cannot get value " << deviceName << "." <<
			valueName << sendLog;
		return NEXT_COMMAND_NEXT;
	}
	if (fabs (val->getValueDouble () - tarval) <= range)
	{
		return NEXT_COMMAND_NEXT;
	}
	return Element::idle ();
}

int ElementSleep::defnextCommand (rts2core::DevClient * client, rts2core::Command ** new_command, char new_device[DEVICE_NAME_SIZE])
{
	if (!isnan (sec))
	{
		// this caused idle to be called after sec..
		// Element keep care that it will not be called before sec expires
		setIdleTimeout (sec);
		return NEXT_COMMAND_KEEP;
	}
	return NEXT_COMMAND_NEXT;
}

int ElementSleep::idle ()
{
	sec = NAN;
	return NEXT_COMMAND_NEXT;
}

void ElementSleep::printJson (std::ostream &os)
{
	os << "\"cmd\":\"sleep\",\"seconds\":" << sec;
}

int ElementWaitForIdle::defnextCommand (rts2core::DevClient * client, rts2core::Command ** new_command, char new_device[DEVICE_NAME_SIZE])
{
	if (client->getConnection ()->queEmptyForOriginator (client) == false)
		return NEXT_COMMAND_KEEP;
	if ((client->getConnection ()->getState () & DEVICE_STATUS_MASK) == DEVICE_IDLE)
		return NEXT_COMMAND_NEXT;
	return NEXT_COMMAND_KEEP;
}
