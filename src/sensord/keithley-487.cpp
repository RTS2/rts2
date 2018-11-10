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

#include "error.h"

namespace rts2sensord
{

struct keithG4Header
{
	// header
	uint8_t type1;
	uint8_t type2;
	uint8_t reserved;
	uint8_t instrset;
	int16_t bytecount;
};

class Keithley487:public Gpib
{
	private:
		rts2core::ValueFloat * curr;
		rts2core::ValueBool *sourceOn;
		rts2core::ValueDoubleMinMax *voltage;
		rts2core::ValueSelection *zeroCheck;
	protected:
		virtual int init ();
		virtual int info ();
		virtual int setValue (rts2core::Value * old_value, rts2core::Value * new_value);
	public:
		Keithley487 (int argc, char **argv);
};

};

using namespace rts2sensord;

Keithley487::Keithley487 (int argc,char **argv):Gpib (argc, argv)
{
	createValue (curr, "CURRENT", "[pA] measured current (in picoAmps)", true, RTS2_VWHEN_BEFORE_END, 0);
	createValue (sourceOn, "ON", "If voltage source is switched on", true, RTS2_VALUE_WRITABLE);
	createValue (voltage, "VOLTAGE", "[V] voltage level", true, RTS2_VALUE_WRITABLE);

	voltage->setValueDouble (0);
	voltage->setMin (-70);
	voltage->setMax (0);

	createValue (zeroCheck, "ZERO_CHECK", "Zero check on/off", true, RTS2_VALUE_WRITABLE);
	zeroCheck->addSelVal ("off");
	zeroCheck->addSelVal ("on, no correction");
	zeroCheck->addSelVal ("on, with correction");
}

int Keithley487::init ()
{
	int ret;
	ret = Gpib::init ();
	if (ret)
		return ret;
	// binary for Intel
	try
	{
		gpibWrite ("C0N1T5");
	}
	catch (rts2core::Error &er)
	{
		logStream (MESSAGE_ERROR) << er << sendLog;
		return -1;
	}
	return 0;
}

int Keithley487::info ()
{
	struct keithG4Header header;

	char buf[10];

	try
	{
		int blen = 10;
		gpibWrite ("G4X");
		gpibRead (buf, blen);

		memcpy (&(header), buf, 6);

		if (header.type1 == header.type2 && header.type1 == 0x81)
		{
			curr->setValueFloat ((*((float *) (buf + 6))) * 1e12);
		}
		else
		{
			logStream (MESSAGE_ERROR) << "invalid reply header - expected 0x81, got " << header.type1 << " " << header.type2 << sendLog;
			return -1;
		}
	}
	catch (rts2core::Error &er)
	{
		logStream (MESSAGE_ERROR) << er << sendLog;
		return -1;
	}

	return Gpib::info ();
}

int Keithley487::setValue (rts2core::Value * old_value, rts2core::Value * new_value)
{
	char buf[50];
	try
	{
		if (old_value == sourceOn)
		{
			// operate..
			strcpy (buf, "O0X");
			if (((rts2core::ValueBool *) new_value)->getValueBool ())
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
	catch (rts2core::Error &er)
	{
		logStream (MESSAGE_ERROR) << er << sendLog;
		return -1;
	}
	return Gpib::setValue (old_value, new_value);
}

int main (int argc, char **argv)
{
	Keithley487 device = Keithley487 (argc, argv);
	return device.run ();
}
