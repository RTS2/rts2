/* 
 * Extract gain and other parameters from Phillips Web Cam.
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

#include "sensord.h"
#include "rts2-config.h"
#ifdef RTS2_HAVE_SYS_IOCCOM_H
#include <sys/ioccom.h>
#endif // RTS2_HAVE_SYS_IOCCOM_H
#include "pwc-ioctl.h"

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

namespace rts2sensord
{

/**
 * Phillips web cam driver.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class PWC:public Sensor
{
	public:
		PWC (int argc, char **argv);
		virtual ~PWC ();

		virtual int info ();
	protected:
		virtual int processOption (int opt);
		virtual int init ();
	private:
		const char *videoDevice;
		int videoFd;
		rts2core::ValueInteger *gain;
};

};

using namespace rts2sensord;

PWC::PWC (int argc, char ** argv):Sensor (argc, argv)
{
	createValue (gain, "AGC", "Automatic Gain Controll", true);
	
	videoDevice = "/dev/video0";
	videoFd = -1;
	addOption ('f', NULL, 1, "video device file (default to /dev/video0)");
}

PWC::~PWC ()
{
	if (videoFd > 0)
		close (videoFd);
}

int PWC::info ()
{
	int dummy;
	if ( ioctl (videoFd, VIDIOCPWCGAGC, &dummy) == -1)
	{
		logStream (MESSAGE_ERROR) << "cannot get AGC " << strerror (errno) << sendLog;
		return -1;
	}
	gain->setValueInteger (dummy);
	return Sensor::info ();
}

int PWC::processOption (int opt)
{
	switch (opt)
	{
		case 'f':
			videoDevice = optarg;
			break;
		default:
			return Sensor::processOption (opt);
	}
	return 0;
}

int PWC::init ()
{
	int ret;
	ret = Sensor::init ();
	if (ret)
		return ret;
	videoFd = open (videoDevice, O_RDWR);
	if (videoFd < 0)
	{
		logStream (MESSAGE_ERROR) << "Cannot open file " << videoDevice << ":" << strerror (errno) << sendLog;
		return -1;
	}
	return 0;
}

int
main (int argc, char **argv)
{
	PWC device = PWC (argc, argv);
	return device.run ();
}
