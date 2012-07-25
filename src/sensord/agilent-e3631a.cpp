/* 
 * Driver for HP Agilent E3631A power supply
 * Copyright (C) 2012 Petr Kubanek., Institute of Physics <kubanek@fzu.cz>
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

#include "sensorgpib.h"

#include "error.h"

namespace rts2sensord
{

/**
 * Driver for HP Agilent E3631A power supply.
 *
 * @author Petr Kubanek <kubanek@fzu.cz>
 */
class AgilentE3631A:public Gpib
{
	public:
		AgilentE3631A (int argc, char **argv);
		virtual ~ AgilentE3631A (void);

		virtual int info ();

	protected:
		virtual int initHardware ();
		virtual int initValues ();

		virtual int setValue (rts2core::Value * oldValue, rts2core::Value * newValue);

	private:
		rts2core::ValueFloat *p6v;
};

}

using namespace rts2sensord;

AgilentE3631A::AgilentE3631A (int argc, char **argv):Gpib (argc, argv)
{
	createValue (p6v, "P6V", "6V output voltage", true, RTS2_VALUE_WRITABLE);
}

AgilentE3631A::~AgilentE3631A (void)
{
}

int AgilentE3631A::info ()
{
	try
	{
		readValue ("MEAS:VOLT? P6V", p6v);
	}
	catch (rts2core::Error er)
	{
		logStream (MESSAGE_ERROR) << er << sendLog;
		return -1;
	}
	return Gpib::info ();
}

int AgilentE3631A::initHardware ()
{
	int ret = Gpib::initHardware ();
	if (ret)
		return ret;

	gpibWrite ("SYST:REM");
	gpibWrite ("*RST;*CLS");
	return 0;
}

int AgilentE3631A::initValues ()
{
	rts2core::ValueString *model = new rts2core::ValueString ("model");
	readValue ("*IDN?\n", model);
	addConstValue (model);
	return Gpib::initValues ();
}

int AgilentE3631A::setValue (rts2core::Value * oldValue, rts2core::Value * newValue)
{
	try
	{
		if (oldValue == p6v)
		{
			writeValue ("APPL P6V,3.0", newValue);
			return 0;
		}
	}
	catch (rts2core::Error er)
	{
		logStream (MESSAGE_ERROR) << er << sendLog;
		return -2;
	}
	return Gpib::setValue (oldValue, newValue);
}

int main (int argc, char **argv)
{
	AgilentE3631A device (argc, argv);
	return device.run ();
}
