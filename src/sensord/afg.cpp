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
class AFG:public Gpib
{
	public:
		AFG (int argc, char **argv);
		virtual ~AFG (void);

		virtual int info ();

	protected:
		virtual int setValue (rts2core::Value * old_value, rts2core::Value * new_value);

	private:
};

}

using namespace rts2sensord;

AFG::AFG (int in_argc, char **in_argv):Gpib (in_argc, in_argv)
{
	setBool01 ();
}

AFG::~AFG (void)
{
}

int AFG::setValue (rts2core::Value * old_value, rts2core::Value * new_value)
{
	try
	{

	}
	catch (rts2core::Error &er)
	{
		logStream (MESSAGE_ERROR) << "cannot set " << new_value->getName () << " " << er << sendLog;
		return -2;
	}
	return Gpib::setValue (old_value, new_value);
}

int AFG::info ()
{
	try
	{
	}
	catch (rts2core::Error &er)
	{
		logStream (MESSAGE_ERROR) << er << sendLog;
		setReplyWithValueName (true);
		return -1;
	}
	setReplyWithValueName (true);
	return 0;
}

int main (int argc, char **argv)
{
	AFG device = AFG (argc, argv);
	return device.run ();
}
