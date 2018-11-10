/* 
 * JY488 & SPEX488 TRIAX Series controll.
 * Copyright (C) 2009 Petr Kubanek <petr@kubanek.net>
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

namespace rts2sensord
{
/**
 * This class provides access to functionality of TRIAX series
 * Jobin-Yvon/Horiba monochromators on GPIB/IEEE 488 bus.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Triax:public Gpib
{
	public:
		Triax (int argc, char **argv);

		virtual int info ();

	protected:
		virtual int init ();
		virtual int initValues ();

		virtual int setValue (rts2core::Value *oldValue, rts2core::Value *newValue);
	
	private:
		rts2core::ValueString *mainVersion;
		rts2core::ValueString *bootVersion;
		rts2core::ValueInteger *motorPosition;

		rts2core::ValueSelection *entryMirror;
		rts2core::ValueSelection *exitMirror;

		void initTriax ();
		// send command, wait for reply - 'o' comming from GPIB
		void sendCommand (const char *cmd);
		void getValue (rts2core::Value *val, const char *cmd);

		void setValue (char cmd, int p1, int p2);

		// test if motor is busy. Return false if motor is idle.
		bool isBusy (const char *cmd);
};

}

using namespace rts2sensord;

Triax::Triax (int argc, char **argv):Gpib (argc, argv)
{
	createValue (mainVersion, "main_version", "version of main firmware", false);
	createValue (bootVersion, "boot_version", "version of boot firmware", false);
	createValue (motorPosition, "MOTOR", "position of the motor", true, RTS2_VALUE_WRITABLE);

	createValue (entryMirror, "ENTRY", "position of entry mirror", true, RTS2_VALUE_WRITABLE);
	entryMirror->addSelVal ("SIDE");
	entryMirror->addSelVal ("FRONT");

	createValue (exitMirror, "EXIT", "position of exit mirror", true, RTS2_VALUE_WRITABLE);
	exitMirror->addSelVal ("SIDE");
	exitMirror->addSelVal ("FRONT");
}

int Triax::info ()
{
	try
	{
		getValue (motorPosition, "H0\r");
	}
	catch (rts2core::Error &er)
	{
		logStream (MESSAGE_ERROR) << er << sendLog;
		return -1;
	}
	return Gpib::info ();
}

int Triax::init ()
{
	int ret = Gpib::init ();
	if (ret)
		return ret;

	try
	{
		gpibWriteBuffer ("\222", 1);
		sleep (1);
		char buf[11];
		int l = 10;
		gpibWriteRead (" ", buf, 9);
		switch (*buf)
		{
			case 'B':
				gpibWriteBuffer ("O2000\0", 6);
				usleep (USEC_SEC / 2);
				gpibRead (buf, l);
				if (*buf != '*')
				{
					logStream (MESSAGE_ERROR) << "Wrong response from 'START MAIN PROGRAM'" << sendLog;
					return -1;
				}
				// init us..
				initTriax ();
				break;
			case 'F':
				break;
			default:
				logStream (MESSAGE_ERROR) << "Unknown response from WHERE I AM command " << *buf << sendLog;
				return -1;
		}
	}
	catch (rts2core::Error &er)
	{
		logStream (MESSAGE_ERROR) << er << sendLog;
		return -1;
	}
	return 0;
}

int Triax::initValues ()
{
	try
	{
		getValue (mainVersion, "z");
		getValue (bootVersion, "y");
	}
	catch (rts2core::Error &er)
	{
		logStream (MESSAGE_ERROR) << er << sendLog;
		return -1;
	}
	return 0;
}

int Triax::setValue (rts2core::Value *oldValue, rts2core::Value *newValue)
{
	try
	{
		if (oldValue == motorPosition)
		{
			setValue ('F', 0, newValue->getValueInteger () - motorPosition->getValueInteger ());
			return 0;
		}
		if (oldValue == entryMirror)
		{
			const char *cmd;
			switch (newValue->getValueInteger ())
			{
				case 0:
					cmd = "c0\r";
					break;
				case 1:
					cmd = "d0\r";
					break;
			}
			sendCommand (cmd);
			return 0;
		}
		if (oldValue == exitMirror)
		{
			const char *cmd;
			switch (newValue->getValueInteger ())
			{
				case 0:
					cmd = "e0\r";
					break;
				case 1:
					cmd = "f0\r";
					break;
			}
			sendCommand (cmd);
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

void Triax::initTriax ()
{
	settmo (100);
	sendCommand ("A");	
	settmo (10);
}

void Triax::sendCommand (const char *cmd)
{
	char buf[11];
	gpibWriteRead (cmd, buf, 10);
	if (*buf != 'o')
		throw rts2core::Error ("Invalid reply from device!");
}

void Triax::getValue (rts2core::Value *val, const char *cmd)
{
	char buf[21];
	int l = 20;
	gpibWriteRead (cmd, buf, l);
	if (*buf != 'o')
		throw rts2core::Error ("wrong response to command!");
	char *p = buf;
	p++;
	while (p - buf < l && *p != '\r')
		p++;
	if (p - buf >= l)
		throw rts2core::Error ("cannot find ending \\r (CR) in response!");
	*p = '\0';
	val->setValueString (buf + 1);
}

void Triax::setValue (char cmd, int p1, int p2)
{
	std::ostringstream _os;
	_os << cmd << p1 << "," << p2 << '\r';
	sendCommand (_os.str ().c_str ());
}

bool Triax::isBusy (const char *cmd)
{
	char buf[11];
	gpibWriteRead (cmd, buf, 10);
	if (buf[0] != 'o')
		throw rts2core::Error ("wrong response to is busy command!");
	return (buf[1] != 'z');
}

int main (int argc, char **argv)
{
	Triax device = Triax (argc, argv);
	return device.run ();
}
