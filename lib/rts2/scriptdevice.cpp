/* 
 * Class for devices with scripting support.
 * Copyright (C) 2007-2008 Petr Kubanek <petr@kubanek.net>
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

#include "scriptdevice.h"

using namespace rts2core;

ScriptDevice::ScriptDevice (int in_argc, char **in_argv, int in_device_type, const char *default_name):Device (in_argc, in_argv, in_device_type, default_name)
{
	createValue (scriptRepCount, "SCRIPREP", "script loop count", true, RTS2_VALUE_WRITABLE, 0);
	scriptRepCount->setValueInteger (0);

	createValue (runningScript, "SCRIPT", "script used to take this images", true, RTS2_DT_SCRIPT | RTS2_VALUE_WRITABLE, 0);
	runningScript->setValueCharArr ("");

	createValue (scriptComment, "SCR_COMM", "comment recorded for this script", true, RTS2_VALUE_WRITABLE, CAM_WORKING);

	createValue (commentNumber, "COMM_NUM", "comment order within current script", true, RTS2_VALUE_WRITABLE, CAM_WORKING);
	createValue (scriptStatus, "script_status", "script status", false, RTS2_VALUE_WRITABLE);
	scriptStatus->addSelVal ("run");
	scriptStatus->addSelVal ("pause");
	scriptStatus->addSelVal ("stop");

	createValue (scriptPosition, "scriptPosition", "position within script", false, RTS2_VALUE_WRITABLE, 0);
	scriptPosition->setValueInteger (0);

	createValue (scriptLen, "scriptLen", "length of the current script element", false, RTS2_VALUE_WRITABLE, 0);
	scriptPosition->setValueInteger (0);

	createValue (elementPosition, "elementPosition", "position of element within the script", false, RTS2_VALUE_WRITABLE);
}

int ScriptDevice::commandAuthorized (Rts2Connection * conn)
{
	if (conn->isCommand ("block_enter"))
	{
		blockEnter ();
		return 0;
	}
	else if (conn->isCommand ("block_exit"))
	{
		if (elementPosition->size () <= 0)
			return -2;
		blockExit ();
		return 0;
	}
	else if (conn->isCommand ("element_executed"))
	{
		elementExecuted ();
		return 0;
	}
	else if (conn->isCommand ("element_first"))
	{
		elementFirst ();
		return 0;
	}
	return Device::commandAuthorized (conn);
}

void ScriptDevice::blockEnter ()
{
	elementPosition->addValue (0);
	sendValueAll (elementPosition);
}

void ScriptDevice::blockExit ()
{
	elementPosition->removeLast ();
	sendValueAll (elementPosition);
}

void ScriptDevice::elementExecuted ()
{
	if (elementPosition->size () == 0)
		blockEnter ();

	std::vector <int>::iterator i = elementPosition->valueEnd () - 1;
	*i = *i + 1;
	elementPosition->changed ();

	sendValueAll (elementPosition);
}

void ScriptDevice::elementFirst ()
{
	if (elementPosition->size () == 0)
		blockEnter ();

	std::vector <int>::iterator i = elementPosition->valueEnd () - 1;
	*i = 0;
	elementPosition->changed ();

	sendValueAll (elementPosition);
}
