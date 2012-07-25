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

ConnGpibSerial::ConnGpibSerial (rts2core::Block *_master, const char *_device):ConnGpib (), rts2core::ConnSerial (_device, _master, rts2core::BS9600, rts2core::C8, rts2core::NONE, 40)
{
	eot = 1;
	timeout = 3;
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
	if (rts2core::ConnSerial::writePort ('\n') < 0)
		throw rts2core::Error ("cannot write end endline");
}

void ConnGpibSerial::gpibRead (void *reply, int &blen)
{
	readUSBGpib ((char *) reply, blen);
}

void ConnGpibSerial::gpibWriteRead (const char *cmd, char *reply, int blen)
{
	if (rts2core::ConnSerial::writePort (cmd, strlen (cmd)) < 0)
		throw rts2core::Error ("cannot write to port");
	readUSBGpib (reply, blen);
}

void ConnGpibSerial::gpibWaitSRQ ()
{
	while (true)
	{
		if (rts2core::ConnSerial::writeRead ("*SRQ\n", 6, buf, 10, '\n') < 0)
			throw rts2core::Error ("cannot writeRead *SRQ");
		if (buf[0] == '1')
			return;
	}
}

void ConnGpibSerial::settmo (float _sec)
{
	timeout = _sec;
}

void ConnGpibSerial::readUSBGpib (char *reply, int blen)
{
	int att;
	int rlen = 0;
	for (att = 0; att < 3; att++)
	{
		rlen = rts2core::ConnSerial::readPort (reply, blen);
		std::cout << "rlen " << rlen << std::endl;
		if (rlen > 0)
		{
			blen -= rlen;
			break;
		}
	}
	if (att >= 3)
		throw rts2core::Error ("cannot read from port in writeRead");
	// read until something is available
	std::cout << "rlen blen " << rlen << " " << blen << std::endl;
	sleep (1);
	while (blen > 0 && (rlen = rts2core::ConnSerial::readPortNoBlock (reply + rlen, blen)) != 0)
	{
		blen -= rlen;
		std::cout << "rlen blen " << rlen << " " << blen << std::endl;
		sleep (1);
	}
	std::cout << "rlen " << rlen << std::endl;
}
