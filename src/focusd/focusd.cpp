/**
 * Copyright (C) 2005-2009 Petr Kubanek <petr@kubanek.net>
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

#include "focusd.h"

#define OPT_START  OPT_LOCAL + 235

using namespace rts2focusd;

Focusd::Focusd (int in_argc, char **in_argv):
Rts2Device (in_argc, in_argv, DEVICE_TYPE_FOCUS, "F0")
{
	temperature = NULL;

	createValue (position, "FOC_POS", "focuser position", true);
	createValue (target, "FOC_TAR", "focuser target position", true);

	createValue (defaultPosition, "FOC_DEF", "default target value", true);
	createValue (focusingOffset, "FOC_FOFF", "offset from focusing routine", true);
	createValue (tempOffset, "FOC_TOFF", "temporary offset for focusing", true);

	addOption (OPT_START, "start-position", 1,
		"focuser start position (focuser will be set to this one, if initial position is detected");
}


int
Focusd::processOption (int in_opt)
{
	switch (in_opt)
	{
		case OPT_START:
			defaultPosition->setValueCharArr (optarg);
			break;
		default:
			return Rts2Device::processOption (in_opt);
	}
	return 0;
}


void
Focusd::checkState ()
{
	if ((getState () & FOC_MASK_FOCUSING) == FOC_FOCUSING)
	{
		int ret;
		ret = isFocusing ();

		if (ret >= 0)
		{
			setTimeout (ret);
			sendValueAll (position);
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
Focusd::initValues ()
{
	addConstValue ("FOC_TYPE", "focuser type", focType);

	if (isAtStartPosition () == false)
	{
		setPosition (defaultPosition->getValueInteger ());
	}
	else
	{
		target->setValueInteger (getPosition ());
	}

	return Rts2Device::initValues ();
}


int
Focusd::idle ()
{
	checkState ();
	return Rts2Device::idle ();
}


int
Focusd::setPosition (int num)
{
	int ret;
	target->setValueInteger (num);
	sendValueAll (target);
	ret = setTo (num);
	if (ret)
	{
		logStream (MESSAGE_ERROR) << "Cannot set focuser to " << num << sendLog;
		return ret;
	}
	maskState (FOC_MASK_FOCUSING | BOP_EXPOSURE, FOC_FOCUSING | BOP_EXPOSURE, "focusing started");
	return ret;
}


int
Focusd::autoFocus (Rts2Conn * conn)
{
	/* ask for priority */

	maskState (FOC_MASK_FOCUSING, FOC_FOCUSING, "autofocus started");

	// command ("priority 50");

	return 0;
}


int
Focusd::isFocusing ()
{
	int ret;
	time_t now;
	time (&now);
	if (now > focusTimeout)
		return -1;
	ret = info ();
	if (ret)
		return -1;
	if (getPosition () != getTarget ())
		return USEC_SEC;
	return -2;
}


int
Focusd::endFocusing ()
{
	return 0;
}


int
Focusd::setValue (Rts2Value * old_value, Rts2Value * new_value)
{
	if (old_value == position || old_value == target)
	{
		return setPosition (new_value->getValueInteger ())? -2 : 0;
	}
	if (old_value == defaultPosition || old_value == focusingOffset || old_value == tempOffset)
	{
		return updatePosition (new_value->getValueInteger () - old_value->getValueInteger ())? -2 : 0;
	}
	return Rts2Device::setValue (old_value, new_value);
}


int
Focusd::scriptEnds ()
{
	updatePosition (-1 * tempOffset->getValueInteger ());
	tempOffset->setValueInteger (0);
	sendValueAll (tempOffset);
	return Rts2Device::scriptEnds ();
}

int
Focusd::commandAuthorized (Rts2Conn * conn)
{
	if (conn->isCommand ("help"))
	{
		conn->sendMsg ("info  - information about focuser");
		conn->sendMsg ("focus - auto focusing");
		conn->sendMsg ("exit  - exit from connection");
		conn->sendMsg ("help  - print, what you are reading just now");
		return 0;
	}
	else if (conn->isCommand ("focus"))
	{
		// CHECK_PRIORITY;

		return autoFocus (conn);
	}
	return Rts2Device::commandAuthorized (conn);
}
