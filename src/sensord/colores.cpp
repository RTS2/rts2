/* 
 * Driver for B2 colores control board.
 * Copyright (C) 2011 Petr Kubanek <petr@kubanek.net>
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

#include "../../lib/rts2/connserial.h"

namespace rts2sensord
{

/**
 * Colores board control various I/O.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Colores:public Sensor
{
	public:
		Colores (int argc, char **argv);
		virtual ~Colores ();

	protected:
		virtual int processOption (int opt);
		virtual int initHardware ();
		virtual int info ();

	private:
		char *device_file;
		rts2core::ConnSerial *coloresConn;

		rts2core::ValueBool *filtA;
};

}

using namespace rts2sensord;

Colores::Colores (int argc, char **argv): Sensor (argc, argv)
{
	device_file = NULL;
	coloresConn = NULL;

	createValue (filtA, "A", "A filter", false, RTS2_VALUE_WRITABLE);

	addOption ('f', NULL, 1, "serial port with the module (ussually /dev/ttyUSB)");

	setIdleInfoInterval (10);
}

Colores::~Colores ()
{
	delete coloresConn;
}

int Colores::processOption (int opt)
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

int Colores::initHardware ()
{
	if (device_file == NULL)
	{
		logStream (MESSAGE_ERROR) << "you must specify device file (TTY port)" << sendLog;
		return -1;
	}

	coloresConn = new rts2core::ConnSerial (device_file, this, rts2core::BS57600, rts2core::C8, rts2core::NONE, 30);
	int ret = coloresConn->init ();
	if (ret)
		return ret;
	
	coloresConn->flushPortIO ();
	coloresConn->setDebug (true);

	return 0;
}

int Colores::info ()
{
	char buf[200];
	int ret = coloresConn->writeRead ("i\n", 1, buf, 199, '\n');
	if (ret < 0)
		return -1;

	return Sensor::info ();
}

int main (int argc, char **argv)
{
	Colores device (argc, argv);
	return device.run ();
}

