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

Focusd::Focusd (int in_argc, char **in_argv):rts2core::Device (in_argc, in_argv, DEVICE_TYPE_FOCUS, "F0")
{
	temperature = NULL;

	linearOffset = NULL;
	slope = NULL;
	intercept = NULL;
	temperatureNightOnly = NULL;

	createValue (position, "FOC_POS", "focuser position", true); // reported by focuser, use FOC_TAR to set the target position
	createValue (target, "FOC_TAR", "focuser target position", true, RTS2_VALUE_WRITABLE);

	createValue (defaultPosition, "FOC_DEF", "default target value", true, RTS2_VALUE_WRITABLE);
	createValue (filterOffset, "FOC_FILTEROFF", "offset related to actual filter", true, RTS2_VALUE_WRITABLE);
	createValue (focusingOffset, "FOC_FOFF", "offset from focusing routine", true, RTS2_VALUE_WRITABLE);
	createValue (tempOffset, "FOC_TOFF", "temporary offset for focusing", true, RTS2_VALUE_WRITABLE);

	createValue (speed, "foc_speed", "[steps/sec] speed of focuser movement", false);
	speed->setValueDouble (0);

	filterOffset->setValueDouble (0);
	focusingOffset->setValueDouble (0);
	tempOffset->setValueDouble (0);

	addOption (OPT_START, "start-position", 1, "focuser start position (focuser will be set to this one, if initial position is detected");
}

int Focusd::processOption (int in_opt)
{
	switch (in_opt)
	{
		case OPT_START:
			defaultPosition->setValueCharArr (optarg);
			break;
		default:
			return rts2core::Device::processOption (in_opt);
	}
	return 0;
}

void Focusd::positionWritable ()
{
	position->setWritable ();
	updateMetaInformations (position);
}

int Focusd::initValues ()
{
	addConstValue ("FOC_TYPE", "focuser type", focType);

	if (isnan (defaultPosition->getValueDouble ()))
	{
		// refresh position values
		if (info ())
			return -1;
		target->setValueDouble (getPosition ());
		defaultPosition->setValueDouble (getPosition ());
	}
	else if (isAtStartPosition () == false)
	{
		setPosition (defaultPosition->getValueDouble ());
	}
	else
	{
		target->setValueDouble (getPosition ());
	}

	return rts2core::Device::initValues ();
}

int Focusd::idle ()
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
			sendValueAll (position);
			infoAll ();
			setTimeout (USEC_SEC);
			if (ret)
			{
				maskState (DEVICE_ERROR_MASK | FOC_MASK_FOCUSING | BOP_EXPOSURE, DEVICE_ERROR_HW | FOC_SLEEPING, "focusing finished with error");
			}
			else
			{
				maskState (FOC_MASK_FOCUSING | BOP_EXPOSURE, FOC_SLEEPING, "focusing finished without error");
				logStream (MESSAGE_INFO) << "focuser moved to " << position->getValueFloat () << sendLog;
			}
		}
	}
	else if (linearOffset != NULL && slope != NULL && intercept != NULL && temperature != NULL && temperatureNightOnly != NULL && linearOffset->getValueBool () == true && !isnan (slope->getValueFloat ()) && !isnan (intercept->getValueFloat ()) && !isnan (temperature->getValueDouble ()))
	{
		if (temperatureNightOnly->getValueBool () == false || getMasterState () == (SERVERD_NIGHT | SERVERD_ON) )
		{
			// calculate new position based on linear fit to temperature
			int np = round (slope->getValueFloat () * temperature->getValueFloat () + intercept->getValueFloat ());
			if (np != round (getPosition ()))
			{
				logStream (MESSAGE_INFO) << "temperature offseting focusing value from " << getPosition () << " to " << np << " at temperature " << temperature->getValueFloat () << sendLog;
				setPosition (np);
			}
		}
	}

	return rts2core::Device::idle ();
}

int Focusd::setPosition (float num)
{
	int ret;
	target->setValueDouble (num);
	sendValueAll (target);
	maskState (FOC_MASK_FOCUSING | BOP_EXPOSURE, FOC_FOCUSING | BOP_EXPOSURE, "focus change started", estimateOffsetDuration (num));
	logStream (MESSAGE_INFO) << "changing focuser position to " << num << sendLog;
	if ((!isnan (getFocusMin ()) && num < getFocusMin ()) || (!isnan (getFocusMax ()) && num > getFocusMax ()))
	{
		logStream (MESSAGE_ERROR) << "focuser outside of focuser extend " << num << sendLog;
		maskState (FOC_MASK_FOCUSING | BOP_EXPOSURE | DEVICE_ERROR_HW, DEVICE_ERROR_HW, "focus change is not possible - outside of the range");
		return -1;
	}
	ret = setTo (num);
	if (ret)
	{
		logStream (MESSAGE_ERROR) << "cannot set focuser to " << num << sendLog;
		maskState (FOC_MASK_FOCUSING | BOP_EXPOSURE | DEVICE_ERROR_HW, DEVICE_ERROR_HW, "focus change aborted");
		return ret;
	}
	updateOffsetsExtent ();
	return ret;
}

