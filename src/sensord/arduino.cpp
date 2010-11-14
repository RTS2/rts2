/* 
 * Driver for Arduino laser source.
 * Copyright (C) 2010 Petr Kubanek <petr@kubanek.net>
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
 * Arduino used as simple I/O board. Arduino shuld run rts2.pde code.
 *
 * @author Petr Kubanek <kubanek@fzu.cz>
 */
class Arduino:public Sensor
{
	public:
		Arduino (int argc, char **argv);

	protected:
		virtual int processOption (int opt);
		virtual int init ();
		virtual int info ();
	private:
		char *device_file;
		rts2core::ConnSerial *arduinoConn;

		Rts2ValueBool *decHome;
		Rts2ValueBool *raHome;
		Rts2ValueBool *raLimit;
};

}

using namespace rts2sensord;

Arduino::Arduino (int argc, char **argv): Sensor (argc, argv)
{
	device_file = NULL;
	arduinoConn = NULL;

	createValue (decHome, "DEC_HOME", "DEC axis home sensor", false, RTS2_DT_ONOFF);
	createValue (raHome, "RA_HOME", "RA axis home sensor", false, RTS2_DT_ONOFF);
	createValue (raLimit, "RA_LIMIT", "RA limit switch", false, RTS2_DT_ONOFF);

	addOption ('f', NULL, 1, "serial port with the module (ussually /dev/ttyUSB for Arduino USB serial connection");

	setIdleInfoInterval (1);
}

int Arduino::processOption (int opt)
{
	switch (opt)
	{
		case 'f':
			device_file = optarg;
			break;
		default:
			return Sensor::processOption (opt);
	}
	return 0;
}

int Arduino::init ()
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

	arduinoConn = new rts2core::ConnSerial (device_file, this, rts2core::BS9600, rts2core::C8, rts2core::NONE, 10);
	ret = arduinoConn->init ();
	if (ret)
		return ret;
	
	arduinoConn->flushPortIO ();
	arduinoConn->setDebug (false);

	return 0;
}

int Arduino::info ()
{
	char buf[20];
	int ret = arduinoConn->writeRead ("?", 1, buf, 20, '\n');
	if (ret < 0)
		return -1;

	int i = atoi (buf);

	raLimit->setValueBool (i & 0x01);
	raHome->setValueBool (i & 0x02);
	decHome->setValueBool (i & 0x04);

	return Sensor::info ();
}

int main (int argc, char **argv)
{
	Arduino device (argc, argv);
	return device.run ();
}

