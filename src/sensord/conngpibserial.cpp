/* 
 * Connection for GPIB-serial manufactured by Serial.biz
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

#include "conngpibserial.h"

using namespace rts2sensord;

ConnGpibSerial::ConnGpibSerial (rts2core::Block *_master, const char *_device, rts2core::bSpeedT _baudSpeed, rts2core::cSizeT _cSize, rts2core::parityT _parity, const char *_sep):ConnGpib (), rts2core::ConnSerial (_device, _master, _baudSpeed, _cSize, _parity, 40)
{
	timeout = 3;

	sep = _sep;
	seplen = strlen (sep);
}

ConnGpibSerial::~ConnGpibSerial (void)
{
}

void ConnGpibSerial::initGpib ()
{
	if (rts2core::ConnSerial::init ())
		throw rts2core::Error ("cannot init Serial serial connection");
}

void ConnGpibSerial::gpibWriteBuffer (const char *cmd, int _len)
{
	if (rts2core::ConnSerial::writePort (cmd, _len) < 0)
		throw rts2core::Error ("cannot write command to serial port");
	if (rts2core::ConnSerial::writePort (sep, seplen) < 0)
		throw rts2core::Error ("cannot write end endline");
}

void ConnGpibSerial::gpibRead (void *reply, int &blen)
{
	readPort ((char *) reply, blen, sep);
}

void ConnGpibSerial::gpibWriteRead (const char *cmd, char *reply, int blen)
{
	gpibWriteBuffer (cmd, strlen (cmd));
	gpibRead (reply, blen);
}

void ConnGpibSerial::gpibWaitSRQ ()
{
	return;
}

void ConnGpibSerial::settmo (float _sec)
{
	timeout = _sec;
}
