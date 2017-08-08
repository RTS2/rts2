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

SitechRotator::SitechRotator (const char ax, const char *name, rts2core::ConnSitech *conn, SitechMulti *sitechBase, const char *defaults):Rotator (0, NULL, name, true)
{
	setDeviceName (name);
	sitech = conn;
	axis = ax;
	base = sitechBase;

	setDefaultsFile (defaults);

	updated = false;

	last_errors = 0;

	createValue (r_pos, "REAL", "[cnts] encoder readout", false);
	createValue (t_pos, "TARGET", "[cnts] target position", false, RTS2_VALUE_WRITABLE);

	createValue (speed, "SPEED", "[deg/sec] maximal speed", false, RTS2_VALUE_WRITABLE | RTS2_DT_DEGREES);
	speed->setValueFloat (20);

	createValue (acceleration, "acceleration", "derotator acceleration", false);
	createValue (max_velocity, "max_velocity", "derotator maximal velocity", false);
	createValue (current, "current", "derotator current", false);
	createValue (pwm, "pwm", "derotator PWM", false);
	createValue (errors, "errors", "derotator errors (only for FORCE ONE)", false);
	createValue (errors_val, "errors_val", "derotator error value", false);
	createValue (auto_reset, "auto_reset", "errors which will be auto-reset", false, RTS2_VALUE_WRITABLE | RTS2_DT_HEX);
	auto_reset->setValueInteger (ERROR_TIMED_OVER_CURRENT);

	createValue (pos_error, "pos_error", "derotator position error", false);
	createValue (supply, "supply", "[V] derotator position error", false);
	createValue (pid_out, "pid_out", "derotator PID output", false);
	createValue (autoMode, "auto", "derotator auto mode", false, RTS2_DT_ONOFF | RTS2_VALUE_WRITABLE);
	createValue (ticks, "_ticks", "[cnts] counts per full turn", false);
	createValue (mclock, "mclock", "derotator clocks", false);
	createValue (temperature, "temperature", "[C] derotator CPU board temperature", false);
	createValue (PIDs, "PIDs", "derotator PIDs", false);

	ticks->setValueLong (67108864);
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

int SitechRotator::info ()
{
	int ret = base->callInfo ();
	if (ret)
		return ret;
	return Rotator::info ();
}

int SitechRotator::commandAuthorized (rts2core::Connection *conn)
{
	if (conn->isCommand ("stop"))
	{
		if (!conn->paramEnd ())
			return -2;
		sitech->siTechCommand (axis, "N");
		return 0;
	}
	else if (conn->isCommand ("reset_errors"))
	{
		if (!conn->paramEnd ())
			return -2;
		sitech->resetErrors ();
		return 0;
	}
	else if (conn->isCommand ("reset_controller"))
	{
		if (!conn->paramEnd ())
			return -2;
		sitech->resetController ();
		getConfiguration ();
		return 0;
	}
	else if (conn->isCommand ("go_auto"))
	{
		if (!conn->paramEnd ())
			return -2;
		goAuto ();
		return 0;
	}
	return Rotator::commandAuthorized (conn);
}

int SitechRotator::setValue (rts2core::Value *oldValue, rts2core::Value *newValue)
{
	if (oldValue == t_pos)
	{
		updated = true;
		return 0;
	}
	if (oldValue == autoMode)
	{
		sitech->siTechCommand (axis, ((rts2core::ValueBool *) newValue)->getValueBool () ? "A" : "M0");
		return 0;
	}
	return Rotator::setValue (oldValue, newValue);
}

void SitechRotator::setTarget (double tv)
{
	double paOff = getOffset ();

	double t_angle = tv - paOff;

	if (!std::isnan (getTargetMax ()))
	{
		while (t_angle > getTargetMax ())
			t_angle -= 360.0;
	}
	if (!std::isnan (getTargetMin ()))
	{
		while (t_angle < getTargetMin ())
			t_angle += 360.0;
	}
	if (!std::isnan (getTargetMin ()) && !std::isnan (getTargetMax ()))
	{
		if (t_angle < getTargetMin ())
		{
			if (getTargetMin () < (t_angle + 180) && (t_angle + 180) < getTargetMax ())
			{
				t_angle += 180;
				paOff -= 180;
			}
			else if (getTargetMin () < (t_angle + 90) && (t_angle + 90) < getTargetMax ())
			{
				t_angle += 90;
				paOff -= 90;
			}
			else if (getTargetMin () < (t_angle + 270) && (t_angle + 270) < getTargetMax ())
			{
				t_angle += 270;
				paOff -= 270;
			}
			else
			{
				logStream (MESSAGE_ERROR) << "cannot set target value to " << tv << sendLog;
				return;
			}
		}
		else if (t_angle > getTargetMax ())
		{
			if (getTargetMin () < (t_angle - 180) && (t_angle - 180) < getTargetMax ())
			{
				t_angle -= 180;
				paOff += 180;
			}
			else if (getTargetMin () < (t_angle - 90) && (t_angle - 90) < getTargetMax ())
			{
				t_angle -= 90;
				paOff += 90;
			}
			else if (getTargetMin () < (t_angle - 270) && (t_angle - 270) < getTargetMax ())
			{
				t_angle -= 270;
				paOff += 270;
			}
			else
			{
				logStream (MESSAGE_ERROR) << "cannot set target value to " << tv << sendLog;
				return;
			}
		}
	}

	double t_zangle = getZenithAngleFromTarget(t_angle);

	if (!std::isnan (t_zangle))
	{
		if (!std::isnan(getZenithAngleMax()))
		{
			while (t_zangle > getZenithAngleMax())
			{
				t_angle -= 360.0;
				t_zangle = getZenithAngleFromTarget(t_angle);
			}
		}
		if (!std::isnan(getZenithAngleMin()))
		{
			while (t_zangle < getZenithAngleMin())
			{
				t_angle += 360.0;
				t_zangle = getZenithAngleFromTarget(t_angle);
			}
		}
		if (!std::isnan(getZenithAngleMin()) && !std::isnan(getZenithAngleMax()))
		{
			if (t_zangle < getZenithAngleMin ())
			{
				if (getZenithAngleMin () < (t_zangle + 180) && (t_zangle + 180) < getZenithAngleMax ())
				{
					t_angle += 180;
					paOff -= 180;
				}
				else if (getZenithAngleMin () < (t_zangle + 90) && (t_zangle + 90) < getZenithAngleMax ())
				{
					t_angle += 90;
					paOff -= 90;
				}
				else if (getZenithAngleMin () < (t_zangle + 270) && (t_zangle + 270) < getZenithAngleMax ())
				{
					t_angle += 270;
					paOff -= 270;
				}
				else
				{
					logStream (MESSAGE_ERROR) << "cannot set zenith target value to " << t_zangle << sendLog;
					return;
				}
			}
			else if (t_angle > getZenithAngleMax ())
			{
				if (getZenithAngleMin () < (t_zangle - 180) && (t_zangle - 180) < getZenithAngleMax ())
				{
					t_angle -= 180;
					paOff += 180;
				}
				else if (getZenithAngleMin () < (t_zangle - 90) && (t_zangle - 90) < getZenithAngleMax ())
				{
					t_angle -= 90;
					paOff += 90;
				}
				else if (getZenithAngleMin () < (t_zangle - 270) && (t_zangle - 270) < getZenithAngleMax ())
				{
					t_angle -= 270;
					paOff += 270;
				}
				else
				{
					logStream (MESSAGE_ERROR) << "cannot set zenith target value to " << t_zangle << sendLog;
					return;
				}
			}
		}
	}

	t_pos->setValueLong ((t_angle - getZeroOffset ()) * ticks->getValueLong () / 360.0);
	setPAOffset (paOff);
	updated = true;
}

long SitechRotator::isRotating ()
{
	return (labs (r_pos->getValueLong () - t_pos->getValueLong ()) < 20000) ? -2 : USEC_SEC;
}

void SitechRotator::processAxisStatus (rts2core::SitechAxisStatus *der_status)
{
	if (axis == 'X')
	{
		r_pos->setValueLong (der_status->x_pos);
	}
	else
	{
		r_pos->setValueLong (der_status->y_pos);
	}

	setCurrentPosition (360 * ((double) r_pos->getValueLong () / ticks->getValueLong ()) + getZeroOffset () + getOffset ());

	// not stopped, not in manual mode
	autoMode->setValueBool ((der_status->extra_bits & (axis == 'X' ? 0x02 : 0x20)) == 0);
	mclock->setValueLong (der_status->mclock);
	temperature->setValueInteger (der_status->temperature);

	switch (sitech->sitechType)
	{
		case rts2core::ConnSitech::SERVO_I:
		case rts2core::ConnSitech::SERVO_II:
			break;
		case rts2core::ConnSitech::FORCE_ONE:
		{
			// upper nimble
			uint16_t der_val = (axis == 'X' ? der_status->x_last[0] : der_status->y_last[0]) << 4;
			der_val += axis == 'X' ? der_status->x_last[1] : der_status->y_last[1];

			// find all possible errors
			switch ((axis == 'X' ? der_status->x_last[0] : der_status->y_last[0]) & 0x0F)
			{
				case 0:
					errors_val->setValueInteger (der_val);
					errors->setValueString (sitech->findErrors (der_val));
					if (last_errors != der_val)
					{
						if (der_val == 0)
						{
							clearHWError ();
							logStream (MESSAGE_REPORTIT | MESSAGE_INFO) << "controller error values cleared" << sendLog;
						}
						else
						{
							raiseHWError ();
							logStream (MESSAGE_ERROR) << "controller error(s): " << errors->getValue () << sendLog;
						}
						last_errors = der_val;
						if (der_val & auto_reset->getValueInteger ())
						{
							logStream (MESSAGE_REPORTIT | MESSAGE_INFO) << "reseting error which can be auto-reset" << sendLog;
							sleep (2);
							goAuto ();
						}
					}
					break;

				case 1:
					current->setValueInteger (der_val);
					break;

				case 2:
					supply->setValueInteger (der_val);
					break;

				case 3:
					temperature->setValueInteger (der_val);
					break;

				case 4:
					pid_out->setValueInteger (der_val);
					break;
			}

			pos_error->addValue (*(int16_t*) &(axis == 'X' ? der_status->x_last[2] : der_status->y_last[2]), 20);
			pos_error->calculate ();
			break;
		}
	}
}

void SitechRotator::goAuto ()
{
		sitech->siTechCommand ('X', "A");
		sitech->siTechCommand ('Y', "A");
		getConfiguration ();
}
