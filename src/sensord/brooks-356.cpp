/* 
 * Driver for Granville-Phillips Brooks 356 Micro-Ion Plus Module
 * Copyright (C) 2009 Petr Kubanek
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

#include "sensord.h"

#include "../utils/connserial.h"

namespace rts2sensord
{

/**
 * Driver for Brooks 356 Micor-Ion Plus Module.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Brooks356: public Sensor
{
	public:
		
		Brooks356 (int argc, char **argv);

	private:
		char *device_file;
		rts2core::ConnSerial *brookConn;

		Rts2ValueDouble *pressure;

		Rts2ValueInteger *address;

	protected:
		virtual int processOption (int _opt);
		virtual int init ();

		virtual int info ();
};

}

using namespace rts2sensord;

int Brooks356::processOption (int _opt)
{
	switch (_opt)
	{
		case 'f':
			device_file = optarg;
			return 0;
			break;
	}
	return Sensor::processOption (_opt);
}

int Brooks356::init ()
{
	int ret;
	ret = Sensor::init ();
	if (ret)
		return ret;

	if (device_file == NULL)
	{
		logStream (MESSAGE_ERROR) << "you must specify device file (TTY port)" << sendLog;
		return -1;
	}

	brookConn = new rts2core::ConnSerial (device_file, this, rts2core::BS19200, rts2core::C8, rts2core::NONE, 10);
	ret = brookConn->init ();
	if (ret)
		return ret;
	
	brookConn->flushPortIO ();
	brookConn->setDebug (true);

	return 0;
}

int Brooks356::info ()
{
	int ret;
	char buf[50];
	sprintf (buf, "#%02iRD\r", address->getValueInteger ());
	ret = brookConn->writeRead (buf, 6, buf, 50, '\r');
	if (ret < 0)
		return ret;
	std::istringstream is (buf);
	char c;
	is >> c;
	if (c != '*')
	{
		logStream (MESSAGE_ERROR) << "Invalid reply beggining, expected *, find :" << c << sendLog;
		return -1;
	}
	int radr;
	is >> radr;
	if (radr != address->getValueInteger ())
	{
		logStream (MESSAGE_ERROR) << "Invalid reply address, expected " << address->getValueInteger () << ", find " << radr << sendLog;
		return -1;
	}
	double v;
	is >> v;
	if (is.fail ())
	{
		logStream (MESSAGE_ERROR) << "failed to parse buffer " << buf << sendLog;
		return -1;
	}
	pressure->setValueDouble (v);
	return Sensor::info ();
}

Brooks356::Brooks356 (int argc, char **argv): Sensor (argc, argv)
{
	device_file = NULL;
	brookConn = NULL;

	createValue (pressure, "pressure", "sensor pressure", true);

	createValue (address, "address", "sensor RS-458 address", false);
	address->setValueInteger (1);

	addOption ('f', NULL, 1, "serial port with the module (ussually /dev/ttyUSB for USB RS-456 conventor)");
}

int main (int argc, char **argv)
{
	Brooks356 device = Brooks356 (argc, argv);
	return device.run ();
}
