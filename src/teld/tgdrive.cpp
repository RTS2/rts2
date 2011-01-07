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

TGDrive::TGDrive (const char *_devName, const char *prefix, Rts2Device *_master):rts2core::ConnSerial (_devName, _master, rts2core::BS19200, rts2core::C8, rts2core::NONE, 20)
{
	setLogAsHex (true);

	char pbuf[strlen (prefix) + 20];
	char *p = pbuf + strlen (prefix);

	strcpy (pbuf, prefix);

	strcpy (p, "TPOS");
	_master->createValue (dPos, pbuf, "target position", true, RTS2_VALUE_WRITABLE);
	strcpy (p, "APOS");
	_master->createValue (aPos, pbuf, "actual position", true);
	strcpy (p, "POSERR");
	_master->createValue (posErr, pbuf, "position error", true);
	strcpy (p, "MAXPOSERR");
	_master->createValue (maxPosErr, pbuf, "maximal position error", false, RTS2_VALUE_WRITABLE);
	strcpy (p, "DSPEED");
	_master->createValue (dSpeed, pbuf, "[r/s] desired speed", false, RTS2_VALUE_WRITABLE);
	strcpy (p, "ASPEED");
	_master->createValue (aSpeed, pbuf, "[r/s] actual speed", false);
	strcpy (p, "MAXSPEED");
	_master->createValue (maxSpeed, pbuf, "[r/s] maximal profile generator speed", false, RTS2_VALUE_WRITABLE);
	strcpy (p, "DCURRENT");
	_master->createValue (dCur, pbuf, "[A] desired current", false, RTS2_VALUE_WRITABLE);
	strcpy (p, "ACURRENT");
	_master->createValue (aCur, pbuf, "[A] actual current", false);
	strcpy (p, "STATUS");
	_master->createValue (appStatus, pbuf, "axis status", true, RTS2_DT_HEX);
	strcpy (p, "FAULT");
	_master->createValue (faults, pbuf, "axis faults", true, RTS2_DT_HEX);
	strcpy (p, "MASTERCMD");
	_master->createValue (masterCmd, pbuf, "mastercmd register", false);
	strcpy (p, "FIRMWARE");
	_master->createValue (firmware, pbuf, "firmware version", false);
}

int TGDrive::init ()
{
	int ret = ConnSerial::init ();
	if (ret)
		return ret;	
//	drive->write2b (TGA_MASTER_CMD, 6);
	write2b (TGA_MASTER_CMD, 2);
	write2b (TGA_AFTER_RESET, TGA_AFTER_RESET_ENABLED);
	write2b (TGA_MASTER_CMD, 5);
	write4b (TGA_MODE, 0x4004);

//	write4b (TGA_ACCEL, 4947850);
//	write4b (TGA_DECEL, 4947850);
//	write4b (TGA_VMAX, 458993459);

//	write2b (TGA_DESCUR, 500);
	write2b (TGA_MASTER_CMD, 4);
	firmware->setValueInteger (read2b (TGA_FIRMWARE));
	return 0;
}

void TGDrive::info ()
{
	dPos->setValueInteger (read4b (TGA_TARPOS));
	aPos->setValueInteger (read4b (TGA_CURRPOS));
	posErr->setValueInteger (read4b (TGA_POSERR));
	maxPosErr->setValueInteger (read4b (TGA_MAXPOSERR));
	dSpeed->setValueDouble (read4b (TGA_DSPEED) / TGA_SPEEDFACTOR);
	aSpeed->setValueDouble (read4b (TGA_ASPEED) / TGA_SPEEDFACTOR);
	maxSpeed->setValueDouble (read4b (TGA_VMAX) / TGA_SPEEDFACTOR);
	dCur->setValueFloat (read2b (TGA_DESCUR) / TGA_CURRENTFACTOR);
	aCur->setValueFloat (read2b (TGA_ACTCUR) / TGA_CURRENTFACTOR);
	appStatus->setValueInteger (read2b (TGA_STATUS));
	faults->setValueInteger (read2b (TGA_FAULTS));
	masterCmd->setValueInteger (read2b (TGA_MASTER_CMD));
}

