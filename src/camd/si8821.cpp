/*
 * Driver for Spectral Instruments SI8821 PCI board CCD
 * Copyright (C) 2015 Petr Kubanek <petr@kubanek.net>
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

#include "si8821.h"

namespace rts2camd
{
/**
 * Driver for SI 8821 CCD.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class SI8821:public Camera
{
	public:
		SI8821 (int argc, char **argv);
		virtual ~SI8821 ();

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
		const char *cameraCfg;

		int camera_fd;

		int verbose;

		int init_com (int baud, int parity, int bits, int stopbits, int buffersize);

		int clear_buffer ();

		int send_command (int cmd);

		/**
		 * @return -1 error, 0 no, 1 yes
		 */
		int send_command_yn (int cmd);

		int send_char (int data);
		int receive_char ();
	
		int expect_yn ();
};

}

using namespace rts2camd;

SI8821::SI8821 (int argc, char **argv):Camera (argc, argv)
{
	devicePath = "/dev/SIPCI_0";
	cameraCfg = NULL;

	camera_fd = 0;

	addOption ('f', NULL, 1, "SI device path; default to /dev/SIPCI_0");
	addOption ('c', NULL, 1, "SI camera configuration");
}

SI8821::~SI8821 ()
{
	if (camera_fd)
		close (camera_fd);
}

int SI8821::processOption (int opt)
{
	switch (opt)
	{
		case 'f':
			devicePath = optarg;
			break;
		case 'c':
			cameraCfg = optarg;
			break;
		default:
			return Camera::processOption (opt);
	}
	return 0;
}

int SI8821::initHardware ()
{
	camera_fd = open (devicePath, O_RDWR, 0);
	if (camera_fd < 0)
	{
		logStream (MESSAGE_ERROR) << "cannot open " << devicePath << ":" << strerror (errno) << sendLog;
		return -1;
	}

	verbose = 1;
	if (ioctl (camera_fd, SI_IOCTL_VERBOSE, &verbose) < 0)
	{
		logStream (MESSAGE_ERROR) << "cannot set camera to verbose mode" << sendLog;
		return -1;
	}

	if (init_com (250000, 0, 8, 1, 9000)) /* setup uart for camera */
	{
		logStream (MESSAGE_ERROR) << "cannot init uart for camera: " << strerror << sendLog;
		return -1;
	}

	/*if (cameraCfg)
	{
		if (load_camera_cfg (cameraCfg))
		{
			logStream (MESSAGE_ERROR) << "cannot load camera configuration file " << cameraCfg << sendLog;
			return -1;
		}
		logStream (MESSAGE_INFO) << "loaded configuration file " << cameraCfg << sendLog;
	}*/

	return 0;
}

int SI8821::info ()
{
	return Camera::info ();
}

int SI8821::setValue (rts2core::Value *oldValue, rts2core::Value *newValue)
{
	return Camera::setValue (oldValue, newValue);
}

int SI8821::startExposure ()
{
	// open shutter
	if (send_command_yn ('A') != 1)
		return -1;
	return 0;
}

int SI8821::endExposure (int ret)
{
	// close shutter
	if (send_command_yn ('B') != 1)
		return -1;
	return Camera::endExposure (ret);
}

int SI8821::stopExposure ()
{
	return Camera::stopExposure ();
}

int SI8821::doReadout ()
{
	return -2;
}

int SI8821::init_com (int baud, int parity, int bits, int stopbits, int buffersize)
{
	struct SI_SERIAL_PARAM serial;

	bzero( &serial, sizeof(struct SI_SERIAL_PARAM));

	serial.baud = baud;
	serial.buffersize = buffersize;
	serial.fifotrigger = 8;
	serial.bits = bits;
	serial.stopbits = stopbits;
	serial.parity = parity;

	serial.flags = SI_SERIAL_FLAGS_BLOCK;
	serial.timeout = 1000;
  
	return ioctl (camera_fd, SI_IOCTL_SET_SERIAL, &serial);
}

int SI8821::clear_buffer ()
{
	if (ioctl (camera_fd, SI_IOCTL_SERIAL_CLEAR, 0))
		return -1;
	else
		return 0;
}

int SI8821::send_command (int cmd)
{
  	clear_buffer ();    
	return send_char (cmd);
}

int SI8821::send_command_yn (int cmd)
{
	send_command (cmd);

	return expect_yn ();
}

int SI8821::send_char (int data)
{
	unsigned char wbyte, rbyte;
	int n;

	wbyte = (unsigned char) data;
	if (write (camera_fd,  &wbyte, 1) != 1)
	{
		logStream (MESSAGE_ERROR) << "cannot send char " << data << ":" << strerror << sendLog;
		return -1;
	}

	if ((n = read (camera_fd,  &rbyte, 1)) != 1)
	{
		logStream (MESSAGE_ERROR) << "cannot read reply to command " << data << ":" << strerror << sendLog;
		return -1;
	}

	return 0;
}

int SI8821::receive_char ()
{
	unsigned char rbyte;
	int n;

	if ((n = read (camera_fd,  &rbyte, 1)) != 1)
	{
		if (n<0)
		      logStream (MESSAGE_ERROR) << "receive_char read:" << strerror << sendLog;
		return -1;
	}
	else
	{
		return (int)rbyte;
	}
}

int SI8821::expect_yn ()
{
	int got;

	if ((got = receive_char()) < 0)
		return -1;

	if (got != 'Y' && got != 'N')
		return -1;

	return (got == 'Y');
}

int main (int argc, char **argv)
{
	SI8821 device (argc, argv);
	return device.run ();
}
