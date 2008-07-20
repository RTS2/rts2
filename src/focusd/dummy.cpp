/**
 * Dummy focuser device.
 * Copyright (C) 2005-2008 Petr Kubanek <petr@kubanek.net>
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

class Rts2DevFocuserDummy:public Rts2DevFocuser
{
	private:
		int steps;
	protected:
		virtual bool isAtStartPosition ()
		{
			return false;
		}
	public:
		Rts2DevFocuserDummy (int argc, char **argv);
		~Rts2DevFocuserDummy (void);
		virtual int initValues ();
		virtual int ready ();
		virtual int stepOut (int num);
		virtual int isFocusing ();
};

Rts2DevFocuserDummy::Rts2DevFocuserDummy (int in_argc, char **in_argv):
Rts2DevFocuser (in_argc, in_argv)
{
	focStepSec = 1;
	strcpy (focType, "Dummy");
	createFocTemp ();
}


Rts2DevFocuserDummy::~Rts2DevFocuserDummy ()
{
}


int
Rts2DevFocuserDummy::initValues ()
{
	focPos->setValueInteger (3000);
	focTemp->setValueFloat (100);
	return Rts2DevFocuser::initValues ();
}


int
Rts2DevFocuserDummy::ready ()
{
	return 0;
}


int
Rts2DevFocuserDummy::stepOut (int num)
{
	steps = 1;
	if (num < 0)
		steps *= -1;
	return 0;
}


int
Rts2DevFocuserDummy::isFocusing ()
{
	if (fabs (getFocPos () - focPositionNew) < fabs (steps))
		focPos->setValueInteger (focPositionNew);
	else
		focPos->setValueInteger (getFocPos () + steps);
	return Rts2DevFocuser::isFocusing ();
}


int
main (int argc, char **argv)
{
	Rts2DevFocuserDummy device = Rts2DevFocuserDummy (argc, argv);
	return device.run ();
}
