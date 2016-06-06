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

#include <endian.h>

using namespace rts2core;

ConnSitech::ConnSitech (const char *devName, Block *_master):ConnSerial (devName, _master, BS19200, C8, NONE, 50, 5)
{
	binary = false;
	version = -1;
	PIDSampleRate = 1;
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
	else
	{
		sitechType = FORCE_ONE;
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

void ConnSitech::getAxisStatus (char axis, SitechAxisStatus &ax_status)
{
	switchToBinary ();
	siTechCommand (axis, "XS");

	readAxisStatus (ax_status);
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

	// read back reply
	SitechAxisStatus ax_status;
	readAxisStatus (ax_status);
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

	SitechAxisStatus ax_status;
	readAxisStatus (ax_status);
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

void ConnSitech::getControllerStatus (SitechControllerStatus &controller_status)
{
}

void ConnSitech::getConfiguration (SitechControllerConfiguration &config)
{
	switchToBinary ();
	siTechCommand ('S', "C");

	readConfiguration (config);
}

void ConnSitech::readAxisStatus (SitechAxisStatus &ax_status)
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
		throw Error ("invalid checksum in readAxisStatus");
	}

	// fill in proper return values..
	ax_status.address = ret[0];
	ax_status.x_pos = le32toh (*((uint32_t *) (ret + 1)));
	ax_status.y_pos = le32toh (*((uint32_t *) (ret + 5)));
	ax_status.x_enc = le32toh (*((uint32_t *) (ret + 9)));
	ax_status.y_enc = le32toh (*((uint32_t *) (ret + 13)));

	ax_status.keypad = ret[17];
	ax_status.x_bit = ret[18];
	ax_status.y_bit = ret[19];
	ax_status.extra_bits = ret[20];
	ax_status.ain_1 = le16toh (*((uint16_t *) (ret + 21)));
	ax_status.ain_2 = le16toh (*((uint16_t *) (ret + 23)));
	ax_status.mclock = le32toh (*((uint32_t *) (ret + 25)));
	ax_status.temperature = ret[29];
	ax_status.y_worm_phase = ret[30];
	memcpy (ax_status.x_last, ret + 31, 4);
	memcpy (ax_status.y_last, ret + 35, 4);
}

void ConnSitech::readConfiguration (SitechControllerConfiguration &config)
{
	char ret[400];
	size_t len = readPort (ret, 400);

	if (len != 130)
	{
		std::cout << "len " << len << std::endl;
		flushPortIO ();
		throw Error ("cannot read Sitech configuration");
	}

	// checksum checks
	uint16_t checksum = binaryChecksum (ret, 128, false);
	if ((*((uint16_t *) (ret + 129))) != checksum)
	{
		std::cerr << *((uint16_t *) (ret + 129)) << " " << checksum << std::endl;
		throw Error ("invalid checksum in readConfiguration");
	}

	// fill in proper values

	config.x_acc = le32toh (*((uint32_t *) (ret + 0)));
	config.x_backlash = le32toh (*((uint32_t *) (ret + 4)));
	config.x_error_limit = le16toh (*((uint16_t *) (ret + 8)));
	config.x_p_gain = le16toh (*((uint16_t *) (ret + 10)));
	config.x_i_gain = le16toh (*((uint16_t *) (ret + 12)));
	config.x_d_gain = le16toh (*((uint16_t *) (ret + 14)));
	config.x_o_limit = le16toh (*((uint16_t *) (ret + 16)));
	config.x_c_limit = le16toh (*((uint16_t *) (ret + 18)));
	config.x_i_limit = le16toh (*((uint16_t *) (ret + 20)));
	config.x_bits = ret[21];

	config.p_0 = ret[22];

	config.y_acc = le32toh (*((uint32_t *) (ret + 23)));
	config.y_backlash = le32toh (*((uint32_t *) (ret + 27)));
	config.y_error_limit = le16toh (*((uint16_t *) (ret + 31)));
	config.y_p_gain = le16toh (*((uint16_t *) (ret + 33)));
	config.y_i_gain = le16toh (*((uint16_t *) (ret + 35)));
	config.y_d_gain = le16toh (*((uint16_t *) (ret + 37)));
	config.y_o_limit = le16toh (*((uint16_t *) (ret + 39)));
	config.y_c_limit = le16toh (*((uint16_t *) (ret + 41)));
	config.y_i_limit = le16toh (*((uint16_t *) (ret + 43)));
	config.y_bits = ret[44];

	config.p_1 = ret[45];

	config.address = ret[46];

	config.p_2 = ret[47];

	config.eq_rate = le32toh (*((uint32_t *) (ret + 48)));
	config.eq_updown = le32toh (*((uint32_t *) (ret + 52)));
	
	config.tracking_goal = le32toh (*((uint32_t *) (ret + 56)));

	config.latitude = be16toh (*((uint16_t *) (ret + 60)));

	config.y_enc_ticks = be32toh (*((uint32_t *) (ret + 62)));
	config.x_enc_ticks = be32toh (*((uint32_t *) (ret + 66)));

	config.y_mot_ticks = be32toh (*((uint32_t *) (ret + 70)));
	config.x_mot_ticks = be32toh (*((uint32_t *) (ret + 74)));

	config.x_slew_rate = le32toh (*((uint32_t *) (ret + 78)));
	config.y_slew_rate = le32toh (*((uint32_t *) (ret + 82)));

	config.x_pan_rate = le32toh (*((uint32_t *) (ret + 86)));
	config.y_pan_rate = le32toh (*((uint32_t *) (ret + 90)));

	config.x_guide_rate = le32toh (*((uint32_t *) (ret + 94)));
	config.y_guide_rate = le32toh (*((uint32_t *) (ret + 98)));

	config.pec_auto = ret[102];

	config.p_3 = ret[103];
	config.p_4 = ret[104];

	config.p_5 = ret[105];

	config.p_6 = ret[106];

	config.p_7 = ret[107];

	config.local_deg = le32toh (*((uint32_t *) (ret + 108)));
	config.local_speed = le32toh (*((uint32_t *) (ret + 112)));

	config.backhlash_speed = le32toh (*((uint32_t *) (ret + 116)));

	config.y_pec_ticks = le32toh (*((uint32_t *) (ret + 120)));

	config.p_8 = ret[121];
	config.p_9 = ret[122];	 

}

void ConnSitech::writePortChecksumed (const char *cmd, size_t len)
{
	size_t ret = writePort (cmd, len);
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
			return (double) speed / loop_ticks * (360.0 * PIDSampleRate / SPEED_MULTI);
		default:
			return 0;
	}
}
