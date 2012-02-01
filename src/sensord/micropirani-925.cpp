/* 
 * Driver for MicroPirani 925 pressure gauge
 * Copyright (C) 2012 Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
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

#include "connection/serial.h"

namespace rts2sensord
{

/**
 * Driver for Brooks 356 Micor-Ion Plus Module.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class MicroPirani925: public Sensor
{
	public:
		MicroPirani925 (int argc, char **argv);

	protected:
		virtual int processOption (int _opt);
		virtual int initHardware ();

		virtual int info ();

	private:
		char *device_file;
		rts2core::ConnSerial *microConn;

		rts2core::ValueDouble *pressure;
};

}

using namespace rts2sensord;

int MicroPirani925::processOption (int _opt)
{
	switch (_opt)
	{
		case 'f':
			device_file = optarg;
			break;
		default:
			return Sensor::processOption (_opt);
	}
	return 0;
}

int MicroPirani925::initHardware ()
{
	int ret;
	if (device_file == NULL)
	{
		logStream (MESSAGE_ERROR) << "you must specify device file (TTY port)" << sendLog;
		return -1;
	}

	microConn = new rts2core::ConnSerial (device_file, this, rts2core::BS9600, rts2core::C8, rts2core::NONE, 10);
	ret = microConn->init ();
	if (ret)
		return ret;
	
	microConn->flushPortIO ();
	microConn->setDebug (true);

	return 0;
}

int MicroPirani925::info ()
{
	int ret;
	char buf[50];
	ret = microConn->writeRead ("@253PR1?;FF", 11, buf, 50, ";FF");
	if (ret < 0)
		return ret;
	std::istringstream is (buf + 7);
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

MicroPirani925::MicroPirani925 (int argc, char **argv): Sensor (argc, argv)
{
	device_file = NULL;
	microConn = NULL;

	createValue (pressure, "pressure", "sensor pressure", true);

	addOption ('f', NULL, 1, "serial port with the module (ussually /dev/ttyUSB for USB RS-456 conventor)");
}

int main (int argc, char **argv)
{
	MicroPirani925 device (argc, argv);
	return device.run ();
}
