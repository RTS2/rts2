/* 
 * Connection for GPIB-serial manufactured by Prologix.biz
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

#include "connection/conngpibprologix.h"

using namespace rts2core;

ConnGpibPrologix::ConnGpibPrologix (rts2core::Block *_master, const char *_device, int _pad):ConnGpib (), rts2core::ConnSerial (_device, _master, rts2core::BS115200, rts2core::C8, rts2core::NONE, 40)
{
	pad = _pad;
	eot = 1;
	timeout = 3;
}

ConnGpibPrologix::~ConnGpibPrologix (void)
{
}

void ConnGpibPrologix::initGpib ()
{
	if (rts2core::ConnSerial::init ())
		throw rts2core::Error ("cannot init Prologix serial connection");
	if (rts2core::ConnSerial::writeRead ("++eot_char 10\n++eot_enable 0\n++eos 3\n++eoi 1\n++ver\n", 51, buf, 100, '\n') < 0)
		throw rts2core::Error ("cannot read Prologix version");
}

void ConnGpibPrologix::gpibWriteBuffer (const char *cmd, int _len)
{
	std::stringstream os;
	os << "++addr " << pad << "\n++auto 0\n";
	if (rts2core::ConnSerial::writePort (os.str ().c_str (), os.str().length ()) < 0)
		throw rts2core::Error ("cannot write ++addr command to serial port");
	if (rts2core::ConnSerial::writePort (cmd, _len) < 0)
		throw rts2core::Error ("cannot write command to serial port");
	if (rts2core::ConnSerial::writePort ('\n') < 0)
		throw rts2core::Error ("cannot write end endline");
}

void ConnGpibPrologix::gpibRead (void *reply, int &blen)
{
	std::stringstream os;
	os << "++addr " << pad;
	if (!isnan (timeout))
		os << "\n++read_tmo_ms " << ((int) (timeout * 1000));
	os << "\n++read eoi\n";
	if (rts2core::ConnSerial::writePort (os.str ().c_str (), os.str ().length ()) < 0)
		throw rts2core::Error ("cannot write ++read command to serial port");
	readUSBGpib ((char *) reply, blen);
}

void ConnGpibPrologix::gpibWriteRead (const char *cmd, char *reply, int blen)
{
	std::stringstream os;
	os << "++addr " << pad << "\n++auto 0\n" << cmd << "\n";
	if (!isnan (timeout))
		os << "++read_tmo_ms " << ((int) (timeout * 1000)) << "\n";
	if (rts2core::ConnSerial::writePort (os.str ().c_str (), os.str ().length ()) < 0)
		throw rts2core::Error ("cannot write to port");
	if (rts2core::ConnSerial::writePort ("++read eoi\n", 11) < 0)
		throw rts2core::Error ("cannot send read command to port");
	
	readUSBGpib (reply, blen);
}

void ConnGpibPrologix::gpibWaitSRQ ()
{
	while (true)
	{
		if (rts2core::ConnSerial::writeRead ("++srq\n", 6, buf, 10, '\n') < 0)
			throw rts2core::Error ("cannot writeRead ++sqr");
		if (buf[0] == '1')
			return;
	}
}

void ConnGpibPrologix::devClear ()
{
	std::stringstream os;
	os << "++addr " << pad << "\n++clr\n";
	if (rts2core::ConnSerial::writePort (os.str ().c_str (), os.str ().length ()) < 0)
		throw rts2core::Error ("cannot clear device");
}

void ConnGpibPrologix::settmo (float _sec)
{
	timeout = _sec;
}

void ConnGpibPrologix::readUSBGpib (char *reply, int blen)
{
	int att;
	int rlen = 0;
	for (att = 0; att < 3; att++)
	{
		rlen = rts2core::ConnSerial::readPortNoBlock (reply, blen);
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
