/* 
 * Driver for Binder enviromental test chamber
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

#include "sensord.h"
#include "connection/modbus.h"
#include <bitset>

namespace rts2sensord
{

/**
 * Binder enviromental test chamber.
 *
 * Current private IP: 10.26.210.189 10001
 * Current rack IP: 192.168.1.11 10001
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Binder:public Sensor
{
	public:
		Binder (int argc, char **argv);
		virtual ~Binder (void);

		virtual int info ();

	protected:
		virtual int processOption (int in_opt);

		virtual int initHardware ();

		virtual int setValue (rts2core::Value *oldValue, rts2core::Value *newValue);

	private:
		HostString *host;
		uint8_t unitId;

		rts2core::ConnModbus *binderConn;

		rts2core::ValueFloat *temperature;
		rts2core::ValueFloat *humidity;	

		rts2core::ValueFloat *set_temp;
		rts2core::ValueFloat *set_hum;
};

}

using namespace rts2sensord;

Binder::Binder (int argc, char **argv):Sensor (argc, argv)
{
	binderConn = NULL;
	host = NULL;
	unitId = 1;

	createValue (temperature, "temperature", "[C] temperature in chamber", false);
	createValue (humidity, "humidity", "[%] relative humidity", false);

	createValue (set_temp, "set_temperature", "[C] temperature setpoint", false, RTS2_VALUE_WRITABLE);
	createValue (set_hum, "set_humidity", "[%] humidity setpoint", false, RTS2_VALUE_WRITABLE);

	addOption ('b', NULL, 1, "Binder TCP/IP address and port (separated by :)");
	addOption ('u', NULL, 1, "unit id/adress (default 1)");
}

Binder::~Binder (void)
{
	delete binderConn;
	delete host;
}

int Binder::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'b':
			host = new HostString (optarg, "502");
			break;
		case 'u':
			unitId = atoi (optarg);
		default:
			return Sensor::processOption (in_opt);
	}
	return 0;
}

float getFloat (const uint16_t regs[2])
{
	float rf;
	((uint16_t *) &rf)[0] = regs[0];
	((uint16_t *) &rf)[1] = regs[1];
	return rf;
}

void setFloat (const float fv, uint16_t regs[2])
{
	memcpy (regs, &fv, 4);
	regs[0] = htobe16 (regs[0]);
	regs[1] = htobe16 (regs[1]);
}

int Binder::info ()
{
	uint16_t regs[2];
	try
	{
		binderConn->readHoldingRegisters (unitId, 0x11a9, 2, regs);
		temperature->setValueFloat (getFloat (regs));
		sendValueAll (temperature);

		binderConn->readHoldingRegisters (unitId, 0x11cd, 2, regs);
		humidity->setValueFloat (getFloat (regs));
		sendValueAll (humidity);

		binderConn->readHoldingRegisters (unitId, 0x1581, 2, regs);
		set_temp->setValueFloat (getFloat (regs));

		binderConn->readHoldingRegisters (unitId, 0x1583, 2, regs);
		set_hum->setValueFloat (getFloat (regs));
	}
	catch (rts2core::ConnError err)
	{
		logStream (MESSAGE_ERROR) << "info " << err << sendLog;
		return -1;
	}
	
	return Sensor::info ();
}

int Binder::initHardware ()
{
	if (host == NULL)
	{
		logStream (MESSAGE_ERROR) << "You must specify zelio hostname (with -z option)." << sendLog;
		return -1;
	}
	binderConn = new rts2core::ConnModbusRTUTCP (this, host->getHostname (), host->getPort ());
	binderConn->setDebug (getDebug ());
	
	uint16_t regs[8];

	try
	{
		binderConn->init ();
		binderConn->readHoldingRegisters (unitId, 0x11a9, 2, regs);
	}
	catch (rts2core::ConnError er)
	{
		logStream (MESSAGE_ERROR) << "initHardware " << er << sendLog;
		return -1;
	}

	int ret = info ();
	if (ret)
		return ret;

	return 0;
}

int Binder::setValue (rts2core::Value *oldValue, rts2core::Value *newValue)
{
	uint16_t regs[2];
	if (oldValue == set_temp)
	{
		setFloat (newValue->getValueFloat (), regs);
		binderConn->writeHoldingRegisters (unitId, 0x1581, 2, regs);
	}
	if (oldValue == set_hum)
	{
		setFloat (newValue->getValueFloat (), regs);
		binderConn->writeHoldingRegisters (unitId, 0x1583, 2, regs);
	}
	return Sensor::setValue (oldValue, newValue);
}

int main (int argc, char **argv)
{
	Binder device (argc, argv);
	return device.run ();
}

