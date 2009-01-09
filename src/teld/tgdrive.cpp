/* 
 * RS-232 TGDrives interface.
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

#include "tgdrive.h"
#include <iomanip>

// defines for protocol
#define MSG_START       0x2B

#define STAT_OK         0x00
#define ERR_UNKNOWN     0x81
#define ERR_CHKSUM      0x82
#define ERR_TOOLONG     0x83

using namespace rts2teld;

void
TGDrive::ecWrite (char *msg)
{
	char ec_buf [4 + msg[2] * 2];

  	int i;
	for (i = 0; i < 3; i++)
		ec_buf[i] = msg[i];

	int len = 3;
	unsigned char cs = msg[1] + msg[2];

	// escape
	for (; i < 3 + msg[2]; i++, len++)
	{
		ec_buf[len] = msg[i];
		cs += msg[i];
		if (ec_buf[len] == MSG_START)
		{
			len++;
			ec_buf[len] = MSG_START;
		}
	}

	// checksum
	ec_buf[len] = 0x100 - cs;
	len++;

	Rts2LogStream ls = logStream (MESSAGE_DEBUG);
	ls << "Will write ";
	ls.fill ('0');
	for (i = 0; i < len; i++)
	{
		int b = ec_buf[i];
		ls << "0x" << std::hex << std::setw (2) << (0x000000ff & b) << " ";
	}
	ls << sendLog;

	int ret = writePort (ec_buf, len);
	if (ret)
		throw TGDriveError (1);
}


void
TGDrive::ecRead (char *msg, int len)
{
	// read header
	int ret;
	ret = readPort (msg, len);
	if (ret != len)
	{
		logStream (MESSAGE_ERROR) << "msg[1] " << std::hex << (int) msg[0] << std::hex << (int) msg[1] << sendLog;
		throw TGDriveError (1);
	}
	// check checksum..
	unsigned char cs = 0;
	for (int i = 1; i < len - 1; i++)
		cs += msg[i];

	cs = 0x100 - cs;

	if (msg[len - 1] != cs)
	{
	  	logStream (MESSAGE_ERROR) << "invalid checksum, expected " << std::hex << (int) cs << " received " << std::hex << (int) msg[len - 1] << "." << sendLog;
		// throw TGDriveError (2);
	}
	if (msg[1] != STAT_OK)
	{
		logStream (MESSAGE_ERROR) << "status is not OK, it is " << std::hex << (int) msg[1] << sendLog;
//		throw TGDriveError (msg[1]);
	}
}

void
TGDrive::writeMsg (char op, int16_t address)
{
	char msg[6];

	msg[0] = MSG_START;
	msg[1] = op;
	msg[2] = 2;
	*((int16_t *) (msg + 3)) = address;

	ecWrite (msg);
}


void
TGDrive::writeMsg (char op, int16_t address, char *data, int len)
{
	logStream (MESSAGE_ERROR) << "writeMsg " << len << sendLog;
	char msg[6 + len];
	msg[0] = MSG_START;
	msg[1] = op;
	msg[2] = 3 + len;
	msg[3] = len;
	*((int16_t *) (msg + 4)) = address;
	for (int i = 0; i < len; i++)
	{
		msg[i + 6] = data[i];
	}

	logStream (MESSAGE_ERROR) << "ecWrite " << sendLog;
	ecWrite (msg);
}


void
TGDrive::readStatus ()
{
	char msg[3];
	ecRead (msg, 3);
}


TGDrive::TGDrive (const char *_devName, Rts2Block *_master)
:Rts2ConnSerial (_devName, _master, BS9600, C8, NONE, 20)
{

}


int16_t
TGDrive::read2b (int16_t address)
{
	writeMsg (0xD1, address);
	char msg[5];
	ecRead (msg, 5);
	return * (( int16_t *) (msg + 2));
}


void
TGDrive::write2b (int16_t address, int16_t data)
{
	writeMsg (0x02, address, (char *) &data, 2);
	readStatus ();
}


int32_t
TGDrive::read4b (int16_t address)
{
	writeMsg (0xD4, address);
	char msg[7];
	ecRead (msg, 7);
	return * (( int32_t *) (msg + 2));

}


void
TGDrive::write4b (int16_t address, int32_t data)
{
	writeMsg (0x02, address, (char *) &data, 4);
	readStatus ();
}
