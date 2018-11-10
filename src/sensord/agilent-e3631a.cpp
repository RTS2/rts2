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
		
		virtual int setValue (rts2core::Value * oldValue, rts2core::Value * newValue);

	private:
		rts2core::ValueFloat *p6v, *p25v, *n25v;
		rts2core::ValueFloat *p6a, *p25a, *n25a;

		rts2core::ValueFloat *s_p6v, *s_p25v, *s_n25v;
		rts2core::ValueFloat *s_p6a, *s_p25a, *s_n25a;

		rts2core::ValueBool *oper;

		void writeValue2 (const char *name, rts2core::Value *v1, rts2core::Value *v2);
};

}

using namespace rts2sensord;

AgilentE3631A::AgilentE3631A (int argc, char **argv):Gpib (argc, argv)
{
	createValue (p6v, "P6V", "[V] 6V output voltage", true);
	createValue (p6a, "P6A", "[A] 6V output current", true);
	createValue (p25v, "P25V", "[V] 25V output voltage", true);
	createValue (p25a, "P25A", "[A] 25V output current", true);
	createValue (n25v, "N25V", "[V] negative 25V output voltage", true);
	createValue (n25a, "N25A", "[A] negative 25V output current", true);

	createValue (s_p6v, "set_P6V", "[V] 6V output voltage", false, RTS2_VALUE_WRITABLE);
	createValue (s_p6a, "set_6A", "[A] 6V output current", false, RTS2_VALUE_WRITABLE);
	createValue (s_p25v, "set_P25V", "[V] 25V output voltage", false, RTS2_VALUE_WRITABLE);
	createValue (s_p25a, "set_P25A", "[A] 25V output current", false, RTS2_VALUE_WRITABLE);
	createValue (s_n25v, "set_N25V", "[V] negative 25V output voltage", false, RTS2_VALUE_WRITABLE);
	createValue (s_n25a, "set_N25A", "[A] negative 25V output current", false, RTS2_VALUE_WRITABLE);

	createValue (oper, "OPER", "output on/off", true, RTS2_VALUE_WRITABLE | RTS2_DT_ONOFF);

	s_p6v->setValueFloat (0);
	s_p6a->setValueFloat (0);
	s_p25v->setValueFloat (0);
	s_p25a->setValueFloat (0);
	s_n25v->setValueFloat (0);
	s_n25a->setValueFloat (0);
}

AgilentE3631A::~AgilentE3631A (void)
{
}

int AgilentE3631A::info ()
{
	try
	{
		readValue ("MEAS:VOLT? P6V", p6v);
		readValue ("MEAS:CURR? P6V", p6a);
		readValue ("MEAS:VOLT? P25V", p25v);
		readValue ("MEAS:CURR? P25V", p25a);
		readValue ("MEAS:VOLT? N25V", n25v);
		readValue ("MEAS:CURR? N25V", n25a);
		readValue ("OUTP?", oper);

		char buf[50];
		gpibWriteRead ("SYST:ERR?", buf, 50);
		if (! (buf[0] == '+' && buf[1] == '0'))
			logStream (MESSAGE_ERROR) << "SYSTEM ERROR " << buf << sendLog;
	}
	catch (rts2core::Error &er)
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
	sleep (1);

	return 0;
}

int AgilentE3631A::setValue (rts2core::Value * oldValue, rts2core::Value * newValue)
{
	try
	{
		if (oldValue == s_p6v)
		{
			writeValue2 ("APPL P6V", newValue, s_p6a);
			return 0;
		}
		else if (oldValue == s_p6a)
		{
			writeValue2 ("APPL P6V", s_p6v, newValue);
			return 0;
		}
		else if (oldValue == s_p25v)
		{
			writeValue2 ("APPL P25V", newValue, s_p25a);
			return 0;
		}
		else if (oldValue == s_p25a)
		{
			writeValue2 ("APPL P25V", s_p25v, newValue);
			return 0;
		}
		else if (oldValue == s_n25v)
		{
			writeValue2 ("APPL N25V", newValue, s_n25a);
			return 0;
		}
		else if (oldValue == s_n25a)
		{
			writeValue2 ("APPL N25V", s_n25v, newValue);
			return 0;
		}
		else if (oldValue == oper)
		{
			writeValue ("OUTP", newValue);
			return 0;
		}
	}
	catch (rts2core::Error &er)
	{
		logStream (MESSAGE_ERROR) << er << sendLog;
		return -2;
	}
	return Gpib::setValue (oldValue, newValue);
}

void AgilentE3631A::writeValue2 (const char *name, rts2core::Value *v1, rts2core::Value *v2)
{
	std::ostringstream _os;
	_os << name << "," << v1->getDisplayValue () << "," << v2->getDisplayValue ();
	gpibWrite (_os.str ().c_str ());
}


int main (int argc, char **argv)
{
	AgilentE3631A device (argc, argv);
	return device.run ();
}
