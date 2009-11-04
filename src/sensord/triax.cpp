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

	protected:
		virtual int init ();
		virtual int initValues ();
	
	private:
		Rts2ValueString *mainVersion;
		Rts2ValueString *bootVersion;

		void getValue (Rts2Value *val, const char *cmd);
};

}

using namespace rts2sensord;

Triax::Triax (int argc, char **argv):Gpib (argc, argv)
{
	createValue (mainVersion, "main_version", "version of main firmware", false);
	createValue (bootVersion, "boot_version", "version of boot firmware", false);
}

int Triax::init ()
{
	int ret = Gpib::init ();
	if (ret)
		return ret;

	try
	{
		gpibWrite ("\222");
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
				break;
			case 'F':
				break;
			default:
				logStream (MESSAGE_ERROR) << "Unknown response from WHERE I AM command " << *buf << sendLog;
				return -1;
		}
	}
	catch (rts2core::Error er)
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
	catch (rts2core::Error er)
	{
		logStream (MESSAGE_ERROR) << er << sendLog;
		return -1;
	}
	return 0;
}

void Triax::getValue (Rts2Value *val, const char *cmd)
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

int main (int argc, char **argv)
{
	Triax device = Triax (argc, argv);
	return device.run ();
}
