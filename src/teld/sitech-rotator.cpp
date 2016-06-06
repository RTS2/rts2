/* 
 * Sitech rotator.
 * Copyright (C) 2016 Petr Kubanek <petr@kubanek.net>
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

#include "sitech-rotator.h"

using namespace rts2rotad;

SitechRotator::SitechRotator (const char ax, const char *name, rts2core::ConnSitech *conn):Rotator (0, (char **) &name)
{
	setDeviceName (name);
	sitech = conn;
	axis = ax;

	createValue (r_pos, "ENC", "[cnts] encoder readout", false);
	createValue (t_pos, "TARGET", "[cnts] target position", false);

	createValue (zeroOffs, "zero_offs", "[deg] zero offset", false, RTS2_VALUE_WRITABLE | RTS2_DT_DEGREES);
	zeroOffs->setValueFloat (0);

	createValue (offset, "OFFSET", "[deg] custom offset", false, RTS2_VALUE_WRITABLE | RTS2_DT_DEGREES);
	offset->setValueFloat (0);

	createValue (speed, "SPEED", "[deg/sec] maximal speed", false, RTS2_VALUE_WRITABLE | RTS2_DT_DEGREES);
	speed->setValueFloat (20);

	createValue (acceleration, "acceleration", "derotator acceleration", false);
	createValue (max_velocity, "max_velocity", "derotator maximal velocity", false);
	createValue (current, "current", "derotator current", false);
	createValue (pwm, "pwm", "derotator PWM", false);
	createValue (errors, "errors", "derotator errors (only for FORCE ONE)", false);
	createValue (errors_val, "errors_val", "derotator error value", false);
	createValue (pos_error, "pos_error", "derotator position error", false);
	createValue (supply, "supply", "[V] derotator position error", false);
	createValue (temp, "temp", "[F] derotator temperature", false);
	createValue (pid_out, "pid_out", "1st derotator PID output", false);
	createValue (autoMode, "auto", "derotator auto mode", false, RTS2_DT_ONOFF | RTS2_VALUE_WRITABLE);
	createValue (mclock, "mclock", "derotator clocks", false);
	createValue (temperature, "temperature", "[C] derotator CPU board temperature", false);
	createValue (PIDs, "PIDs", "derotator PIDs", false);
}

void SitechRotator::getConfiguration ()
{
	acceleration->setValueDouble (sitech->getSiTechValue (axis, "R"));
	max_velocity->setValueDouble (sitech->motorSpeed2DegsPerSec (sitech->getSiTechValue (axis, "S"), ticks->getValueLong ()));
	current->setValueDouble (sitech->getSiTechValue (axis, "C") / 100.0);
	pwm->setValueInteger (sitech->getSiTechValue (axis, "O"));
}

void SitechRotator::getPIDs ()
{
	PIDs->clear ();

	PIDs->addValue (sitech->getSiTechValue (axis, "PPP"));
	PIDs->addValue (sitech->getSiTechValue (axis, "III"));
	PIDs->addValue (sitech->getSiTechValue (axis, "DDD"));
}

int SitechRotator::setValue (rts2core::Value *oldValue, rts2core::Value *newValue)
{
	if (oldValue == t_pos)
	{
		//derSetTarget (newValue->getValueLong (), t_der2_pos->getValueLong ());
		return 0;
	}
	return Rotator::setValue (oldValue, newValue);
}

void SitechRotator::setTarget (double tv)
{
	int32_t cnts = (tv - zeroOffs->getValueFloat ()) * ticks->getValueLong () / 360.0;
	sitech->setPosition (axis, cnts, speed->getValueDouble ());
}

long SitechRotator::isRotating ()
{
	return -2;
}
