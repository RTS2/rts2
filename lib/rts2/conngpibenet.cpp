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

#include "connection/conngpibenet.h"

using namespace rts2core;

/**
 * Internal error, thrown when flags is not equal 0. This can signal error or end of data
 * in read command.
 */
class ErrorGpibEnetFlags:public rts2core::Error
{
	public:
		ErrorGpibEnetFlags (int _flags)
		{
			std::ostringstream _os;
			_os << "Flags are equal to " << _flags;
			setMsg (_os.str());
		}
};

void ConnGpibEnet::sread (char **ret_buf)
{
	char gpib_buf[4];
	receiveData (gpib_buf, 4, 30, true);

	flags = ntohs (*((uint16_t *) (gpib_buf)));
	len = ntohs (*((uint16_t *) (gpib_buf + 2)));

	if (flags != 0x0000)
	{
		*ret_buf = NULL;
		throw ErrorGpibEnetFlags (flags);
	}

	if (len == 0)
	{
		*ret_buf = NULL;
		return;
	}

	*ret_buf = new char[len];
	receiveData (*ret_buf, len, 30, true);
}

void ConnGpibEnet::sresp (char **ret_buf)
{
	char *sread_ret;
	sread (&sread_ret);

	// parse top of reply..
	sta = ntohs (*((uint16_t *) (sread_ret)));
	err = ntohs (*((uint16_t *) (sread_ret + 2)));
	cnt = ntohl (*((uint32_t *) (sread_ret + 8)));

	if (ret_buf != NULL)
		*ret_buf = sread_ret;
	else
		delete[] sread_ret;
}

void ConnGpibEnet::gpibWriteBuffer (const char *cmd, int _len)
{
	// write header
	char gpib_buf[13] = "\x23\x05\x05\x08IIII\x00\x54\x00\x00";
	*((int32_t *) (gpib_buf + 4)) = htonl (_len);
	sendData (gpib_buf, 12, true);
	sresp (NULL);

	sendData ((void *) cmd, _len);
	sresp (NULL);
}

void ConnGpibEnet::gpibRead (void *reply, int &blen)
{
	uint16_t data_len = 0;
	char *sbuf;

	char gpib_buf[13] = "\x16\x00\x00\x00IIII\x40\x63\x16\x40";
	*((int32_t *) (gpib_buf + 4)) = htonl (blen);
	sendData (gpib_buf, 12, true);
	sresp (&sbuf);

	try
	{
		// loop till flags == 0;
		while (true)
		{
			// raises error when flags != 0
			sread (&sbuf);
			if (data_len + len >= blen - 1)
			{
				delete[] sbuf;
				throw rts2core::Error ("too short buffer");
			}
			memcpy (((char *)reply) + data_len, sbuf, len);
			data_len += len;
			delete[] sbuf;
		}
	}
	catch (ErrorGpibEnetFlags &er)
	{
		if (data_len > 0)
		{
			blen = data_len;
			((char*)reply)[blen] = '\0';
			sresp (NULL);
			return;
		}
		throw rts2core::Error ("Cannot read data");
	}
}

void ConnGpibEnet::gpibWriteRead (const char *cmd, char *reply, int blen)
{
	gpibWrite (cmd);
	*reply = '\0';
	gpibRead (reply, blen);
}

void ConnGpibEnet::gpibWaitSRQ ()
{
	while (true)
	{
		char *sbuf;
		// iblines..
		sendData ("\x0d\x63\x16\x40\xc0\x58\x16\x40\x40\x63\x16\x40", 12);
		sresp (&sbuf);
		if (sta & (1 << 15))
		{
			delete[] sbuf;
			throw rts2core::Error ("Error while waiting for SRQ");
		}
		if (len != 14)
		{
			delete[] sbuf;
			throw rts2core::Error ("Too short reply from iblines call");
		}
		if (ntohs (*((uint16_t *) (sbuf + 12))) & (1 << 12))
			return;
		usleep (USEC_SEC / 100);
	}
}

void ConnGpibEnet::initGpib ()
{
	rts2core::ConnTCP::init ();

	char gpib_buf[12];
	// command
	gpib_buf[0] = 7;
	// bna - board number?
	gpib_buf[1] = 1;
	// flags
	gpib_buf[2] = 0;
	// eot
	gpib_buf[3] = 0x40 | eot;

	if (sad != 0)
		pad |= 0x80;

	gpib_buf[4] = (char) pad;

	gpib_buf[5] = sad;
	gpib_buf[6] = eos;
	gpib_buf[7] = 0x00;
	gpib_buf[8] = 13;
	gpib_buf[9] = 0x02;
	gpib_buf[10] = 0x04;
	gpib_buf[11] = 0x00;

	sendData (gpib_buf, 12, true);

	timeout = 10;

	char *ret_buf;

	sresp (&ret_buf);

	delete[] ret_buf;
}

void ConnGpibEnet::devClear ()
{
	char gpib_buf[13] = "\x04\xf5\xff\xbf\x14\xf5\xff\xbf\xa9\x8f\x04\x08";
	sendData (gpib_buf, 12, true);
	sresp (NULL);
}

void ConnGpibEnet::settmo (float _sec)
{
	timeout = _sec;
	char gpib_buf[13] = "\x1fI\x00\x00\x20\xe1\x05\x08\xae\xe0\x05\x08";
	gpib_buf[1] = getTimeoutTmo (timeout);
	sendData (gpib_buf, 12, true);
	sresp (NULL);
}

ConnGpibEnet::ConnGpibEnet (rts2core::Block *_master, const char *_address, int _port, int _pad):ConnGpib (), rts2core::ConnTCP (_master, _address, _port)
{
	sad = 0;
	pad = _pad;
	eot = 1;
	eos = 0;
	timeout = NAN;
}

ConnGpibEnet::~ConnGpibEnet (void)
{
}
