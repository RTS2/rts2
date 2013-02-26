/* 
 * DWS T5 temperature and humidity sensor, manufactured by DiHuiTech.
 * Copyright (C) 2012 Petr Kubanek <petr@kubanek.net>
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
#include "connection/serial.h"

namespace rts2sensord
{

/**
 * DWS T5 sensor.
 *
 * @author Petr Kubánek <petr@kubanek.net>
 */
class DWST5:public Sensor
{
	public:
		DWST5 (int argc, char **argv);
		virtual ~DWST5 ();

	protected:
		virtual int processOption (int opt);
		virtual int initHardware ();
		virtual int info ();

	private:
		rts2core::ValueFloat *temperature;
		rts2core::ValueFloat *humidity;

		const char *serial;
		rts2core::ConnSerial *serialConn;
};

}

using namespace rts2sensord;

DWST5::DWST5(int argc, char **argv):Sensor (argc, argv)
{
	createValue (temperature, "TEMPERATURE", "[C] sensor temperature", true);
	createValue (humidity, "HUMIDITY", "[%] sensor relative humidity", true);

	serial = NULL;
	serialConn = NULL;

	addOption ('f', NULL, 1, "serial port of the device");	
}

DWST5::~DWST5 ()
{
	delete serialConn;
}

int DWST5::processOption (int opt)
{
	switch (opt)
	{
		case 'f':
			serial = optarg;
			break;
		default:
			return Sensor::processOption (opt);
	}
	return 0;
}

int DWST5::initHardware ()
{
	serialConn = new rts2core::ConnSerial (serial, this, rts2core::BS9600, rts2core::C8, rts2core::NONE, 10);
	serialConn->setDebug (getDebug ());
	int ret = serialConn->init ();
	if (ret)
		return ret;
	serialConn->flushPortIO ();
	return info ();
}

int DWST5::info ()
{
	unsigned char buf[8] = { 0x01, 0x04, 0x00, 0x00, 0x00, 0x02, 0x71, 0xCB };
	char repl[9];
	if (serialConn->writeRead ((char *) buf, 8, repl, 9) != 9)
		return -1;

	temperature->setValueFloat ((float) ((((int16_t) repl[3]) << 8) | (0x00ff & repl[4])) / 10.0);

	humidity->setValueFloat ((float) ((((int16_t) repl[5]) << 8) | (0x00ff & repl[6])) / 10.0);
	return Sensor::info ();
}

int main (int argc, char **argv)
{
	DWST5 device (argc, argv);
	return device.run ();
}
