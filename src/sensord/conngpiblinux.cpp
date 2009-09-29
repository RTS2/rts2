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

#include "conngpiblinux.h"

#include "../utils/rts2connnosend.h"

using namespace rts2sensord;

void ConnGpibLinux::gpibWrite (const char *cmd)
{
	int ret;
	ret = ibwrt (gpib_dev, cmd, strlen (cmd));
	#ifdef DEBUG_EXTRA
	logStream (MESSAGE_DEBUG) << "write " << cmd << sendLog;
	#endif
	if (ret & ERR)
		throw GpibLinuxError ("error while writing to GPIB bus", cmd, ret);
}

void ConnGpibLinux::gpibRead (void *reply, int &blen)
{
	int ret;
	ret = ibrd (gpib_dev, reply, blen - 1);
	((char *)reply)[ibcnt] = '\0';
	if (ret & ERR)
		throw GpibLinuxError ("error while reading from GPIB bus", (const char *) reply, ret);
	blen = ibcnt;
	#ifdef DEBUG_EXTRA
	logStream (MESSAGE_DEBUG) << "dev " << gpib_dev << " read '" << (char *) reply
		<< "' ret " << ret << sendLog;
	#endif
}

void ConnGpibLinux::gpibWriteRead (const char *cmd, char *reply, int blen)
{
	int ret;
	ret = ibwrt (gpib_dev, cmd, strlen (cmd));
	if (ret & ERR)
		throw GpibLinuxError ("error while writing to GPIB bus", cmd, ret);

	#ifdef DEBUG_EXTRA
	logStream (MESSAGE_DEBUG) << "dev " << gpib_dev << " write " << cmd <<
		" ret " << ret << sendLog;
	#endif
	*reply = '\0';
	ret = ibrd (gpib_dev, reply, blen);
	reply[ibcnt] = '\0';
	if (ret & ERR)
		throw GpibLinuxError ("error while reading from GPIB bus", cmd, ret);
	#ifdef DEBUG_EXTRA
	logStream (MESSAGE_DEBUG) << "dev " << gpib_dev << " read " << reply <<
		" ret " << ret << sendLog;
	#endif
}

void ConnGpibLinux::gpibWaitSRQ ()
{
	short res;
	while (true)
	{
		iblines (interface_num, &res);
		if (ibsta & ERR)
			throw rts2core::Error ("Error while waiting for SQR");
		if (res & BusSRQ)
			return;
	}
}

void ConnGpibLinux::initGpib ()
{
	gpib_dev = ibdev (minor, pad, 0, T3s, 1, 0);
	if (gpib_dev < 0)
	{
		logStream (MESSAGE_ERROR) << "cannot init GPIB device on minor " <<
			minor << ", pad " << pad << sendLog;
		throw rts2core::Error ("cannot init GPIB device");
	}
	ibask (gpib_dev, IbaBNA, &interface_num);
	if (ibsta & ERR)
		throw rts2core::Error ("cannot find interface number");
}

void ConnGpibLinux::devClear ()
{
	ibclr (gpib_dev);
	if (ibsta & ERR)
		throw rts2core::Error ("Cannot clear device state");
}

ConnGpibLinux::ConnGpibLinux (int _minor, int _pad):ConnGpib ()
{
	gpib_dev = -1;

	minor = _minor;
	pad = _pad;
}

ConnGpibLinux::~ConnGpibLinux (void)
{
	ibclr (gpib_dev);
	ibonl (gpib_dev, 0);
}
