/**
 * Driver for sitech mirror rotator
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

#include "sitech-mirror.h"
#include "constsitech.h"

using namespace rts2mirror;

/**
 * Driver for Sitech mirror rotator
 *
 * @author Petr Kubanek <petr@kubanek.net>>
 */

SitechMirror::SitechMirror (const char *name, rts2core::ConnSitech *sitech_c, const char *defaults):Mirror (0, NULL), SitechMultidev ()
{
	setDeviceName (name);
	setSitechConnection (sitech_c);

	setDefaultsFile (defaults);

	last_errors = 0;

	createValue (posA, "posA", "A position", false, RTS2_VALUE_WRITABLE);
	createValue (posB, "posB", "B position", false, RTS2_VALUE_WRITABLE);

	createValue (currPos, "current_pos", "current position", false);
	createValue (tarPos, "target_pos", "target position", false, RTS2_VALUE_WRITABLE);

	createValue (moveSpeed, "speed", "movement speed (controller units)", false, RTS2_VALUE_WRITABLE);
	moveSpeed->setValueLong (67000000);

	createValue (errors, "errors", "controller errors", false);
	createValue (errors_val, "error_str", "controller errors string", false);

	createValue (autoMode, "auto", "driver enabled", false, RTS2_DT_ONOFF | RTS2_VALUE_WRITABLE);

	addPosition ("1");
	addPosition ("2");
}

int SitechMirror::info ()
{
	sitech->getAxisStatus ('X', axisStatus);

	currPos->setValueLong (axisStatus.x_pos);

	autoMode->setValueBool ((axisStatus.extra_bits & AUTO_X) == 0);

	uint16_t val = axisStatus.x_last[0] << 4;
	val += axisStatus.x_last[1];

	switch (axisStatus.x_last[0] & 0x0F)
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

	return Mirror::info ();
}

int SitechMirror::commandAuthorized (rts2core::Connection *conn)
{
	if (conn->isCommand ("go_auto"))
	{
		if (!conn->paramEnd ())
			return -2;
		sitech->siTechCommand ('X', "A");
		return 0;
	}
	return Mirror::commandAuthorized (conn);
}

int SitechMirror::movePosition (int pos)
{
	long t = pos == 0 ? posA->getValueLong () : posB->getValueLong ();

	sitech->setPosition ('X', t, moveSpeed->getValueLong ());
	tarPos->setValueLong (t);

	return 0;
}

int SitechMirror::isMoving ()
{
	int ret = info ();
	if (ret)
		return -1;
	return abs (axisStatus.x_pos - tarPos->getValueLong ()) < 1000 ? -2: 100;
}

int SitechMirror::setValue (rts2core::Value* oldValue, rts2core::Value *newValue)
{
	if (oldValue == tarPos)
	{
		sitech->setPosition ('X', newValue->getValueLong (), moveSpeed->getValueLong ());
		return 0;
	}
	else if (oldValue == moveSpeed)
	{
		sitech->setPosition ('X', tarPos->getValueLong (), newValue->getValueLong ());
		return 0;
	}
	else if (autoMode == oldValue)
	{
		sitech->siTechCommand ('X', ((rts2core::ValueBool *) newValue)->getValueBool () ? "A" : "M0");
		return 0;
	}

	return Mirror::setValue (oldValue, newValue);
}
