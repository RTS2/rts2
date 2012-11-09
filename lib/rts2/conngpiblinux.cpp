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

#include "connection/conngpiblinux.h"

#include "connnosend.h"

using namespace rts2core;

void ConnGpibLinux::gpibWriteBuffer (const char *cmd, int len)
{
	int ret;
	ret = ibwrt (gpib_dev, cmd, len);
	if (debug)
	{
		rts2core::LogStream ls = logStream (MESSAGE_DEBUG);
		ls << ":;gpibWriteBuffer write ";
		ls.logArr (cmd, len);
		ls << sendLog;
	}
	if (ret & ERR)
		throw GpibLinuxError ("error while writing to GPIB bus", cmd, iberr);
}

void ConnGpibLinux::gpibRead (void *reply, int &blen)
{
	int ret;
	ret = ibrd (gpib_dev, reply, blen);
	if (ret & ERR)
		throw GpibLinuxError ("error while pure reading from GPIB bus", (const char *) reply, iberr);
	blen = ibcnt;
	if (debug)
	{
		rts2core::LogStream ls = logStream (MESSAGE_DEBUG);
		ls << "::gpibRead dev " << gpib_dev << " read ";
		ls.logArr ((char *) reply, blen);
		ls << sendLog;
	}
}

void ConnGpibLinux::gpibWriteRead (const char *cmd, char *reply, int blen)
{
	int ret;
	ret = ibwrt (gpib_dev, cmd, strlen (cmd));
	if (ret & ERR)
		throw GpibLinuxError ("error while writing to GPIB bus", cmd, iberr);

	if (debug)
		logStream (MESSAGE_DEBUG) << "dev " << gpib_dev << " write " << cmd <<
			" ret " << ret << sendLog;
	*reply = '\0';
	ret = ibrd (gpib_dev, reply, blen);
	reply[ibcnt] = '\0';
	if (ret & ERR)
		throw GpibLinuxError ("error while reading in write-read cycle from GPIB bus", cmd, iberr);
	if (debug)
	{
		rts2core::LogStream ls = logStream (MESSAGE_DEBUG);
		ls << "::gpibWriteRead dev " << gpib_dev << " read ";
		ls.logArr (reply, ibcnt);
		ls << sendLog;
	}
}

void ConnGpibLinux::gpibWaitSRQ ()
{
	short res;
	while (true)
	{
		iblines (interface_num, &res);
		if ((res & BusSRQ) || (ibsta & SRQI))
			return;
		if (ibsta & ERR)
			throw GpibLinuxError ("Error while waiting for SQR", iberr, ibsta);
	}
}

void ConnGpibLinux::initGpib ()
{
	timeout = 3;
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

void ConnGpibLinux::settmo (float _sec)
{
	timeout = _sec;
	ibtmo (gpib_dev, getTimeoutTmo (_sec));
	if (ibsta & ERR)
		throw rts2core::Error ("Cannot set device timeout");
}

ConnGpibLinux::ConnGpibLinux (int _minor, int _pad):ConnGpib ()
{
	gpib_dev = -1;

	minor = _minor;
	pad = _pad;

	timeout = NAN;
}

ConnGpibLinux::~ConnGpibLinux (void)
{
	ibclr (gpib_dev);
	ibonl (gpib_dev, 0);
}
