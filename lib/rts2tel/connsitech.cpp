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

#include "connection/sitech.h"

#include <endian.h>

using namespace rts2teld;

ConnSitech::ConnSitech (const char *devName, rts2core::Block *_master):rts2core::ConnSerial (devName, _master, rts2core::BS19200, rts2core::C8, rts2core::NONE, 5,5)
{
}

int ConnSitech::init ()
{
	int ret = rts2core::ConnSerial::init ();
	if (ret)
		return ret;

	// check if checksum mode is on
	if (getSiTechValue ('Y', "XY") == 0)
	{
		// switch to checksum mode
		ret = writePort ("YXY1\r", 5);
		if (ret < 0)
			return -1;
	}
	return 0;
}

void ConnSitech::siTechCommand (const char axis, const char *cmd)
{
	size_t len = strlen (cmd);
	char ccmd[len + 3];

	ccmd[0] = axis;
	memcpy (ccmd + 1, cmd, len);
	ccmd[len + 1] = '\r';
	ccmd[len + 2] = '\0';

	writePortChecksumed (ccmd, len + 2);
}

int32_t ConnSitech::getSiTechValue (const char axis, const char *val)
{
	siTechCommand (axis, val);

	char ret[100];

	size_t len = readPort (ret, 100, "\n");
	if (len < 0)
		throw rts2core::Error (std::string ("cannot read response get value command ") + val);

	return atol (ret + 1);
}

void ConnSitech::getAxisStatus (char axis, SitechAxisStatus &ax_status)
{
	siTechCommand (axis, "XS");

	readAxisStatus (ax_status);
}

void ConnSitech::sendAxisRequest (const char axis, SitechAxisRequest &ax_request)
{
	siTechCommand (axis, "XR");

	char data[34];

	*((uint32_t *) (data)) = htole32 (ax_request.x_dest);
	*((uint32_t *) (data + 4)) = htole32 (ax_request.x_speed);
	*((uint32_t *) (data + 8)) = htole32 (ax_request.y_dest);
	*((uint32_t *) (data + 12)) = htole32 (ax_request.y_speed);

	*((uint32_t *) (data + 16)) = htole32 (ax_request.x_rate_adder);
	*((uint32_t *) (data + 20)) = htole32 (ax_request.y_rate_adder);

	*((uint32_t *) (data + 24)) = htole32 (ax_request.x_rate_adder_t);
	*((uint32_t *) (data + 28)) = htole32 (ax_request.y_rate_adder_t);

	*((uint16_t *) (data + 32)) = htole16 (binaryChecksum (data, 32));

	// sends the data..
	writePort (data, 34);

	// read back reply
	SitechAxisStatus ax_status;
	readAxisStatus (ax_status);
}

void ConnSitech::setSiTechValue (const char axis, const char *val, int value)
{
	char *ccmd = NULL;
	size_t len = asprintf (&ccmd, "%c%s%d\r", axis, val, value);

	try
	{
		writePortChecksumed (ccmd, len);
	}
	catch (rts2core::Error &er)
	{
		free (ccmd);
		throw er;
	}

	free (ccmd);
}

void ConnSitech::getControllerStatus (SitechControllerStatus &controller_status)
{

}

void ConnSitech::readAxisStatus (SitechAxisStatus &ax_status)
{
	char ret[42];

	size_t len = readPort (ret, 41);
	if (len != 41)
	{
		flushPortIO ();
		throw rts2core::Error ("cannot read all 41 bits from status response!");
	}

	// checksum checks
	uint16_t checksum = binaryChecksum (ret, 39);

	if ((*((uint16_t *) (ret + 39))) != checksum)
	{
		std::cerr << "checksum expected " << checksum << " received " << (*((uint16_t *) (ret + 39))) << std::endl;
		throw rts2core::Error ("invalid checksum!");
	}

	// fill in proper return values..
	ax_status.address = ret[0];
	ax_status.x_pos = ntohl (*(ret + 1));
	ax_status.y_pos = ntohl (*(ret + 5));
	ax_status.x_enc = ntohl (*(ret + 9));
	ax_status.y_enc = ntohl (*(ret + 13));

	ax_status.keypad = ret[17];
	ax_status.x_bit = ret[18];
	ax_status.y_bit = ret[19];
	ax_status.extra_bit = ret[20];
	ax_status.ain_1 = ntohs (*(ret + 21));
	ax_status.ain_2 = ntohs (*(ret + 23));
	ax_status.mclock = ntohl (*(ret + 25));
	ax_status.temperature = ret[29];
	ax_status.y_worm_phase = ret[30];
	ax_status.x_last = ntohl (*(ret + 31));
	ax_status.y_last = ntohl (*(ret + 35));
}

void ConnSitech::writePortChecksumed (const char *cmd, size_t len)
{
	size_t ret = writePort (cmd, len);
	if (ret < 0)
		throw rts2core::Error (std::string ("cannot write ") + cmd + " :" + strerror (errno));

	ret = writePort ((char) calculateChecksum (cmd, len));
	if (ret < 0)
		throw rts2core::Error (std::string ("cannot write checksum of ") + cmd + " :" + strerror (errno));
}

uint8_t ConnSitech::calculateChecksum (const char *cbuf, size_t len)
{
	uint8_t ret = 0;
	for (size_t i = 0; i < len; i++)
		ret += cbuf[i];

	return ~ret;
}

uint16_t ConnSitech::binaryChecksum (const char *dbuf, size_t blen)
{
	uint16_t checksum = 0;

	for (size_t i = 0; i < blen; i++)
		checksum += (uint8_t) dbuf[i];

	return (checksum ^ 0xFF00);
}
