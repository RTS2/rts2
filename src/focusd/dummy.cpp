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
	public:
		Dummy (int argc, char **argv);
		~Dummy (void);
		virtual int setTo (double num);
		virtual double tcOffset () {return 0.;};
		virtual int isFocusing ();

	protected:
		virtual int processOption (int opt);
		virtual int initValues ();

		virtual int setValue (rts2core::Value *old_value, rts2core::Value *new_value);

		virtual bool isAtStartPosition () { return false; }

	private:
		rts2core::ValueFloat *focSteps;
		rts2core::ValueDouble *focMin;
		rts2core::ValueDouble *focMax;
};

}

using namespace rts2focusd;

Dummy::Dummy (int argc, char **argv):Focusd (argc, argv)
{
	focType = std::string ("Dummy");
	createTemperature ();

	createValue (focMin, "foc_min", "minimal focuser value", false, RTS2_VALUE_WRITABLE);
	createValue (focMax, "foc_max", "maximal focuser value", false, RTS2_VALUE_WRITABLE);

	createValue (focSteps, "focstep", "focuser steps (step size per second)", false, RTS2_VALUE_WRITABLE);
	focSteps->setValueFloat (1);

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
			focSteps->setValueFloat (atof (optarg));
			break;
		default:
			return Focusd::processOption (opt);
	}
	return 0;
}

int Dummy::initValues ()
{
	position->setValueDouble (0);
	defaultPosition->setValueDouble (0);
	temperature->setValueFloat (100);
	return Focusd::initValues ();
}

int Dummy::setValue (rts2core::Value *old_value, rts2core::Value *new_value)
{
	if (old_value == focMin)
	{
		setFocusExtent (new_value->getValueDouble (), focMax->getValueDouble ());
		return 0;
	}
	else if (old_value == focMax)
	{
		setFocusExtent (focMin->getValueDouble (), new_value->getValueDouble ());
		return 0;
	}
	return Focusd::setValue (old_value, new_value);
}

int Dummy::setTo (double num)
{
	if (position->getValueDouble () > num)
		focSteps->setValueDouble (-1 * fabs (focSteps->getValueFloat ()));
	else	
		focSteps->setValueDouble (fabs (focSteps->getValueDouble ()));
	sendValueAll (focSteps);
	return 0;
}

int Dummy::isFocusing ()
{
	if (fabs (getPosition () - getTarget ()) < fabs (focSteps->getValueInteger ()))
	{
		position->setValueInteger (getTarget ());
		return -2;
	}
	// else
	position->setValueInteger (getPosition () + focSteps->getValueInteger ());
	sendValueAll (position);
	return USEC_SEC;
}

int main (int argc, char **argv)
{
	Dummy device (argc, argv);
	return device.run ();
}
