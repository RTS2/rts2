/* 
 * Driver for ThorLaser laser source.
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

#include "connection/connscpi.h"

namespace rts2sensord
{

/**
 * Main class for ThorLabs PM 100D power measurement unit.
 *
 * @author Petr Kubanek <kubanek@fzu.cz>
 */
class ThorPM100D:public Sensor
{
	public:
		ThorPM100D (int argc, char **argv);
		virtual ~ThorPM100D ();

	protected:
		virtual int processOption (int opt);
		virtual int initHardware ();

	private:
		char *device_file;
		rts2core::ConnSCPI *usbtmcConn;
};

}

using namespace rts2sensord;

ThorPM100D::ThorPM100D (int argc, char **argv): Sensor (argc, argv)
{
	device_file = NULL;
	usbtmcConn = NULL;

	addOption ('f', NULL, 1, "serial port with the module (ussually /dev/ttyUSB for ThorLaser USB serial connection");
}

ThorPM100D::~ThorPM100D ()
{
	delete usbtmcConn;
}

int ThorPM100D::processOption (int opt)
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

int ThorPM100D::initHardware ()
{
	if (device_file == NULL)
	{
		logStream (MESSAGE_ERROR) << "you must specify device file (TTY port)" << sendLog;
		return -1;
	}

	usbtmcConn = new rts2core::ConnSCPI (device_file);

	usbtmcConn->setDebug (getDebug ());

	usbtmcConn->initGpib ();

	char buf[100];

	usbtmcConn->gpibWriteRead ("*IDN?\n", buf, 100);
	
	return 0;
}

int main (int argc, char **argv)
{
	ThorPM100D device (argc, argv);
	return device.run ();
}

