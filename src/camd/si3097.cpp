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
		
		virtual int setValue (rts2core::Value *oldValue, rts2core::Value *newValue);

		virtual int startExposure ();
		virtual int endExposure (int ret);

		virtual int stopExposure ();

		virtual int doReadout ();

	private:
		const char *devicePath;

		int camera_fd;

		int clearBuffer ();

		// write command to camera
		//
		// @param c command to send to the camera
		// @return -1 on error, 0 on success
		int writeCommand (const char c);

		struct SI_DMA_CONFIG dma_config;

		rts2core::ValueBool *fan;
};

}

using namespace rts2camd;

SI3097::SI3097 (int argc, char **argv):Camera (argc, argv)
{
	memset (&dma_config, 0, sizeof (SI_DMA_CONFIG));

	createExpType ();

	createValue (fan, "FAN", "camera fan", true, RTS2_VALUE_WRITABLE);

	devicePath = "/dev/si3097";
	camera_fd = 0;

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


	// initialize camera serial port
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

	// initialize DMA
	dma_config.maxever = 32 * 1024 * 1024;
	dma_config.total = getWidth () * getHeight () * 2;
	dma_config.buflen = 1024 * 1024;
	dma_config.timeout = 1000; // 10 secs
	dma_config.config = SI_DMA_CONFIG_WAKEUP_ONEND;

	if (ioctl (camera_fd, SI_IOCTL_DMA_INIT, &dma_config))
	{
		logStream (MESSAGE_ERROR) << "cannot setup camera DMA:" << strerror (errno) << sendLog;
		return -1;
	}

	return 0;
}

int SI3097::info ()
{
	return Camera::info ();
}

int SI3097::setValue (rts2core::Value *oldValue, rts2core::Value *newValue)
{
	if (oldValue == fan)
	{
		return writeCommand (fan->getValueBool () ? 'S' : 'T') == 0 ? 0 : -2;
	}
	return Camera::setValue (oldValue, newValue);
}

int SI3097::startExposure ()
{
	// open shutter
	if (writeCommand ('A'))
		return -1;
	return 0;
}

int SI3097::endExposure (int ret)
{
	// close shutter
	if (writeCommand ('B'))
		return -1;
	return Camera::endExposure (ret);
}

int SI3097::stopExposure ()
{
	int ret = writeCommand ('0');
	if (ret)
		return ret;
	return Camera::stopExposure ();
}

int SI3097::doReadout ()
{
	return -2;
}

int SI3097::clearBuffer ()
{
	if (ioctl (camera_fd, SI_IOCTL_SERIAL_CLEAR, 0))
		return -1;
	return 0;
}

int SI3097::writeCommand (const char c)
{
	if (write (camera_fd, &c, 1) != 1)
	{
		logStream (MESSAGE_ERROR) << "cannot send command " << c << " to camera." << sendLog;
		return -1;
	}
	return 0;	
}

int main (int argc, char **argv)
{
	SI3097 device (argc, argv);
	return device.run ();
}
