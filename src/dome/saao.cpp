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
#define CMD_FORCE_MULTI_COILS	"0F"
#define CMD_PRESET_MULTI_REG	"10"
#define CMD_REPORT_SLAVE_ID	"11"

// byte 1
#define STATUS_LIGHTS_MANUAL	0x0100
#define STATUS_SHUTTER_MAN_OP	0x0200
#define STATUS_SHUTTRE_MAN_CL	0x0400
#define STATUS_DOME_MAN_RIGHT	0x0800
#define STATUS_DOME_MAN_LEFT	0x1000
#define STATUS_SHUTTER_POWER	0x4000
#define STATUS_ROTAT_POWER	0x8000

// byte 2
#define STATUS_SHUTTER_CLOSED	0x0001
#define STATUS_SHUTTER_OPENED	0x0002
#define STATUS_SHUTTER_MOVING	0x0004
#define STATUS_SHUTTER_FAULTS	0x0008
#define STATUS_DOME_MOVING	0x0010
#define STATUS_DOME_FAULT	0x0020
#define STATUS_LIGHTS		0x0080

// byte 3
#define STATUS_EM_PRESSED	0x0400
#define STATUS_CLOSED_REM	0x0800
#define STATUS_CLOSED_RAIN	0x1000
#define STATUS_PWR_FAILURE	0x2000
#define STATUS_CLOSED_PWR	0x4000
#define STATUS_WATCHDOG		0x8000


// byte 4
#define STATUS_REMOTE		0x0001
#define STATUS_RAINING		0x0002

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

		virtual int setValue (rts2core::Value *oldValue, rts2core::Value *newValue);

		virtual int startOpen ();
		virtual long isOpened ();
		virtual int endOpen ();

		virtual int startClose ();
		virtual long isClosed ();
		virtual int endClose ();

		virtual double getSlitWidth (double alt);

	private:
		const char *device_file;

		void sendCommand (uint8_t address, const char *cmd, const char *data, char *ret, int retl);
		void readRegisters (uint8_t address, uint16_t regaddr, int len, uint16_t *rbuf);
		void setRegisters (uint8_t address, uint16_t regaddr, int len, const uint16_t *sbuf);

		void setDome (bool lights, bool rotatP, bool shutterP, bool shutterClose, bool shutterOpen, float reqPosition);

		rts2core::ConnSerial *domeConn;

		rts2core::ValueBool *remoteControl;
		rts2core::ValueBool *raining;
		rts2core::ValueBool *domeAuto;

		rts2core::ValueBool *shutterClosed;
		rts2core::ValueBool *shutterOpened;
		rts2core::ValueBool *shutterMoving;
		rts2core::ValueBool *shutterFaults;

		rts2core::ValueBool *domeMoving;
		rts2core::ValueBool *domeFault;

		rts2core::ValueBool *lightsOn;
		rts2core::ValueBool *lightsManual;

		rts2core::ValueBool *shutterPower;
		rts2core::ValueBool *rotatPower;
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

uint8_t hexp2num (char h1, char h2)
{
	uint8_t ret = hex2num (h1);
	return (ret << 4) | hex2num (h2);
}

uint16_t hexq2num (uint8_t h1, uint8_t h2)
{
	uint16_t ret = h1;
	return (ret << 8) | h2;
}

// convert number in binary hex to number
// e.g. 0x3535 = 3535
uint16_t bhex2num (uint16_t h)
{
	uint16_t ret = 0;
	uint16_t mask = 0xF000;
	for (int i = 12; i >= 0; i -= 4)
	{
		ret *= 10;
		ret += (h & mask) >> i;
		mask >>= 4;
	}
	return ret;
}

// convert number to hex
// expample 3536 = 0x3536
uint16_t num2hex (uint16_t num)
{
	uint16_t ret = 0;

	for (int i = 0; i <= 12; i += 4)
	{
		ret |= (num % 10) << i;
		num /= 10;
	}

	return ret;
}

