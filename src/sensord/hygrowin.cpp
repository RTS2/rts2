/* 
 * Rotronic AG MOK-WIN / Hygro-Win sensor.
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

#include "sensord.h"

#include "../utils/connserial.h"

namespace rts2sensord
{

/**
 * Rotronic AG MOK-WIN / Hygrowin humidity and temperature sensor.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Hygrowin: public Sensor
{
	public:
		Hygrowin (int argc, char **argv);
		virtual ~Hygrowin (void);

	protected:
		virtual int processOption (int in_opt);
		virtual int init ();

		virtual int info ();
	private:
		char *device_file;
		rts2core::ConnSerial *hygroConn;

		rts2core::ValueFloat *temperature;
		rts2core::ValueFloat *humidity;
};

};

using namespace rts2sensord;

Hygrowin::Hygrowin (int argc, char **argv):Sensor (argc, argv)
{
	hygroConn = NULL;

	createValue (temperature, "TEMPERATURE", "measured temperature (in degrees Celsius)", true);
	createValue (humidity, "HUMIDITY", "measured humidity (%)", true);

	addOption ('f', NULL, 1, "serial port with HygroWin device");

	setIdleInfoInterval (2);
}

Hygrowin::~Hygrowin (void)
{
	delete hygroConn;
}

int Hygrowin::processOption (int opt)
{
	switch (opt)
	{
		case 'f':
			device_file = optarg;
			break;
		default:
			return Sensor::processOption (opt);
	}
	return 0;
}

int Hygrowin::init ()
{
	int ret;
	ret = Sensor::init ();
	if (ret)
		return ret;

	hygroConn = new rts2core::ConnSerial (device_file, this, rts2core::BS2400, rts2core::C7, rts2core::EVEN, 40);
	ret = hygroConn->init ();
	if (ret)
		return ret;
	
	hygroConn->flushPortIO ();
	hygroConn->writePort ("\r", 1);

	return 0;
}

int Hygrowin::info ()
{
	char buf[25];
	int ret = hygroConn->readPort (buf, 25, '\r');
	if (ret <= 0)
	{
		logStream (MESSAGE_ERROR) << "Cannot read from serial port" << sendLog;
		return -1;
	}
	if (ret != 21)
	{
		logStream (MESSAGE_ERROR) << "Invalid length readed from the serial port " << ret << sendLog;
		return -1;
	}
	// flush possibly any old responses
	hygroConn->flushPortIO ();
	// parse response
	std::istringstream _is (buf + 8);
	float n1, n2;
	char f;
	_is >> n1 >> f >> n2;
	if (_is.fail ())
	{
		logStream (MESSAGE_ERROR) << "Cannot parse reply:" << buf << sendLog;
		return -1;
	}
	temperature->setValueFloat (n1);
	humidity->setValueFloat (n2);
	return Sensor::info ();
}

int main (int argc, char **argv)
{
	Hygrowin device (argc, argv);
	return device.run ();
}
