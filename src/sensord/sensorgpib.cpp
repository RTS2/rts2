/* 
 * Class for GPIB sensors.
 * Copyright (C) 2007-2008 Petr Kubanek <petr@kubanek.net>
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

#include "sensorgpib.h"

int
Rts2DevSensorGpib::gpibWrite (const char *buf)
{
	int ret;
	ret = ibwrt (gpib_dev, buf, strlen (buf));
	#ifdef DEBUG_EXTRA
	logStream (MESSAGE_DEBUG) << "write " << buf << sendLog;
	#endif
	if (ret & ERR)
	{
		logStream (MESSAGE_ERROR) << "error writing " << buf << sendLog;
		return -1;
	}
	return 0;
}


int
Rts2DevSensorGpib::gpibRead (void *buf, int blen)
{
	int ret;
	ret = ibrd (gpib_dev, buf, blen);
	if (ret & ERR)
	{
		logStream (MESSAGE_ERROR) << "error reading " << buf << " " << ret <<
			sendLog;
		return -1;
	}
	#ifdef DEBUG_EXTRA
	logStream (MESSAGE_DEBUG) << "dev " << gpib_dev << " read '" << (char *) buf
		<< "' ret " << ret << sendLog;
	#endif
	return 0;
}


int
Rts2DevSensorGpib::gpibWriteRead (const char *buf, char *val, int blen)
{
	int ret;
	ret = ibwrt (gpib_dev, buf, strlen (buf));
	if (ret & ERR)
	{
		logStream (MESSAGE_ERROR) << "error writing " << buf << " " << ret <<
			sendLog;
		return -1;
	}
	#ifdef DEBUG_EXTRA
	logStream (MESSAGE_DEBUG) << "dev " << gpib_dev << " write " << buf <<
		" ret " << ret << sendLog;
	#endif
	*val = '\0';
	ret = ibrd (gpib_dev, val, blen);
	val[ibcnt] = '\0';
	if (ret & ERR)
	{
		logStream (MESSAGE_ERROR) << "error reading reply from " << buf <<
			", readed " << val << " " << ret << sendLog;
		return -1;
	}
	#ifdef DEBUG_EXTRA
	logStream (MESSAGE_DEBUG) << "dev " << gpib_dev << " read " << val <<
		" ret " << ret << sendLog;
	#endif
	return 0;
}


int
Rts2DevSensorGpib::gpibWaitSRQ ()
{
	int ret;
	short res;
	while (true)
	{
		ret = iblines (gpib_dev, &res);
		if (ret & ERR)
		{
			logStream (MESSAGE_ERROR) << "Error while waiting for SRQ " << ret
				<< sendLog;
		}
		if (res & BusSRQ)
			return 0;
	}
}


int
Rts2DevSensorGpib::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'm':
			minor = atoi (optarg);
			break;
		case 'p':
			pad = atoi (optarg);
			break;
		default:
			return Rts2DevSensor::processOption (in_opt);
	}
	return 0;
}


int
Rts2DevSensorGpib::init ()
{
	int ret;
	ret = Rts2DevSensor::init ();
	if (ret)
		return ret;

	gpib_dev = ibdev (minor, pad, 0, T3s, 1, 0);
	if (gpib_dev < 0)
	{
		logStream (MESSAGE_ERROR) << "cannot init GPIB device on minor " <<
			minor << ", pad " << pad << sendLog;
		return -1;
	}
	return 0;
}


Rts2DevSensorGpib::Rts2DevSensorGpib (int in_argc, char **in_argv):Rts2DevSensor (in_argc,
in_argv)
{
	gpib_dev = -1;

	minor = 0;
	pad = 0;

	addOption ('m', "minor", 1, "board number (default to 0)");
	addOption ('p', "pad", 1, "device number (counted from 0, not from 1)");
}


Rts2DevSensorGpib::~Rts2DevSensorGpib (void)
{
	ibclr (gpib_dev);
	ibonl (gpib_dev, 0);
}