int Focusd::isFocusing ()
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

int Focusd::endFocusing ()
{
	return 0;
}

void Focusd::setFocusExtent (double foc_min, double foc_max)
{
	target->setMin (foc_min);
	target->setMax (foc_max);
	updateMetaInformations (target);

	defaultPosition->setMin (foc_min);
	defaultPosition->setMax (foc_max);
	updateMetaInformations (defaultPosition);

	updateOffsetsExtent ();
}

void Focusd::updateOffsetsExtent ()
{
	double targetDiffMin = target->getMin () - target->getValueFloat ();
	double targetDiffMax = target->getMax () - target->getValueFloat ();
	filterOffset->setMin (targetDiffMin + filterOffset->getValueFloat ());
	filterOffset->setMax (targetDiffMax + filterOffset->getValueFloat ());
	updateMetaInformations (filterOffset);

	focusingOffset->setMin (targetDiffMin + focusingOffset->getValueFloat ());
	focusingOffset->setMax (targetDiffMax + focusingOffset->getValueFloat ());
	updateMetaInformations (focusingOffset);

	tempOffset->setMin (targetDiffMin + tempOffset->getValueFloat ());
	tempOffset->setMax (targetDiffMax + tempOffset->getValueFloat ());
	updateMetaInformations (tempOffset);
}

int Focusd::setValue (rts2core::Value * old_value, rts2core::Value * new_value)
{
        float tco = tcOffset();

	if (old_value == target)
	{
		return setPosition (new_value->getValueFloat ()) ? -2 : 0;
	}
	else if (old_value == defaultPosition)
	{
		return setPosition (new_value->getValueFloat () + filterOffset->getValueFloat () + focusingOffset->getValueFloat () + tempOffset->getValueFloat () + tco) ? -2 : 0;
	}
	else if (old_value == position)
	{
		if (writePosition (new_value->getValueDouble ()))
			return -2;

		// set target and default target to preset position
		target->setValueDouble (new_value->getValueDouble ());
		defaultPosition->setValueDouble (new_value->getValueDouble ());
		
		sendValueAll (target);
		sendValueAll (defaultPosition);
		return 0;
	}
	else if (old_value == filterOffset)
	{
		return setPosition (defaultPosition->getValueFloat () + new_value->getValueFloat () + focusingOffset->getValueFloat () + tempOffset->getValueFloat () + tco ) ? -2 : 0;
	}
	else if (old_value == focusingOffset)
	{
		return setPosition (defaultPosition->getValueFloat () + filterOffset->getValueFloat () + new_value->getValueFloat () + tempOffset->getValueFloat () + tco )? -2 : 0;
	}  
	else if (old_value == tempOffset)
	{
		return setPosition (defaultPosition->getValueFloat () + filterOffset->getValueFloat () + focusingOffset->getValueFloat () + new_value->getValueFloat () + tco )? -2 : 0;
	}
	return rts2core::Device::setValue (old_value, new_value);
}

void Focusd::createLinearOffset ()
{
	createValue (linearOffset, "linear_offset", "linear offset", false, RTS2_DT_ONOFF | RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);
	linearOffset->setValueBool (false);
	createValue (slope, "linear_slope", "slope parameter for linear function to fit temperature sensor", false, RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);
	createValue (intercept, "linear_intercept", "intercept parameter for 0 value of temperature sensor", false, RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);
	createValue (temperatureNightOnly, "linear_nightonly", "performs temperature based focusing only during night", false, RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);
	temperatureNightOnly->setValueBool (true);
}

int Focusd::scriptEnds ()
{
	tempOffset->setValueDouble (0);
	sendValueAll (tempOffset);
	setPosition (defaultPosition->getValueFloat () + filterOffset->getValueFloat () + focusingOffset->getValueFloat () + tempOffset->getValueFloat () + tcOffset());
	return rts2core::Device::scriptEnds ();
}

int Focusd::commandAuthorized (rts2core::Connection * conn)
{
	if (conn->isCommand ("help"))
	{
		conn->sendMsg ("info  - information about focuser");
		conn->sendMsg ("exit  - exit from connection");
		conn->sendMsg ("help  - print, what you are reading just now");
		return 0;
	}
	return rts2core::Device::commandAuthorized (conn);
}