/**
 * Calculates longitude CRC on data. Add modulo 0xFF, produces hex represenation of the CRC.
 */
void calculateLCRC (const char *buf, size_t len, char crc[3])
{
	// CRC is calculated from hex representation..length must be dividable by 2
	uint8_t sum = 0;
	if (len % 2 != 0)
		throw rts2core::Error ("length of CRC buffer must be dividable by even!");
	for (size_t i = 0; i < len; i += 2)
	{
		sum += hexp2num (buf[i], buf[i+1]);
	}
	snprintf (crc, 3, "%02X", (0x100 - (sum & 0xFF)));
}

SAAO::SAAO (int argc, char **argv):Cupola (argc, argv)
{
	device_file = "/dev/ttyS0";
	domeConn = NULL;

	createValue (remoteControl, "remote", "remote control enabled", false);
	createValue (raining, "raining", "if true, rain is detected", false);
	createValue (domeAuto, "dome_auto", "dome track azimuth", false, RTS2_VALUE_WRITABLE);
	domeAuto->setValueBool (true);

	createValue (shutterClosed, "shutter_closed", "dome shutter is closed", false);
	createValue (shutterOpened, "shutter_opened", "dome shutter is opened", false);
	createValue (shutterMoving, "shutter_moving", "dome shutter is moving", false);
	createValue (shutterFaults, "shutter_faults", "dome shutter faults", false);

	createValue (domeMoving, "dome_moving", "dome is moving", false);
	createValue (domeFault, "dome_fault", "dome fault", false);

	createValue (lightsOn, "lights", "dome lights", false, RTS2_DT_ONOFF | RTS2_VALUE_WRITABLE);
	createValue (lightsManual, "lights_manual", "dome lights switched on by manual switch", false, RTS2_DT_ONOFF);

	createValue (shutterPower, "shutter_power", "shutter power", false, RTS2_DT_ONOFF | RTS2_VALUE_WRITABLE);
	createValue (rotatPower, "rotator_power", "dome rotator power", false, RTS2_DT_ONOFF | RTS2_VALUE_WRITABLE);

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
	return 0;
}

int SAAO::info ()
{
	uint16_t reg[8];
	readRegisters (1, 0x106E, 8, reg);

	remoteControl->setValueBool (reg[1] & STATUS_REMOTE);
	raining->setValueBool (reg[1] & STATUS_RAINING);

	shutterClosed->setValueBool (reg[0] & STATUS_SHUTTER_CLOSED);
	shutterOpened->setValueBool (reg[0] & STATUS_SHUTTER_OPENED);
	shutterMoving->setValueBool (reg[0] & STATUS_SHUTTER_MOVING);
	shutterFaults->setValueBool (reg[0] & STATUS_SHUTTER_FAULTS);

	domeMoving->setValueBool (reg[0] & STATUS_DOME_MOVING);
	domeFault->setValueBool (reg[0] & STATUS_DOME_FAULT);

	lightsOn->setValueBool (reg[0] & STATUS_LIGHTS);

	lightsManual->setValueBool (reg[0] & STATUS_LIGHTS_MANUAL);

	shutterPower->setValueBool (reg[0] & STATUS_SHUTTER_POWER);
	rotatPower->setValueBool (reg[0] & STATUS_ROTAT_POWER);

	setCurrentAz (bhex2num (reg[2]) / 10.0);

	return Cupola::info ();
}

int SAAO::setValue (rts2core::Value *oldValue, rts2core::Value *newValue)
{
	if (oldValue == lightsOn)
	{
		setDome (((rts2core::ValueBool *) newValue)->getValueBool (), rotatPower->getValueBool (), shutterPower->getValueBool (), false, false, NAN);
		return 0;
	}
//	else if (oldValue == domePosition)
//	{
//		setDome (lightsOn->getValueBool (), rotatPower->getValueBool (), shutterPower->getValueBool (), false, false, newValue->getValueFloat ());
//		return 0;
//	}
	return Cupola::setValue (oldValue, newValue);
}

