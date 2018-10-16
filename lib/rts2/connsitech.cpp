/*
 * Sitech telescope protocol handling.
 * Copyright (C) 2014 Petr Kubanek <petr@kubanek.net>
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

#include "constsitech.h"
#include "connection/sitech.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifdef RTS2_HAVE_ENDIAN_H
#include <endian.h>
#endif

using namespace rts2core;

ConnSitech::ConnSitech (const char *devName, Block *_master):ConnSerial (devName, _master, BS19200, C8, NONE, 50, 5)
{
	binary = false;
	version = -1;
	PIDSampleRate = 1;

	logFile = -1;
	logCount = 0;

	memset (&last_status, 0, sizeof (last_status));

	memset (&flashBuffer, 0, sizeof (flashBuffer));
}

int ConnSitech::init ()
{
	int ret = ConnSerial::init ();
	if (ret)
		return ret;

	// check if checksum mode is on; that needs to be send always with checksum
	binary = true;
	binary = getSiTechValue ('Y', "XY") == 0;

	version = getSiTechValue ('X', "V");

	if (version != 0)
	{
		logStream (MESSAGE_DEBUG) << "Sidereal Technology Controller version " << version / 10.0 << sendLog;
	}
	else
	{
		logStream (MESSAGE_ERROR) << "A200HR drive control did not respond." << sendLog;
		return -1;
	}

	if (version < 112)
	{
		sitechType = SERVO_II;
	}
	else if (version < 180)
	{
		sitechType = FORCE_ONE;
		countUp = getSiTechValue ('Y', "XHC");
		PIDSampleRate = (CRYSTAL_FREQ / 12.0) / (SPEED_MULTI - countUp);
	}
	else
	{
		sitechType = FORCE_TWO;
		countUp = getSiTechValue ('Y', "XHC");
		PIDSampleRate = (CRYSTAL_FREQ / 12.0) / (SPEED_MULTI - countUp);
	}


	return 0;
}

void ConnSitech::switchToASCI ()
{
	if (binary == true)
	{
		int ret = writePort ("YXY0\r\xb8", 6);
		if (ret < 0)
			throw Error ("Cannot switch to ASCII mode");
	}
	binary = false;
}

void ConnSitech::switchToBinary ()
{
	if (binary == false)
	{
		int ret = writePort ("YXY1\r", 5);
		if (ret < 0)
			throw Error ("Cannot switch to ASCII mode");
	}
	binary = true;
}

std::string ConnSitech::findErrors (uint16_t e)
{
	std::string ret;
	if (e & ERROR_GATE_VOLTS_LOW)
		ret += "Gate Volts Low. ";
	if (e & ERROR_OVERCURRENT_HARDWARE)
		ret += "OverCurrent Hardware. ";
	if (e & ERROR_OVERCURRENT_FIRMWARE)
		ret += "OverCurrent Firmware. ";
	if (e & ERROR_MOTOR_VOLTS_LOW)
		ret += "Motor Volts Low. ";
	if (e & ERROR_POWER_BOARD_OVER_TEMP)
		ret += "Power Board Over Temp. ";
	if (e & ERROR_NEEDS_RESET)
		ret += "Needs Reset (may not have any other errors). ";
	if (e & ERROR_LIMIT_MINUS)
		ret += "Limit -. ";
	if (e & ERROR_LIMIT_PLUS)
		ret += "Limit +. ";
	if (e & ERROR_TIMED_OVER_CURRENT)
		ret += "Timed Over Current. ";
	if (e & ERROR_POSITION_ERROR)
		ret += "Position Error. ";
	if (e & ERROR_BISS_ENCODER_ERROR)
		ret += "BiSS Encoder Error. ";
	if (e & ERROR_CHECKSUM)
		ret += "Checksum Error in return from Power Board. ";
	return ret;
}

void ConnSitech::resetErrors ()
{
	siTechCommand ('Y', "XS");
}

void ConnSitech::resetController ()
{
	siTechCommand ('X', "Q");
}

void ConnSitech::siTechCommand (const char axis, const char *cmd)
{
	size_t len = strlen (cmd);
	char ccmd[len + 3];

	ccmd[0] = axis;
	memcpy (ccmd + 1, cmd, len);
	ccmd[len + 1] = '\r';
	ccmd[len + 2] = '\0';

	if (binary)
	{
		writePortChecksumed (ccmd, len + 2);
	}
	else
	{
		int ret = writePort (ccmd, len + 2);
		if (ret < 0)
			throw Error (std::string("cannot send command ") + cmd);
	}
}

int32_t ConnSitech::getSiTechValue (const char axis, const char *val)
{
	//switchToASCI ();
	siTechCommand (axis, val);

	char ret[100];

	size_t len = readPort (ret, 100, "\n");
	if (len <= 0)
		throw Error (std::string ("cannot read response get value command ") + val);

	return atol (ret + 1);
}

void ConnSitech::getAxisStatus (char axis)
{
	switchToBinary ();
	siTechCommand (axis, "XS");

	readAxisStatus ();
}

void ConnSitech::sendYAxisRequest (SitechYAxisRequest &ax_request)
{
	switchToBinary ();
	siTechCommand ('Y', "XR");

	char data[34];

	*((uint32_t *) (data)) = htole32 (ax_request.x_dest);
	*((uint32_t *) (data + 4)) = htole32 (ax_request.x_speed);
	*((uint32_t *) (data + 8)) = htole32 (ax_request.y_dest);
	*((uint32_t *) (data + 12)) = htole32 (ax_request.y_speed);

	*((uint32_t *) (data + 16)) = htole32 (ax_request.x_rate_adder);
	*((uint32_t *) (data + 20)) = htole32 (ax_request.y_rate_adder);

	*((uint32_t *) (data + 24)) = htole32 (ax_request.x_rate_adder_t);
	*((uint32_t *) (data + 28)) = htole32 (ax_request.y_rate_adder_t);

	*((uint16_t *) (data + 32)) = htole16 (binaryChecksum (data, 32, true));

	// sends the data..
	writePort (data, 34);

	if (logFile > 0)
		logBuffer ('Y', data, 34);

	// read back reply
	readAxisStatus ();
}

void ConnSitech::sendXAxisRequest (SitechXAxisRequest &ax_request)
{
	switchToBinary ();
	siTechCommand ('X', "XR");

	char data[21];

	*((uint32_t *) (data)) = htole32 (ax_request.x_dest);
	*((uint32_t *) (data + 4)) = htole32 (ax_request.x_speed);
	*((uint32_t *) (data + 8)) = htole32 (ax_request.y_dest);
	*((uint32_t *) (data + 12)) = htole32 (ax_request.y_speed);

	// we would like to set Xbits and Ybits..
	*((uint8_t *) (data + 16)) = 1;    // there isn't reason to use this anymore
	*((uint8_t *) (data + 17)) = ax_request.x_bits;
	*((uint8_t *) (data + 18)) = ax_request.y_bits;

	*((uint16_t *) (data + 19)) = htole16 (binaryChecksum (data, 19, true));

	writePort (data, 21);

	if (logFile > 0)
		logBuffer ('X', data, 21);

	readAxisStatus ();
}

void ConnSitech::setSiTechValue (const char axis, const char *val, int value)
{
	char *ccmd = NULL;
	asprintf (&ccmd, "%s%d", val, value);

	siTechCommand (axis, ccmd);
}

void ConnSitech::setSiTechValueLong (const char axis, const char *val, long value)
{
	char *ccmd = NULL;
	asprintf (&ccmd, "%s%ld", val, value);

	siTechCommand (axis, ccmd);
}

void ConnSitech::setPosition (const char axis, uint32_t target, uint32_t speed)
{
	char *ccmd = NULL;
	asprintf (&ccmd, "%dS%d", target, speed);

	siTechCommand (axis, ccmd);
}

void ConnSitech::getControllerStatus (__attribute__ ((unused)) SitechControllerStatus &controller_status)
{
}

void ConnSitech::readAxisStatus ()
{
	char ret[42];

	size_t len = readPort (ret, 41);
	if (len != 41)
	{
		flushPortIO ();
		throw Error ("cannot read all 41 bits from status response!");
	}

	// checksum checks
	uint16_t checksum = binaryChecksum (ret, 39, true);

	if ((*((uint16_t *) (ret + 39))) != checksum)
	{
		std::ostringstream oss;
		oss << "invalid checksum in readAxisStatus, expected " <<  (*((uint16_t *) (ret + 39))) << ", calculated " << checksum;
		throw Error (oss.str());
	}

	// fill in proper return values..
	last_status.address = ret[0];
	last_status.x_pos = le32toh (*((uint32_t *) (ret + 1)));
	last_status.y_pos = le32toh (*((uint32_t *) (ret + 5)));
	last_status.x_enc = le32toh (*((uint32_t *) (ret + 9)));
	last_status.y_enc = le32toh (*((uint32_t *) (ret + 13)));

	last_status.keypad = ret[17];
	last_status.x_bit = ret[18];
	last_status.y_bit = ret[19];
	last_status.extra_bits = ret[20];
	last_status.ain_1 = le16toh (*((uint16_t *) (ret + 21)));
	last_status.ain_2 = le16toh (*((uint16_t *) (ret + 23)));
	last_status.mclock = le32toh (*((uint32_t *) (ret + 25)));
	last_status.temperature = ret[29];
	last_status.y_worm_phase = ret[30];
	memcpy (last_status.x_last, ret + 31, 4);
	memcpy (last_status.y_last, ret + 35, 4);

	if (logFile > 0)
		logBuffer ('A', ret, 41);
}

void ConnSitech::writePortChecksumed (const char *cmd, size_t len)
{
	int ret = writePort (cmd, len);
	if (ret < 0)
		throw Error (std::string ("cannot write ") + cmd + " :" + strerror (errno));

	ret = writePort ((char) calculateChecksum (cmd, len));
	if (ret < 0)
		throw Error (std::string ("cannot write checksum of ") + cmd + " :" + strerror (errno));
}

uint8_t ConnSitech::calculateChecksum (const char *cbuf, size_t len)
{
	uint8_t ret = 0;
	for (size_t i = 0; i < len; i++)
		ret += cbuf[i];

	return ~ret;
}

uint16_t ConnSitech::binaryChecksum (const char *dbuf, size_t blen, bool invertH)
{
	uint16_t checksum = 0;

	for (size_t i = 0; i < blen; i++)
		checksum += (uint8_t) dbuf[i];

	return invertH ? (checksum ^ 0xFF00) : checksum;
}

double ConnSitech::degsPerSec2MotorSpeed (double dps, int32_t loop_ticks, double full_circle)
{
	switch (sitechType)
	{
		case SERVO_I:
		case SERVO_II:
			return ((loop_ticks / full_circle) * dps) / 1953;
		case FORCE_ONE:
		case FORCE_TWO:
			return ((loop_ticks / full_circle) * dps) / PIDSampleRate;
		default:
			return 0;
	}
}

double ConnSitech::ticksPerSec2MotorSpeed (double tps)
{
	switch (sitechType)
	{
		case SERVO_I:
		case SERVO_II:
			return tps * SPEED_MULTI / 1953;
		case FORCE_ONE:
		case FORCE_TWO:
			return tps * SPEED_MULTI / PIDSampleRate;
		default:
			return 0;
	}
}

double ConnSitech::motorSpeed2DegsPerSec (int32_t speed, int32_t loop_ticks)
{
	switch (sitechType)
	{
		case SERVO_I:
		case SERVO_II:
			return (double) speed / loop_ticks * (360.0 * 1953 / SPEED_MULTI);
		case FORCE_ONE:
		case FORCE_TWO:
			return (double) speed / loop_ticks * (360.0 * PIDSampleRate / SPEED_MULTI);
		default:
			return 0;
	}
}

void ConnSitech::startLogging (const char *logFileName)
{
	endLogging ();

	mkpath (logFileName, 0777);
	logFile = open (logFileName, O_APPEND | O_WRONLY | O_CREAT);
	if (logFile < 0)
	{
		logStream (MESSAGE_WARNING) << "cannot start SiTech logging to " << logFileName << " " << strerror (errno) << sendLog;
		return;
	}
	fchmod (logFile, 0644);
	logStream (MESSAGE_DEBUG) << "logging to " << logFileName << sendLog;
}

void ConnSitech::endLogging ()
{
	if (logFile > 0)
	{
		fsync (logFile);
		close (logFile);
	}
	logFile = -1;
}

int ConnSitech::flashLoad ()
{
	switchToBinary ();
	siTechCommand ('S', "C");

	size_t l = 0;

	switch (sitechType)
	{
		case SERVO_I:
		case SERVO_II:
			l = 130;
			break;
		case FORCE_ONE:
		case FORCE_TWO:
			l = 514;
			break;
	}
	size_t ret = readPort ((char *) flashBuffer, l);
	if (ret != l)
		return -1;

	uint16_t checksum = binaryChecksum ((char *) flashBuffer, l - 2, true);

	if ((*((uint16_t *) (flashBuffer + l - 2))) != checksum)
	{
		return -2;
	}

	return 0;
}

int ConnSitech::flashStore ()
{
	return -1;
}

/**
Flash content - integers etc. are low endian.

Starting at location 0 is Secondary Axis (dec/alt) Configuration bytes:
0 long int (4 bytes) Secondary Axis Acceleration
4 UInt (2 bytes) Secondary Axis Servo PID Sample Rate Count Up Value 61440 to 63718
6 Short int (2 bytes) Spare, not used yet.
8 long int (4 bytes) Secondary Axis Position Error Limit
12 int (2 bytes) Secondary axis Proportional while tracking 0-32767
14 int (2 bytes) Secondary axis Integral while tracking 0-32767
16 int (2 bytes) Secondary axis Derivative while tracking 0-32767
18 int (2 bytes) Secondary axis Output Limit 0-32767
20 int (2 bytes) Secondary axis Current Limit 0-255 = 0-48.45 amps.  Max is 10 amps, or 53, so in effect, only 1 byte is used.
22 UInt (2 bytes) Secondary Axis X_Bits 0-65535
24 int (2 bytes) Secondary axis Integral Limit 0-32767
26 long int (4 bytes) Secondary Axis Motor Location at 270 motor electrical deg's
	note: this one is read only.  You can't write to this.  It's calculated during the axis initilization process.
30 long int (4 bytes) Secondary Axis Slew Rate
34 long int (4 bytes) Secondary Axis Pan Rate
38 long int (4 bytes) Secondary Axis Tweak Rate
42 long int (4 bytes) Secondary Axis Guide Rate
46 long int (4 bytes) Secondary Axis Ticks per motor electrical angle
50 long int (4 bytes) Secondary Axis Ticks per axis revolution for motor encoder
54 long int (4 bytes) Secondary Axis Ticks per axis revolution for scope encoder
58 long int (4 bytes) Secondary Axis Ticks per Worm revolution for scope encoder (not used yet)
62 int (2 bytes) Secondary axis six pole data
64 int (2 bytes) Secondary axis Not used
66 int (2 bytes) Secondary axis Proportional while Slewing 0-32767
68 int (2 bytes) Secondary axis Integral while Slewing 0-32767
70 int (2 bytes) Secondary axis Derivative while Slewing 0-32767
72 int (2 bytes) Secondary axis PID Deadband (Zero Crossing Filter) 0-255, but spare byte after
74 int (2 bytes) Secondary axis PID Deadband (Zero Crossing Filter) 0-255, but spare byte after
76 int (2 bytes) Secondary axis Initialization Ending Winding PWM Value, 0-4095.
78 int (2 bytes) Secondary axis Initialization Starting Winding PWM Value, 0-4095.
80 int (2 bytes) Secondary axis Brake Threshold.  WHen the PID output reaches this value, the brake is released if in Auto

From bytes 84 to 99 is spare bytes;
Starting at location 100 is Primary Axis Configuration bytes, see above

from bytes 184 to 399 is spare bytes

Starting at location 400 is common configuration bytes.  Note: None of these are used yet:
400 int (2 bytes) Latitude
402 long int (4 bytes) Sidereal Rate
406 long int (4 bytes) Sidereal Up/Down
410 long int (4 bytes) Sidereal Goal
414 long int (4 bytes) Spiral Distance
418 long int (4 bytes) Spiral Speed
422 int (2 bytes) Baud Rate
424 int (2 bytes) Argo Navis Bits
426 int (2 bytes) Address of controller.
*/

int16_t ConnSitech::getFlashInt16 (int i)
{
	return le16toh (*((uint16_t*) (flashBuffer + i)));
}

int32_t ConnSitech::getFlashInt32 (int i)
{
	return le32toh (*((uint32_t*) (flashBuffer + i)));
}

void ConnSitech::logBuffer (char spec, void *data, size_t len)
{
	struct timeval tv;
	gettimeofday (&tv, NULL);
	write (logFile, &spec, 1);
	write (logFile, &tv, sizeof (tv));
	write (logFile, data, len);
	logCount++;
	if (logCount > 200)
	{
		fsync (logFile);
		logCount = 0;
	}
}
