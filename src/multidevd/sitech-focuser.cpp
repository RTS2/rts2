/**
 * Driver for Sitech focuser
 * Copyright (C) 2016 Petr Kubanek
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

#include "sitech-focuser.h"
#include "constsitech.h"

using namespace rts2focusd;

#define POSITION_FACTOR     100

SitechFocuser::SitechFocuser (const char *name, rts2core::ConnSitech *sitech_c, const char *defaults, const char *extTemp):Focusd (0, NULL), SitechMultidev ()
{
	setDeviceName (name);
	setSitechConnection (sitech_c);

	last_errors = 0;

	if (extTemp != NULL)
	{
		if (createTemperature (extTemp) == 0)
			createLinearOffset ();
	}

	setDefaultsFile (defaults);

	createValue (encoder, "encoder", "encoder position", false);

	createValue (focSpeed, "speed", "focuser speed (in controller units)", false, RTS2_VALUE_WRITABLE);
	focSpeed->setValueLong (10000000);

	createValue (errors, "errors", "controller errors", false);
	createValue (errors_val, "error_str", "controller errors string", false);

	createValue (autoMode, "auto", "driver enabled", false, RTS2_DT_ONOFF | RTS2_VALUE_WRITABLE);
}

int SitechFocuser::setValue (rts2core::Value *oldValue, rts2core::Value *newValue)
{
	if (autoMode == oldValue)
	{
		sitech->siTechCommand ('Y', ((rts2core::ValueBool *) newValue)->getValueBool () ? "A" : "M0");
		return 0;
	}

	return Focusd::setValue (oldValue, newValue);
}

int SitechFocuser::info ()
{
	sitech->getAxisStatus ('X', axisStatus);

	position->setValueDouble (axisStatus.y_pos / POSITION_FACTOR);
	encoder->setValueLong (axisStatus.y_enc);

	autoMode->setValueBool ((axisStatus.extra_bits & AUTO_Y) == 0);

	uint16_t val = axisStatus.y_last[0] << 4;
	val += axisStatus.y_last[1];

	switch (axisStatus.y_last[0] & 0x0F)
	{
		case 0:
			errors_val->setValueInteger (val);
			errors->setValueString (sitech->findErrors (val));
			if (last_errors != val)
			{
				if (val == 0)
				{
					clearHWError ();
					logStream (MESSAGE_REPORTIT | MESSAGE_INFO) << "controller error values cleared" << sendLog;
				}
				else
				{
					raiseHWError ();
					logStream (MESSAGE_ERROR) << "controller error(s): " << errors->getValue () << sendLog;
				}
				last_errors = val;
			}
			break;
	}

	return Focusd::info ();	
}

int SitechFocuser::commandAuthorized (rts2core::Connection *conn)
{
	if (conn->isCommand ("go_auto"))
	{
		if (!conn->paramEnd ())
			return -2;
		sitech->siTechCommand ('Y', "A");
		return 0;
	}
	return Focusd::commandAuthorized (conn);
}

int SitechFocuser::setTo (double num)
{
	sitech->setPosition ('Y', num * POSITION_FACTOR, focSpeed->getValueLong ());

	return 0;
}

double SitechFocuser::tcOffset ()
{
	return 0;
}

bool SitechFocuser::isAtStartPosition ()
{
	return false;
}
