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

#include "connection/tgdrive.h"
#include <iomanip>

// defines for protocol
#define MSG_START       0x2B

#define STAT_OK         0x00
#define ERR_UNKNOWN     0x81
#define ERR_CHKSUM      0x82
#define ERR_TOOLONG     0x83

using namespace rts2teld;

TGDrive::TGDrive (const char *_devName, const char *prefix, rts2core::Device *_master):rts2core::ConnSerial (_devName, _master, rts2core::BS19200, rts2core::C8, rts2core::NONE, 60)
{
	setLogAsHex (true);

	char pbuf[strlen (prefix) + 20];
	char *p = pbuf + strlen (prefix);

	strcpy (pbuf, prefix);

	strcpy (p, "TPOS");
	_master->createValue (dPos, pbuf, "target position", true, RTS2_VALUE_WRITABLE);
	strcpy (p, "APOS");
	_master->createValue (aPos, pbuf, "actual position", true, RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);
	strcpy (p, "POSERR");
	_master->createValue (posErr, pbuf, "position error", true);
	strcpy (p, "MAXPOSERR");
	_master->createValue (maxPosErr, pbuf, "maximal position error", false, RTS2_VALUE_WRITABLE);
	strcpy (p, "DSPEED");
	_master->createValue (dSpeed, pbuf, "[r/s] desired speed", false, RTS2_VALUE_WRITABLE);
	strcpy (p, "DSPEED_INT");
	_master->createValue (dSpeedInt, pbuf, "desired speed in internal units", false, RTS2_VALUE_WRITABLE);
	strcpy (p, "ASPEED");
	_master->createValue (aSpeed, pbuf, "[r/s] actual speed", false);
	strcpy (p, "ASPEED_INT");
	_master->createValue (aSpeedInt, pbuf, "[r/s] actual speed", false);
	strcpy (p, "MAXSPEED");
	_master->createValue (maxSpeed, pbuf, "[r/s] maximal profile generator speed", false, RTS2_VALUE_WRITABLE);
	strcpy (p, "ACCEL");
	_master->createValue (accel, pbuf, "[r/s**2] acceleration", false, RTS2_VALUE_WRITABLE);
	strcpy (p, "DECEL");
	_master->createValue (decel, pbuf, "[r/s**2] decceleration", false, RTS2_VALUE_WRITABLE);
	strcpy (p, "EMERDECEL");
	_master->createValue (emerDecel, pbuf, "[r/s**2] emergency deceleration", false, RTS2_VALUE_WRITABLE);
	strcpy (p, "DCURRENT");
	_master->createValue (dCur, pbuf, "[A] desired current Iq", false, RTS2_VALUE_WRITABLE);
	strcpy (p, "ACURRENT");
	_master->createValue (aCur, pbuf, "[A] actual current Iq", false);
	strcpy (p, "POSITION_KP");
	_master->createValue (positionKp, pbuf, "proporcional gain for the position controller, in cascade above speed controller", false, RTS2_VALUE_WRITABLE);
	strcpy (p, "SPEED_KP");
	_master->createValue (speedKp, pbuf, "proporcional gain for the speed controller, in cascade above q-current controller", false, RTS2_VALUE_WRITABLE);
	strcpy (p, "SPEED_KI");
	_master->createValue (speedKi, pbuf, "integration gain for the speed controller, in cascade above q-current controller", false, RTS2_VALUE_WRITABLE);
	strcpy (p, "SPEED_IMAX");
	_master->createValue (speedImax, pbuf, "peak current for the speed controller, in cascade above q-current controller", false, RTS2_VALUE_WRITABLE);
	strcpy (p, "Q_CURRENT_KP");
	_master->createValue (qCurrentKp, pbuf, "proporcional gain for the current q-component (torque) controller", false, RTS2_VALUE_WRITABLE);
	strcpy (p, "Q_CURRENT_KI");
	_master->createValue (qCurrentKi, pbuf, "integration gain for the current q-component (torque) controller", false, RTS2_VALUE_WRITABLE);
	strcpy (p, "D_CURRENT_KP");
	_master->createValue (dCurrentKp, pbuf, "proporcional gain for the current d-component (flux) controller", false, RTS2_VALUE_WRITABLE);
	strcpy (p, "D_CURRENT_KI");
	_master->createValue (dCurrentKi, pbuf, "integration gain for the current d-component (flux) controller", false, RTS2_VALUE_WRITABLE);
	strcpy (p, "MODE");
	_master->createValue (tgaMode, pbuf, "axis mode", false, RTS2_DT_HEX);
	strcpy (p, "STATUS");
	_master->createValue (appStatus, pbuf, "axis status", true, RTS2_DT_HEX);
	strcpy (p, "FAULT");
	_master->createValue (faults, pbuf, "axis faults", true, RTS2_DT_HEX);
	strcpy (p, "MASTERCMD");
	_master->createValue (masterCmd, pbuf, "mastercmd register", false);
	strcpy (p, "FIRMWARE");
	_master->createValue (firmware, pbuf, "firmware version", false);

	stopped = false;
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
	setMode (TGA_MODE_PA);

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
	dSpeedInt->setValueInteger (read4b (TGA_DSPEED));
	dSpeed->setValueDouble (dSpeedInt->getValueInteger () / TGA_SPEEDFACTOR);
	aSpeedInt->setValueInteger (read4b (TGA_ASPEED));
	aSpeed->setValueDouble (aSpeedInt->getValueInteger () / TGA_SPEEDFACTOR);
	maxSpeed->setValueDouble (read4b (TGA_VMAX) / TGA_SPEEDFACTOR);
	accel->setValueDouble (read4b (TGA_ACCEL) / TGA_ACCELFACTOR);
	decel->setValueDouble (read4b (TGA_DECEL) / TGA_ACCELFACTOR);
	emerDecel->setValueDouble (read4b (TGA_EMERDECEL) / TGA_ACCELFACTOR);
	dCur->setValueFloat (read2b (TGA_DESCUR) / TGA_CURRENTFACTOR);
	aCur->setValueFloat (read2b (TGA_ACTCUR) / TGA_CURRENTFACTOR);
	positionKp->setValueFloat (read2b (TGA_POSITIONREG_KP) / TGA_GAINFACTOR);
	speedKp->setValueFloat (read2b (TGA_SPEEDREG_KP) / TGA_GAINFACTOR);
	speedKi->setValueFloat (read2b (TGA_SPEEDREG_KI) / TGA_GAINFACTOR);
	speedImax->setValueFloat (read2b (TGA_SPEEDREG_IMAX) / TGA_CURRENTFACTOR);
	qCurrentKp->setValueFloat (read2b (TGA_Q_CURRENTREG_KP) / TGA_GAINFACTOR);
	qCurrentKi->setValueFloat (read2b (TGA_Q_CURRENTREG_KI) / TGA_GAINFACTOR);
	dCurrentKp->setValueFloat (read2b (TGA_D_CURRENTREG_KP) / TGA_GAINFACTOR);
	dCurrentKi->setValueFloat (read2b (TGA_D_CURRENTREG_KI) / TGA_GAINFACTOR);
	tgaMode->setValueInteger (read4b (TGA_MODE));
	appStatus->setValueInteger (read2b (TGA_STATUS));
	faults->setValueInteger (read2b (TGA_FAULTS));
	masterCmd->setValueInteger (read2b (TGA_MASTER_CMD));
}

