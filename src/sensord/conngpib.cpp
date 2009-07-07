/* 
 * Connection for GPIB bus.
 * Copyright (C) 2007-2009 Petr Kubanek <petr@kubanek.net>
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

#include "conngpib.h"

using namespace rts2sensord;

int
ConnGpib::gpibWrite (const char *_buf)
{
	int ret;
	ret = ibwrt (gpib_dev, _buf, strlen (_buf));
	#ifdef DEBUG_EXTRA
	logStream (MESSAGE_DEBUG) << "write " << _buf << sendLog;
	#endif
	if (ret & ERR)
	{
		logStream (MESSAGE_ERROR) << "error writing " << _buf
			<< gpib_error_string (ret) << " " << ret << sendLog;
		return -1;
	}
	return 0;
}


int
ConnGpib::gpibRead (void *_buf, int blen)
{
	int ret;
	ret = ibrd (gpib_dev, _buf, blen);
	if (ret & ERR)
	{
		logStream (MESSAGE_ERROR) << "error reading " << _buf << " " 
			<< gpib_error_string (ret) << " " << ret << sendLog;
		return -1;
	}
	#ifdef DEBUG_EXTRA
	logStream (MESSAGE_DEBUG) << "dev " << gpib_dev << " read '" << (char *) _buf
		<< "' ret " << ret << sendLog;
	#endif
	return 0;
}


int
ConnGpib::gpibWriteRead (const char *_buf, char *val, int blen)
{
	int ret;
	ret = ibwrt (gpib_dev, _buf, strlen (_buf));
	if (ret & ERR)
	{
		logStream (MESSAGE_ERROR) << "error writing " << _buf << " " << ret <<
			sendLog;
		return -1;
	}
	#ifdef DEBUG_EXTRA
	logStream (MESSAGE_DEBUG) << "dev " << gpib_dev << " write " << _buf <<
		" ret " << ret << sendLog;
	#endif
	*val = '\0';
	ret = ibrd (gpib_dev, val, blen);
	val[ibcnt] = '\0';
	if (ret & ERR)
	{
		logStream (MESSAGE_ERROR) << "error reading reply from " << _buf <<
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
ConnGpib::gpibWaitSRQ ()
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
ConnGpib::init ()
{
	gpib_dev = ibdev (minor, pad, 0, T3s, 1, 0);
	if (gpib_dev < 0)
	{
		logStream (MESSAGE_ERROR) << "cannot init GPIB device on minor " <<
			minor << ", pad " << pad << sendLog;
		return -1;
	}
	return 0;
}


ConnGpib::ConnGpib (Rts2Block *_master):Rts2ConnNoSend (_master)
{
	gpib_dev = -1;

	minor = 0;
	pad = 0;
}


ConnGpib::~ConnGpib (void)
{
	ibclr (gpib_dev);
	ibonl (gpib_dev, 0);
}
