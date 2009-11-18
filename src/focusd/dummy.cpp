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

#include "focusd.h"

#define OPT_FOC_STEPS    OPT_LOCAL + 1001

namespace rts2focusd
{

/**
 * Class for dummy focuser. Can be used to create driver for new device.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Dummy:public Focusd
{
	private:
		Rts2ValueInteger *focSteps;
	protected:
		virtual int processOption (int opt);
		virtual int initValues ();

		virtual bool isAtStartPosition ()
		{
			return false;
		}
	public:
		Dummy (int argc, char **argv);
		~Dummy (void);
		virtual int setTo (int num);
		virtual int isFocusing ();
};

}

using namespace rts2focusd;

Dummy::Dummy (int argc, char **argv):Focusd (argc, argv)
{
	focType = std::string ("Dummy");
	createTemperature ();

	createValue (focSteps, "focstep", "focuser steps (step size per second)", false, RTS2_VALUE_WRITABLE);
	focSteps->setValueInteger (1);

	addOption (OPT_FOC_STEPS, "focstep", 1, "initial value of focuser steps");
}

Dummy::~Dummy ()
{
}

int Dummy::processOption (int opt)
{
	switch (opt)
	{
		case OPT_FOC_STEPS:
			focSteps->setValueInteger (atoi (optarg));
			break;
		default:
			return Focusd::processOption (opt);
	}
	return 0;
}

int Dummy::initValues ()
{
	temperature->setValueFloat (100);
	return Focusd::initValues ();
}

int Dummy::setTo (int num)
{
	if (position->getValueInteger () > num)
		focSteps->setValueInteger (-1 * fabs (focSteps->getValueInteger ()));
	else	
		focSteps->setValueInteger (fabs (focSteps->getValueInteger ()));
	sendValueAll (focSteps);
	return 0;
}

int Dummy::isFocusing ()
{
	if (fabs (getPosition () - getTarget ()) < fabs (focSteps->getValueInteger ()))
		position->setValueInteger (getTarget ());
	else
		position->setValueInteger (getPosition () + focSteps->getValueInteger ());
	return Focusd::isFocusing ();
}

int main (int argc, char **argv)
{
	Dummy device (argc, argv);
	return device.run ();
}
