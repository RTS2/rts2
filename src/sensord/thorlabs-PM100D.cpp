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

#include "connection/serial.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

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

	protected:
		virtual int processOption (int opt);
		virtual int initHardware ();

	private:
		char *device_file;
		rts2core::ConnSerial *usbtmcConn;

		int usbtmcFile;
};

}

using namespace rts2sensord;

ThorPM100D::ThorPM100D (int argc, char **argv): Sensor (argc, argv)
{
	device_file = NULL;
	usbtmcFile = -1;

	addOption ('f', NULL, 1, "serial port with the module (ussually /dev/ttyUSB for ThorLaser USB serial connection");
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

	usbtmcFile = open (device_file, O_RDWR);

	if (usbtmcFile == -1)
		return -1;

	ssize_t wsize = write (usbtmcFile, "*IDN?\n", 6);

	char buf[51];

	ssize_t rsize = read (usbtmcFile, buf, 50);
	if (rsize < 0)
	{
		logStream (MESSAGE_ERROR) << "cannot communicate with PM100D on " << device_file << sendLog;
		return -1;
	}

	logStream (MESSAGE_DEBUG) << "Communication with PM100D yeld " << rsize << sendLog;
	buf[rsize] = '\0';
	logStream (MESSAGE_DEBUG) << "Communication with PM100D yeld " << rsize << " characters: " << buf << sendLog;
	
	return 0;
}

int main (int argc, char **argv)
{
	ThorPM100D device (argc, argv);
	return device.run ();
}

