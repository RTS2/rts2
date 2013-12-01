/*
 * Driver for Spectral Instruments SI3097 PCI board CCD
 * Copyright (C) 2013 Petr Kubanek <petr@kubanek.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "camd.h"

#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "si3097.h"

namespace rts2camd
{
/**
 * Driver for SI 3097 CCD. http://ccd.mii.cz
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class SI3097:public Camera
{
	public:
		SI3097 (int argc, char **argv);
		virtual ~SI3097 ();

	protected:
		virtual int processOption (int opt);
		virtual int initHardware ();

		virtual int info ();
		
		virtual int setCoolTemp (float new_temp);

		virtual int startExposure ();
		virtual int endExposure (int ret);

		virtual int stopExposure ();

		virtual int doReadout ();

	private:
		const char *devicePath;

		int camera_fd;
};

}

using namespace rts2camd;

SI3097::SI3097 (int argc, char **argv):Camera (argc, argv)
{
	createExpType ();
	createFilter ();
	createTempAir ();
	createTempCCD ();
	createTempSet ();

	devicePath = "/dev/si3097";
	camera_fd = -1;

	addOption ('f', NULL, 1, "device path - default to /dev/si3097");
}

SI3097::~SI3097 ()
{
	if (camera_fd)
		close (camera_fd);
}

int SI3097::processOption (int opt)
{
	switch (opt)
	{
		case 'f':
			devicePath = optarg;
			break;
		default:
			return Camera::processOption (opt);
	}
	return 0;
}

int SI3097::initHardware ()
{
	camera_fd = open (devicePath, O_RDWR, 0);
	if (camera_fd < 0)
	{
		logStream (MESSAGE_ERROR) << "cannot open " << devicePath << ":" << strerror (errno) << sendLog;
		return -1;
	}

	struct SI_SERIAL_PARAM si_param;
	si_param.flags = SI_SERIAL_FLAGS_BLOCK;
	si_param.baud = 19200;
	si_param.bits = 0;
	si_param.parity = 1;
	si_param.stopbits = 1;
	si_param.buffersize = 16384;
	si_param.fifotrigger = 8;
	si_param.timeout = 1000; /* 10 secs */

	if (ioctl (camera_fd, SI_IOCTL_SET_SERIAL, &si_param) < 0)
	{
		logStream (MESSAGE_ERROR) << "cannot set serial parameters on " << devicePath << ":" << strerror (errno) << sendLog;
		return -1;
	}

	return 0;
}

int SI3097::info ()
{
	return Camera::info ();
}

int SI3097::setCoolTemp (float new_temp)
{
	return Camera::setCoolTemp (new_temp);
}

int SI3097::startExposure ()
{
	return 0;
}

int SI3097::endExposure (int ret)
{
	return Camera::endExposure (ret);
}

int SI3097::stopExposure ()
{
	return Camera::stopExposure ();
}

int SI3097::doReadout ()
{
	return -2;
}

int main (int argc, char **argv)
{
	SI3097 device (argc, argv);
	return device.run ();
}
