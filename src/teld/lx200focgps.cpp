/* 
 * Source demonstrating how to use multiple sensors.
 * Copyright (C) 2012 Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
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

#include "multidev.h"

#include "rts2lx200/tellx200gps.h"
#include "focusd.h"

#define EVENT_TIMER_TEST     RTS2_LOCAL_EVENT + 5060

/**
 * Focuser for LX200 telescope. As LX200 focusers are unable to count steps, this
 * estimate focuser movement based on time spend focusing in/out.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class LX200Foc:public rts2focusd::Focusd
{
	public:
		LX200Foc (int argc, char **argv, rts2teld::TelLX200GPS *tel);

	protected:
		virtual int setTo (double num);
		virtual double tcOffset () { return 0; }

		virtual bool isAtStartPosition () { return true; }

		virtual int isFocusing ();
		virtual int endFocusing ();
	
	private:
		rts2teld::TelLX200GPS *telConn;

		double start_focusing;
		double expected_end;
		double offset;
};



LX200Foc::LX200Foc (int argc, char **argv, rts2teld::TelLX200GPS *tel):Focusd (argc, argv)
{
	telConn = tel;

	start_focusing = NAN;
	expected_end = NAN;
	offset = NAN;

	setSpeed (10);
}

int LX200Foc::setTo (double num)
{
	offset = getPosition () - num;
	if (fabs (offset) < 0.1)
		return 0;
	start_focusing = getNow ();
	expected_end = start_focusing + estimateOffsetDuration (num);
	if (telConn->getSerialConn ()->writePort (offset > 0 ? ":F+#" : ":F-#", 4) < 0)
		return -1;
	return 0;
}

int LX200Foc::isFocusing ()
{
	if (expected_end < getNow ())
		return rts2focusd::Focusd::isFocusing ();
	return -2;
}

int LX200Foc::endFocusing ()
{
	telConn->getSerialConn ()->writePort (":FQ#", 4);
	return rts2focusd::Focusd::endFocusing ();
}

int main (int argc, char ** argv)
{
	rts2core::MultiDev md;
	rts2teld::TelLX200GPS *tel = new rts2teld::TelLX200GPS (argc, argv);
	md.push_back (tel);
	md.push_back (new LX200Foc (argc, argv, tel));

	return md.run ();
}
