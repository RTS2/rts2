/* 
 * Driver for Keithley 6485 and 6487 picoAmpermeter.
 * Copyright (C) 2008-2009 Petr Kubanek <petr@kubanek.net>
 * Copyright (C) 2008-2012 Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
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
 * CPX 400 SCPI regulated power supply driver. Manual available at
 * http://labrf.av.it.pt/Data/Manuais%20&%20Tutoriais/33%20-%20Power%20Supply%20CPX400DP/CPX400D%20&%20DP%20Instruction%20Manual.pdf
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class CPX400:public Gpib
{
	public:
		CPX400 (int argc, char **argv);
		virtual ~CPX400 (void);

		virtual int info ();

	protected:
		virtual int setValue (rts2core::Value * old_value, rts2core::Value * new_value);

	private:
		rts2core::ValueDoubleMinMax *svolt;
		rts2core::ValueDoubleMinMax *samps;
		rts2core::ValueBool *on;
};

}

using namespace rts2sensord;

CPX400::CPX400 (int in_argc, char **in_argv):Gpib (in_argc, in_argv)
{
	createValue (svolt, "VOLTAGE", "[V] power source voltage", true, RTS2_VALUE_WRITABLE);
	createValue (samps, "CURRENT", "[A] power source current", true, RTS2_VALUE_WRITABLE);
	createValue (on, "ON", "power state", true, RTS2_VALUE_WRITABLE | RTS2_DT_ONOFF);

	setBool01 ();
}

CPX400::~CPX400 (void)
{
}

int CPX400::setValue (rts2core::Value * old_value, rts2core::Value * new_value)
{
	try
	{
		if (old_value == svolt)
		{
			writeValue ("V1", new_value);
			return 0;
		}
		else if (old_value == samps)
		{
			writeValue ("I1", new_value);
			return 0;
		}
		else if (old_value == on)
		{
			writeValue ("OP1", new_value);
			return 0;
		}
	}
	catch (rts2core::Error &er)
	{
		logStream (MESSAGE_ERROR) << "cannot set " << new_value->getName () << " " << er << sendLog;
		return -2;
	}
	return Gpib::setValue (old_value, new_value);
}

int CPX400::info ()
{
	try
	{
		setReplyWithValueName (true);
		readValue ("V1?", svolt);
		readValue ("I1?", samps);
		setReplyWithValueName (false);
		readValue ("OP1?", on);
	}
	catch (rts2core::Error &er)
	{
		logStream (MESSAGE_ERROR) << er << sendLog;
		return -1;
	}
	return 0;
}

int main (int argc, char **argv)
{
	CPX400 device = CPX400 (argc, argv);
	return device.run ();
}
