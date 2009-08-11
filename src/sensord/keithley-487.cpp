/* 
 * Driver for Keithley 487 picoAmpermeter.
 * Copyright (C) 2008-2009 Petr Kubanek <petr@kubanek.net>
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

#include "../utils/error.h"

namespace rts2sensord
{

struct keithG4Header
{
	// header
	char type1;
	char type2;
	char reserved;
	char instrset;
	int16_t bytecount;
};

class Keithley487:public Gpib
{
	private:
		Rts2ValueFloat * curr;
		Rts2ValueBool *sourceOn;
		Rts2ValueDoubleMinMax *voltage;
		Rts2ValueSelection *zeroCheck;
	protected:
		virtual int init ();
		virtual int info ();
		virtual int setValue (Rts2Value * old_value, Rts2Value * new_value);
	public:
		Keithley487 (int argc, char **argv);
};

};

using namespace rts2sensord;

Keithley487::Keithley487 (int argc,char **argv):Gpib (argc, argv)
{
	createValue (curr, "CURRENT", "Measured current", true, RTS2_VWHEN_BEFORE_END, 0, false);
	createValue (sourceOn, "ON", "If voltage source is switched on", true);
	createValue (voltage, "VOLTAGE", "Voltage level", true);

	voltage->setValueDouble (0);
	voltage->setMin (-70);
	voltage->setMax (0);

	createValue (zeroCheck, "ZERO_CHECK", "Zero check on/off", true);
	zeroCheck->addSelVal ("off");
	zeroCheck->addSelVal ("on, no correction");
	zeroCheck->addSelVal ("on, with correction");
}


int
Keithley487::init ()
{
	int ret;
	ret = Gpib::init ();
	if (ret)
		return ret;
	// binary for Intel
	try
	{
		gpibWrite ("G4C0X");
	}
	catch (rts2core::Error er)
	{
		logStream (MESSAGE_ERROR) << er << sendLog;
		return -1;
	}
	return 0;
}


int
Keithley487::info ()
{
	struct
	{
		struct keithG4Header header;
		char value[4];
	} oneShot;
	try
	{
		int blen = 10;
		gpibWrite ("N1T5X");
		gpibRead (&oneShot, blen);
	
		// scale properly..
		curr->setValueFloat (*((float *) (&(oneShot.value))) * 1e+12);
	}
	catch (rts2core::Error er)
	{
		logStream (MESSAGE_ERROR) << er << sendLog;
		return -1;
	}

	return Gpib::info ();
}


int Keithley487::setValue (Rts2Value * old_value, Rts2Value * new_value)
{
	char buf[50];
	try
	{
		if (old_value == sourceOn)
		{
			// operate..
			strcpy (buf, "O0X");
			if (((Rts2ValueBool *) new_value)->getValueBool ())
				buf[1] = '1';
			gpibWrite (buf);
			return 0;
		}
		if (old_value == voltage)
		{
			snprintf (buf, 50, "V%f,%iX", new_value->getValueFloat (), fabs (new_value->getValueFloat ()) > 50 ? 1 : 0);
			gpibWrite (buf);
			return 0;
		}
		if (old_value == zeroCheck)
		{
			snprintf (buf, 50, "C%iX", new_value->getValueInteger ());
			gpibWrite (buf);
			return 0;
		}
	}
	catch (rts2core::Error er)
	{
		logStream (MESSAGE_ERROR) << er << sendLog;
		return -1;
	}
	return Gpib::setValue (old_value, new_value);
}


int
main (int argc, char **argv)
{
	Keithley487 device = Keithley487 (argc, argv);
	return device.run ();
}