int SAAO::startOpen ()
{
	setDome (lightsOn->getValueBool (), true, true, false, true, NAN);
	return 0;
}

long SAAO::isOpened ()
{
	info ();
	return shutterOpened->getValueBool () ? -2 : 0;
}

int SAAO::endOpen ()
{
	return 0;
}

int SAAO::startClose ()
{
	setDome (lightsOn->getValueBool (), true, true, true, false, NAN);
	return 0;
}

long SAAO::isClosed ()
{
	info ();
	return shutterClosed->getValueBool () ? -2 : 0;
}

int SAAO::endClose ()
{
	return 0;
}

double SAAO::getSlitWidth (double alt)
{
	return 1;
}

void SAAO::sendCommand (uint8_t address, const char *cmd, const char *data, char *ret, int retl)
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
		throw rts2core::Error ("invalid start of response packet");

	// checks for CRC
	char lcrc[3];
	calculateLCRC (retb + 1, retc - 5, lcrc);

	if (memcmp (retb + retc - 4, lcrc, 2) != 0)
		throw rts2core::Error ("invalid reply LCRC");

	if (retl < (retc - 9))
		throw rts2core::Error ("too short buffer for reply");
	memcpy (ret, retb + 5, retc - 9);
}

void SAAO::readRegisters (uint8_t address, uint16_t regaddr, int len, uint16_t *rbuf)
{
	char data[9];
	snprintf (data, 9, "%04X%04d", regaddr, len);
	char retb[len * 4 + 2];
	sendCommand (address, CMD_READ_HOLDING_REG, data, retb, len * 4 + 2);
	if (hexp2num (retb[0], retb[1]) != len * 2)
		throw rts2core::Error ("not all registers were read");

	// converts hex to numbers
	for (int i = 0; i < len; i++)
		rbuf[i] = hexq2num (hexp2num (retb[i * 4 + 2], retb[i * 4 + 3]), hexp2num (retb[i * 4 + 4], retb[i * 4 + 5]));
}

void SAAO::setRegisters (uint8_t address, uint16_t regaddr, int len, const uint16_t *sbuf)
{
	char rbuf[8];
	char payload[len * 4 + 11];
	// payload header
	snprintf (payload, 11, "%04X%04X%02X", regaddr, len, len * 2);
	for (int i = 0; i < len; i++)
		snprintf (payload + 10 + i * 4, 5, "%04X", sbuf[i]);
	sendCommand (address, CMD_PRESET_MULTI_REG, payload, rbuf, 8);
}

void SAAO::setDome (bool lights, bool rotatP, bool shutterP, bool shutterClose, bool shutterOpen, float reqPosition)
{
	uint16_t data[4];
	memset (data, 0, sizeof (data));
	// reset watchdog
	data[1] |= 0x8000;

	// request control, if not enabled..
	if (remoteControl->getValueBool () == false)
	{
		// get transition for remote control..
		uint16_t req = 0x8000;
		setRegisters (1, 0x1065, 1, &req);
		data[1] |= 0x0001;
	}

	// keep power as requested
	if (rotatP)
		data[0] |= 0x8000;
	if (shutterP)
		data[0] |= 0x4000;

	if (shutterClose)
		data[0] |= 0x0001;
	else if (shutterOpen)
		data[0] |= 0x0002;
	
	if (lights)
		data[0] |= 0x0100;

	if (!isnan (reqPosition) && reqPosition > 0 && reqPosition < 360)
	{
		data[0] |= 0x0008;
		data[2] = num2hex (reqPosition * 10);
		if (domeAuto->getValueBool ())
			data[0] |= 0x0010;
	}
	setRegisters (0x01, 0x1064, 3, data);
}

int main (int argc, char **argv)
{
	SAAO dome (argc, argv);
	return dome.run ();
}
