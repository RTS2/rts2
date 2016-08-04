/* 
 * Driver for SAAO Delta-DVP Series PLC copula
 * Copyright (C) 2016 Petr Kubanek <petr@kubanek.net>
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

#include "cupola.h"
#include "connection/serial.h"
#include "error.h"

#define STX			0x3A
#define END1			0x0D
#define END0			0x0A

#define CMD_READ_COIL_STATUS	"01"
#define CMD_READ_INPUT_STATUS	"02"
#define CMD_READ_HOLDING_REG	"03"
#define CMD_FORCE_SINGLE_COIL	"05"
#define CMD_PRESET_SINGLE_REG	"06"
#define CMD_FORCE_MULTI_COILS	"15"
#define CMD_PRESET_MULTI_REG	"16"
#define CMD_REPORT_SLAVE_ID	"17"

#define STATUS_BIT_LIGHTS	0x0080

namespace rts2dome
{

/**
 * Driver for DELTA DVP Series PLC controlling 1m SAAO dome.
 *
 * @author Petr Kubánek <petr@kubanek.net>
 */
class SAAO:public Cupola
{
	public:
		SAAO (int argc, char **argv);
		virtual ~SAAO ();

	protected:
		virtual int processOption (int opt);
		virtual int initHardware ();

		virtual int info ();

		virtual int startOpen ();
		virtual long isOpened ();
		virtual int endOpen ();

		virtual int startClose ();
		virtual long isClosed ();
		virtual int endClose ();

		virtual double getSlitWidth (double alt);

	private:
		const char *device_file;

		void sendCommand (uint8_t address, const char *cmd, const char *data, char *ret, size_t retl);
		void readRegisters (uint8_t address, uint16_t regaddr, int len, uint8_t *rbuf);

		rts2core::ConnSerial *domeConn;

		rts2core::ValueBool *lightsOn;
};

}

using namespace rts2dome;

uint8_t hex2num (char hex)
{
	if (hex >= '0' && hex <= '9')
		return hex - '0';
	if (hex >= 'A' && hex <= 'F')
		return hex - 'A' + 10;
	throw rts2core::Error ("invalid hex");
}

uint16_t hexp2num (char h1, char h2)
{
	uint16_t ret = hex2num (h1);
	return (ret << 4) | hex2num (h2);
}

/**
 * Calculates longitude CRC on data. Add modulo 0xFF, produces hex represenation of the CRC.
 */
void calculateLCRC (const char *buf, size_t len, char crc[3])
{
	// CRC is calculated from hex representation..length must be dividable by 2
	int sumhi = 0;
	int sumlo = 0;
	if (len % 2 != 0)
		throw rts2core::Error ("length of CRC buffer must be dividable by even!");
	for (size_t i = 0; i < len; i++)
	{
		sumhi += hex2num (buf[i++]);
		sumlo += hex2num (buf[i]);
	}
	sumhi = ~(0xF - (sumhi & 0xF));
	sumlo = 0x10 - (sumlo & 0xF);
	int sum = ((sumhi << 4) & 0xF0) | (sumlo & 0x0F);
	snprintf (crc, 3, "%02X", sum);
}

SAAO::SAAO (int argc, char **argv):Cupola (argc, argv)
{
	device_file = "/dev/ttyS0";
	domeConn = NULL;

	createValue (lightsOn, "lights", "dome lights", false, RTS2_DT_ONOFF);

	addOption ('f', NULL, 1, "path to device file, default is /dev/ttyS0");
}

SAAO::~SAAO ()
{
	delete domeConn;
}

int SAAO::processOption (int opt)
{
	switch (opt)
	{
		case 'f':
			device_file = optarg;
			break;
		default:
			return Cupola::processOption (opt);
	}
	return 0;
}

int SAAO::initHardware ()
{
	domeConn = new rts2core::ConnSerial (device_file, this, rts2core::BS9600, rts2core::C7, rts2core::EVEN, 50);
	domeConn->setDebug (getDebug ());
	int ret = domeConn->init ();
	if (ret)
		return ret;
	info ();
	return 0;
}

int SAAO::info ()
{
	uint8_t reg[16];
	readRegisters (1, 0x106E, 8, reg);

	lightsOn->setValueBool (reg[0] & STATUS_BIT_LIGHTS);
	return Cupola::info ();
}

int SAAO::startOpen ()
{
	return 0;
}

long SAAO::isOpened ()
{
	return 0;
}

int SAAO::endOpen ()
{
	return 0;
}

int SAAO::startClose ()
{
	return 0;
}

long SAAO::isClosed ()
{
	return 0;
}

int SAAO::endClose ()
{
	return 0;
}

double SAAO::getSlitWidth (double alt)
{
	return 1;
}

void SAAO::sendCommand (uint8_t address, const char *cmd, const char *data, char *ret, size_t retl)
{
	size_t dl = strlen (data);
	char buf[8 + dl];
	buf[0] = STX;
	snprintf (buf + 1, 3, "%02X", address);
	memcpy (buf + 3, cmd, 2);
	strcpy (buf + 5, data);
	calculateLCRC (buf + 1, 4 + dl, buf + 5 + dl);
	buf[7 + dl] = END1;
	buf[8 + dl] = END0;
	char retb[100];
	int retc = domeConn->writeRead (buf, 9 + dl, retb, 100, "\r\n");
	if (retc < 10)
		throw rts2core::Error ("short reply to read command");
	if (memcmp (buf, retb, 5) != 0)
		throw rts2core::Error ("invalid start of reply packet reply");
	memcpy (ret, retb + 5, (int) retl > retc ? retc - 9 : retl);
}

void SAAO::readRegisters (uint8_t address, uint16_t regaddr, int len, uint8_t *rbuf)
{
	char cmd[9];
	snprintf (cmd, 9, "%04X%04d", regaddr, len);
	char retb[len * 2 + 2];
	sendCommand (address, CMD_READ_HOLDING_REG, cmd, retb, len * 2 + 2);
	if (hexp2num (retb[0], retb[1]) != len * 2)
		throw rts2core::Error ("not all registers were read");
	// converts hex to numbers
	for (int i = 0; i < len; i++)
		rbuf[i] = hexp2num (retb[i + 2], retb[i + 3]);
}

int main (int argc, char **argv)
{
	SAAO dome (argc, argv);
	return dome.run ();
}