int TGDrive::setValue (Rts2Value *old_value, Rts2Value *new_value)
{
	try
	{
		if (old_value == dPos)
		{
			write4b (TGA_TARPOS, new_value->getValueInteger ());
		}
		else if (old_value == maxPosErr)
		{
			write4b (TGA_MAXPOSERR, new_value->getValueInteger ());
		}
		else if (old_value == dCur)
		{
			write2b (TGA_DESCUR, new_value->getValueFloat () * TGA_CURRENTFACTOR);
		}
		else if (old_value == dSpeed)
		{
			write4b (TGA_DSPEED, new_value->getValueDouble () * TGA_SPEEDFACTOR);
		}
		else if (old_value == maxSpeed)
		{
			write4b (TGA_VMAX, new_value->getValueDouble () * TGA_SPEEDFACTOR);
		}
		else
		{
			// otherwise pass to next possible value
			return 1;
		}
	}
	catch (TGDriveError e)
	{
		return -2;
	}
	return 0;
}

int16_t TGDrive::read2b (int16_t address)
{
	writeMsg (0xD1, address);
	char msg[10];
	ecRead (msg, 5);
	return * (( int16_t *) (msg + 2));
}

void TGDrive::write2b (int16_t address, int16_t data)
{
	writeMsg (0x02, address, (char *) &data, 2);
	readStatus ();
}

int32_t TGDrive::read4b (int16_t address)
{
	writeMsg (0xD2, address);
	char msg[14];
	ecRead (msg, 7);
	return * (( int32_t *) (msg + 2));

}

void TGDrive::write4b (int16_t address, int32_t data)
{
	writeMsg (0x02, address, (char *) &data, 4);
	readStatus ();
}

void TGDrive::ecWrite (char *msg)
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

	int ret = writePort (ec_buf, len);
	if (ret)
		throw TGDriveError (1);
}

void TGDrive::ecRead (char *msg, int len)
{
	// read header
	int ret;
	ret = readPort (msg, len);
	if (ret < 0)
		throw rts2core::Error ("Cannot read from device");
	// check checksum..
	unsigned char cs = 0;
	// get rid of all escapes..
	int escaped = 0;
	int checked_end = 1;
	int clen = len;
	do
	{
		if (escaped > 0)
		{
			ret = readPort (msg + clen, escaped);
			if (ret < 0)
				throw rts2core::Error ("cannot read rest from port");
			if (escaped > 1 && msg[clen + escaped - 1] == MSG_START && msg[clen + escaped - 2] != MSG_START)
			{
				ret = readPort (msg + clen + escaped, 1);
				if (ret < 0)
					throw rts2core::Error ("cannot read second escape");
			}
			clen += escaped;
			escaped = 0;
		}
		for (; checked_end < clen; checked_end++)
		{
			if (msg[checked_end] == MSG_START)
			{
				if (checked_end == clen - 1)
				{
					escaped++;
					break;
				}
				checked_end++;
				if (msg[checked_end] == MSG_START)
				{
					memmove (msg + checked_end - 1, msg + checked_end, clen - checked_end);
					clen--;
					checked_end--;
					// only increase characters to receive if this is not the last character
					if (checked_end < len - 1)
						escaped++;
				}
				else
				{
					logStream (MESSAGE_ERROR) << "unescaped MSG_START" << sendLog;
				}
			}	
		}
	} while (escaped > 0);

	for (int i = 1; i < len - 1; i++)
		cs += msg[i];

	cs = 0x100 - cs;

	if ((0x00ff & msg[len - 1]) != cs)
	{
	  	Rts2LogStream ls = logStream (MESSAGE_ERROR);
		ls << "invalid checksum, expected " << std::hex << (int) cs << " received " << std::hex << (0x00ff & ((int) msg[len - 1])) << ":";
		for (int j = 0; j < len; j++)
			ls << std::hex << (int) msg[j] << " ";
		ls << sendLog;
		throw TGDriveError (2);
	}
	if (msg[1] != STAT_OK)
	{
		logStream (MESSAGE_ERROR) << "status is not OK, it is " << std::hex << (int) msg[1] << sendLog;
		throw TGDriveError (msg[1]);
	}
}

void TGDrive::writeMsg (char op, int16_t address)
{
	char msg[7];

	msg[0] = MSG_START;
	msg[1] = op;
	*((int16_t *) (msg + 2)) = address;
	char cs = 0x100 - (op + msg[2] + msg[3]);
	int len = 2;
	for (int i = 2; i < 4; i++, len++)
	{
		if (msg[len] == MSG_START)
		{
			if (i == 2)
				msg[len + 2] = msg[len];
			len++;
			msg[len] = MSG_START;
		}
	}
	msg[len] = cs;
	len++;

	int ret = writePort (msg, len);
	if (ret)
		throw TGDriveError (1);
}

void TGDrive::writeMsg (char op, int16_t address, char *data, int len)
{
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
	ecWrite (msg);
}

void TGDrive::readStatus ()
{
	char msg[6];
	ecRead (msg, 3);
}
