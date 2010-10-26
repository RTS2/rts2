/* 
 * Driver for ThorLaser laser source.
 * Copyright (C) 2010 Petr Kubanek <petr@kubanek.net>
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

class ThorLaser:public Sensor
{
	public:
		ThorLaser (int argc, char **argv);

	protected:
		virtual int processOption (int opt);
		virtual int init ();
		virtual int info ();

		virtual int setValue (Rts2Value *old_value, Rts2Value *new_value);
	private:
		char *device_file;
		rts2core::ConnSerial *laserConn;

		Rts2ValueBool *enable1;
		Rts2ValueFloat *temp1;
};

}

using namespace rts2sensord;

int ThorLaser::processOption (int opt)
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

int ThorLaser::init ()
{
	int ret;
	ret = Sensor::init ();
	if (ret)
		return ret;

	if (device_file == NULL)
	{
		logStream (MESSAGE_ERROR) << "you must specify device file (TTY port)" << sendLog;
		return -1;
	}

	laserConn = new rts2core::ConnSerial (device_file, this, rts2core::BS115200, rts2core::C8, rts2core::NONE, 10);
	ret = laserConn->init ();
	if (ret)
		return ret;
	
	laserConn->flushPortIO ();
	laserConn->setDebug (true);

	return 0;
}

int ThorLaser::info ()
{
	int ret;
	char buf[50];
	ret = laserConn->writeRead ("channel=1\r", 10, buf, 50, '\r');

	ret = laserConn->writeRead ("temp?\r", 6, buf, 50, '\r');
	if (ret < 0)
		return ret;
	if (!strcmp (buf, "< temp?"))
	{
		logStream (MESSAGE_ERROR) << "invalid reply to temperature command: " << buf << sendLog;
		return -1;
	}
	ret = laserConn->readPort (buf, 50, '\r');
	if (ret < 0)
		return ret;

	std::istringstream is (buf);
	float f;
	is >> f;

	if (is.fail ())
	{
		logStream (MESSAGE_ERROR) << "failed to parse buffer " << buf << sendLog;
		return -1;
	}
	temp1->setValueFloat (f);

	ret = laserConn->writeRead ("enable?\r", 8, buf, 50, '\r');
	if (ret < 0)
		return ret;

	ret = laserConn->readPort (buf, 50, '\r');
	if (ret < 0)
		return ret;
	enable1->setValueInteger (buf[0] == '1');

	return Sensor::info ();
}

int ThorLaser::setValue (Rts2Value *old_value, Rts2Value *new_value)
{
	if (old_value == enable1)
	{
		char buf[50];
		sprintf (buf, "enable=%i\r", ((Rts2ValueBool *) new_value)->getValueBool () ? 1 : 0);
		int ret = laserConn->writeRead (buf, strlen (buf), buf, 50, '\r');
		if (ret < 0)
			return ret;
		return 0;
	}
	return Sensor::setValue (old_value, new_value);
}

ThorLaser::ThorLaser (int argc, char **argv): Sensor (argc, argv)
{
	device_file = NULL;
	laserConn = NULL;

	createValue (enable1, "channel_1", "channel 1 on/off", true, RTS2_VALUE_WRITABLE | RTS2_DT_ONOFF);
	enable1->setValueBool (false);

	createValue (temp1, "temp_1", "temperature of the 1st channel", true);

	addOption ('f', NULL, 1, "serial port with the module (ussually /dev/ttyUSB for ThorLaser USB serial connection");
}

int main (int argc, char **argv)
{
	ThorLaser device (argc, argv);
	return device.run ();
}

