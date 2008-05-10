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

#include "rts2scriptdevice.h"

Rts2ScriptDevice::Rts2ScriptDevice (int in_argc, char **in_argv,
int in_device_type, const char *default_name):
Rts2Device (in_argc, in_argv, in_device_type, default_name)
{
	createValue (scriptRepCount, "SCRIPREP", "script loop count", true, 0, 0,
		true);
	scriptRepCount->setValueInteger (0);

	createValue (runningScript, "SCRIPT", "script used to take this images",
		true, RTS2_DT_SCRIPT, 0, true);
	runningScript->setValueString ("");

	createValue (scriptComment, "SCR_COMM", "comment recorded for this script",
		true, 0, CAM_WORKING, true);

	createValue (commentNumber, "COMM_NUM", "comment order within current script",
		true, 0, CAM_WORKING, true);

	createValue (scriptPosition, "scriptPosition", "position within script", false, 0, 0, true);
	scriptPosition->setValueInteger (0);

	createValue (scriptLen, "scriptLen", "length of the current script element", false, 0, 0, true);
	scriptPosition->setValueInteger (0);
}


int
Rts2ScriptDevice::setValue (Rts2Value * old_value, Rts2Value * new_value)
{
	if (
		old_value == scriptRepCount
		|| old_value == runningScript
		|| old_value == scriptComment
		|| old_value == commentNumber
		|| old_value == scriptPosition
		|| old_value == scriptLen
		)
		return 0;
	return Rts2Device::setValue (old_value, new_value);
}