int TGDrive::setValue (rts2core::Value *old_value, rts2core::Value *new_value)
{
	try
	{
		if (old_value == dPos)
		{
			setTargetPos (new_value->getValueInteger ());
		}
		else if (old_value == aPos)
		{
			setCurrentPos (new_value->getValueInteger ());
		}
		else if (old_value == maxPosErr)
		{
			write4b (TGA_MAXPOSERR, new_value->getValueInteger ());
		}
		else if (old_value == accel)
		{
			write4b (TGA_ACCEL, new_value->getValueDouble () * TGA_ACCELFACTOR);
		}
		else if (old_value == decel)
		{
			write4b (TGA_DECEL, new_value->getValueDouble () * TGA_ACCELFACTOR);
		}
		else if (old_value == emerDecel)
		{
			write4b (TGA_EMERDECEL, new_value->getValueDouble () * TGA_ACCELFACTOR);
		}
		else if (old_value == dCur)
		{
			write2b (TGA_DESCUR, new_value->getValueFloat () * TGA_CURRENTFACTOR);
		}
		else if (old_value == dSpeed)
		{
			setTargetSpeed (new_value->getValueDouble () * TGA_SPEEDFACTOR);
		}
		else if (old_value == dSpeedInt)
		{
			setTargetSpeed (new_value->getValueInteger ());
		}
		else if (old_value == maxSpeed)
		{
			setMaxSpeed (new_value->getValueDouble ());
		}
		else if (old_value == positionKp)
		{
			write2b (TGA_POSITIONREG_KP, new_value->getValueFloat () * TGA_GAINFACTOR);
		}
		else if (old_value == speedKp)
		{
			write2b (TGA_SPEEDREG_KP, new_value->getValueFloat () * TGA_GAINFACTOR);
		}
		else if (old_value == speedKi)
		{
			write2b (TGA_SPEEDREG_KI, new_value->getValueFloat () * TGA_GAINFACTOR);
		}
		else if (old_value == speedImax)
		{
			write2b (TGA_SPEEDREG_IMAX, new_value->getValueFloat () * TGA_CURRENTFACTOR);
		}
		else if (old_value == qCurrentKp)
		{
			write2b (TGA_Q_CURRENTREG_KP, new_value->getValueFloat () * TGA_GAINFACTOR);
		}
		else if (old_value == qCurrentKi)
		{
			write2b (TGA_Q_CURRENTREG_KI, new_value->getValueFloat () * TGA_GAINFACTOR);
		}
		else if (old_value == dCurrentKp)
		{
			write2b (TGA_D_CURRENTREG_KP, new_value->getValueFloat () * TGA_GAINFACTOR);
		}
		else if (old_value == dCurrentKi)
		{
			write2b (TGA_D_CURRENTREG_KI, new_value->getValueFloat () * TGA_GAINFACTOR);
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

void TGDrive::setMode (int32_t mode)
{
	write4b (TGA_MODE, mode);
}

void TGDrive::setTargetPos (int32_t pos)
{
	setMode (TGA_MODE_PA);
	stopped = false;
	write4b (TGA_TARPOS, pos);
}

void TGDrive::setCurrentPos (int32_t pos)
{
	write2b (TGA_MASTER_CMD, 1);

	write4b (TGA_CURRPOS, pos);
	usleep (USEC_SEC / 10);

	reset ();
}

void TGDrive::setTargetSpeed (int32_t dspeed, bool changeMode)
{
	if (changeMode || (read4b (TGA_STATUS) & 0x02))
	{
		setMode (TGA_MODE_DS);
	}
	stopped = (dspeed == 0);
	write4b (TGA_DSPEED, dspeed);
}

void TGDrive::setMaxSpeed (double speed)
{
	write4b (TGA_VMAX, speed * TGA_SPEEDFACTOR);
}

void TGDrive::stop ()
{
	// other way to stop..with backslahs
	// setTargetPos (getPosition ());

	// other possibility is to switch to speed mode..
	write4b (TGA_DSPEED, 0);
	stoppedPosition = read4b (TGA_CURRPOS);
	setMode (TGA_MODE_DS);
	stopped = true;
}

bool TGDrive::checkStop ()
{
	if (stopped == false)
		return false;
	appStatus->setValueInteger (read2b (TGA_STATUS));
	if ((appStatus->getValueInteger () & 0x08) == 0x08)
	{
		setMode (TGA_MODE_PA);
		write4b (TGA_TARPOS, stoppedPosition);
		stopped = false;
		return false;
	}
	return true;
}

void TGDrive::reset ()
{
	write2b (TGA_MASTER_CMD, 5);
	write2b (TGA_MASTER_CMD, 2);
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
	char ec_buf [5 + msg[2] * 2];

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
	cs = 0x100 - cs;
	ec_buf[len] = cs;
	len++;
	if (cs == MSG_START)
	{
		ec_buf[len] = MSG_START;
		len++;
	}

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
	  	rts2core::LogStream ls = logStream (MESSAGE_ERROR);
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
	for (int i = 0; i < 2; i++, len++)
	{
		if (msg[len] == MSG_START)
		{
			if (i == 0)
				msg[4] = msg[3];
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
