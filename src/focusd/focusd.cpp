/**
 * Copyright (C) 2005-2008 Petr Kubanek <petr@kubanek.net>
 * Copyright (C) 2005-2007 Stanislav Vitek
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "focuser.h"

Rts2DevFocuser::Rts2DevFocuser (int in_argc, char **in_argv):
Rts2Device (in_argc, in_argv, DEVICE_TYPE_FOCUS, "F0")
{
	focStepSec = 100;
	homePos = 750;
	startPosition = INT_MIN;

	focTemp = NULL;

	createValue (focPos, "FOC_POS", "focuser position", true, 0, 0, true);
	createValue (focTarget, "TARGET", "focuser target value", true);

	addOption ('o', "home", 1, "home position (default to 750!)");
	addOption ('p', "start_position", 1,
		"focuser start position (focuser will be set to this one, if initial position is detected");
}


int
Rts2DevFocuser::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'o':
			homePos = atoi (optarg);
			break;
		case 'p':
			startPosition = atoi (optarg);
			break;
		default:
			return Rts2Device::processOption (in_opt);
	}
	return 0;
}


void
Rts2DevFocuser::checkState ()
{
	if ((getState () & FOC_MASK_FOCUSING) == FOC_FOCUSING)
	{
		int ret;
		ret = isFocusing ();

		if (ret >= 0)
		{
			setTimeout (ret);
			sendValueAll (focPos);
		}
		else
		{
			ret = endFocusing ();
			infoAll ();
			setTimeout (USEC_SEC);
			if (ret)
				maskState (DEVICE_ERROR_MASK | FOC_MASK_FOCUSING | BOP_EXPOSURE,
					DEVICE_ERROR_HW | FOC_SLEEPING,
					"focusing finished with error");
			else
				maskState (FOC_MASK_FOCUSING | BOP_EXPOSURE, FOC_SLEEPING,
					"focusing finished without errror");
		}
	}
}


int
Rts2DevFocuser::initValues ()
{
	addConstValue ("FOC_TYPE", "focuser type", focType);

	return Rts2Device::initValues ();
}


int
Rts2DevFocuser::idle ()
{
	checkState ();
	return Rts2Device::idle ();
}


int
Rts2DevFocuser::ready (Rts2Conn * conn)
{
	int ret;

	ret = ready ();
	if (ret)
	{
		conn->sendCommandEnd (DEVDEM_E_HW, "focuser not ready");
		return -1;
	}
	return 0;
}


int
Rts2DevFocuser::setTo (int num)
{
	int ret;
	int steps;
	ret = info ();
	if (ret)
		return ret;

	setFocTarget (num);
	steps = num - getFocPos ();
	setFocusTimeout ((int) ceil (abs (steps) / focStepSec) + 5);

	return stepOut (steps);
}


int
Rts2DevFocuser::home ()
{
	return setTo (homePos);
}


int
Rts2DevFocuser::stepOut (Rts2Conn * conn, int num)
{
	int ret;
	ret = info ();
	if (ret)
		return ret;

	setFocTarget (getFocPos () + num);
	setFocusTimeout ((int) ceil (abs (num) / focStepSec) + 5);

	ret = stepOut (num);
	if (ret)
	{
		if (conn)
		{
			conn->sendCommandEnd (DEVDEM_E_HW, "cannot step out");
		}
	}
	else
	{
		maskState (FOC_MASK_FOCUSING | BOP_EXPOSURE,
			FOC_FOCUSING | BOP_EXPOSURE, "focusing started");
	}
	return ret;
}


int
Rts2DevFocuser::setTo (Rts2Conn * conn, int num)
{
	int ret;
	ret = setTo (num);
	if (ret && conn)
		conn->sendCommandEnd (DEVDEM_E_HW, "cannot step out");
	else
		maskState (FOC_MASK_FOCUSING | BOP_EXPOSURE, FOC_FOCUSING | BOP_EXPOSURE,
			"focusing started");
	return ret;
}


int
Rts2DevFocuser::home (Rts2Conn * conn)
{
	int ret;
	ret = home ();
	if (ret)
		conn->sendCommandEnd (DEVDEM_E_HW, "cannot home focuser");
	else
		maskState (FOC_MASK_FOCUSING | BOP_EXPOSURE, FOC_FOCUSING | BOP_EXPOSURE,
			"homing started");
	return ret;
}


int
Rts2DevFocuser::autoFocus (Rts2Conn * conn)
{
	/* ask for priority */

	maskState (FOC_MASK_FOCUSING, FOC_FOCUSING, "autofocus started");

	// command ("priority 50");

	return 0;
}


int
Rts2DevFocuser::isFocusing ()
{
	int ret;
	time_t now;
	time (&now);
	if (now > focusTimeout)
		return -1;
	ret = info ();
	if (ret)
		return -1;
	if (getFocPos () != getFocTarget ())
		return USEC_SEC;
	return -2;
}


int
Rts2DevFocuser::endFocusing ()
{
	return 0;
}


void
Rts2DevFocuser::setFocusTimeout (int timeout)
{
	time (&focusTimeout);
	focusTimeout += timeout;
}


int
Rts2DevFocuser::checkStartPosition ()
{
	if (startPosition != INT_MIN && isAtStartPosition ())
	{
		return setTo (NULL, startPosition);
	}
	return 0;
}


int
Rts2DevFocuser::setValue (Rts2Value * old_value, Rts2Value * new_value)
{
	if (old_value == focPos || old_value == focTarget)
	{
		return setTo (NULL, new_value->getValueInteger ())? -2 : 1;
	}
	return Rts2Device::setValue (old_value, new_value);
}


int
Rts2DevFocuser::commandAuthorized (Rts2Conn * conn)
{
	if (conn->isCommand ("help"))
	{
		conn->sendMsg ("ready - is focuser ready?");
		conn->sendMsg ("info  - information about focuser");
		conn->sendMsg ("step  - move by given steps offset");
		conn->sendMsg ("set   - set to given position");
		conn->sendMsg ("focus - auto focusing");
		conn->sendMsg ("exit  - exit from connection");
		conn->sendMsg ("help  - print, what you are reading just now");
		return 0;
	}
	else if (conn->isCommand ("step"))
	{
		int num;
		// CHECK_PRIORITY;

		if (conn->paramNextInteger (&num) || !conn->paramEnd ())
			return -2;

		return stepOut (conn, num);
	}
	else if (conn->isCommand ("set"))
	{
		int num;
		// CHECK_PRIORITY;

		if (conn->paramNextInteger (&num) || !conn->paramEnd ())
			return -2;

		return setTo (conn, num);
	}
	else if (conn->isCommand ("focus"))
	{
		// CHECK_PRIORITY;

		return autoFocus (conn);
	}
	else if (conn->isCommand ("home"))
	{
		if (!conn->paramEnd ())
			return -2;
		return home (conn);
	}
	else if (conn->isCommand ("switch"))
	{
		int switch_num;
		int new_state;
		if (conn->paramNextInteger (&switch_num)
			|| conn->paramNextInteger (&new_state) || !conn->paramEnd ())
			return -2;
		return setSwitch (switch_num, new_state);
	}
	return Rts2Device::commandAuthorized (conn);
}
