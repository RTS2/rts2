/* 
 * Sensor for Reinhardt weather station
 * Copyright (C) 2015 Petr Kubanek <petr@kubanek.net>
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
 * Class for Reinhard device.
 *
 * @author Petr Kubánek <petr@kubanek.net>
 */
class Reinhardt:public SensorWeather
{
	public:
		Reinhardt (int argc, char **argv);
		virtual ~Reinhardt ();

		virtual void addSelectSocks (fd_set &read_set, fd_set &write_set, fd_set &exp_set);
		virtual void selectSuccess (fd_set &read_set, fd_set &write_set, fd_set &exp_set);

	protected:
		virtual int processOption (int opt);
		virtual int initHardware ();
		virtual bool isGoodWeather ();
	private:
		char *device_file;
		rts2core::ConnSerial *reinhardtConn;

		rts2core::ValueDouble *temperature;
};

}

using namespace rts2sensord;

Reinhardt::Reinhardt (int argc, char **argv):SensorWeather (argc, argv)
{
	device_file = NULL;
	reinhardtConn = NULL;

	addOption ('f', NULL, 1, "serial port with the module (ussually /dev/ttyUSB for ThorLaser USB serial connection");
}

Reinhardt::~Reinhardt ()
{
	delete reinhardtConn;
}

void Reinhardt::addSelectSocks (fd_set &read_set, fd_set &write_set, fd_set &exp_set)
{
	reinhardtConn->add (&read_set, &write_set, &exp_set);
	SensorWeather::addSelectSocks (read_set, write_set, exp_set);
}

void Reinhardt::selectSuccess (fd_set &read_set, fd_set &write_set, fd_set &exp_set)
{
	if (reinhardtConn->receivedData (&read_set))
	{
		char data[200];
		int ret = reinhardtConn->readPort (data, 200, '\n');
		if (ret > 0)
		{
			data[ret] = '\0';
			std::cerr << "data " << data << std::endl;
		}
	}

	SensorWeather::selectSuccess (read_set, write_set, exp_set);
}

int Reinhardt::processOption (int opt)
{
	switch (opt)
	{
		case 'f':
			device_file = optarg;
			break;
		default:
			return SensorWeather::processOption (opt);
	}
	return 0;
}

int Reinhardt::initHardware ()
{
	if (device_file == NULL)
	{
		logStream (MESSAGE_ERROR) << "you must specify device file (TTY port)" << sendLog;
		return -1;
	}

	reinhardtConn = new rts2core::ConnSerial (device_file, this, rts2core::BS9600, rts2core::C8, rts2core::NONE, 30);
	int ret = reinhardtConn->init ();
	if (ret)
		return ret;
	reinhardtConn->setDebug (getDebug ());
	reinhardtConn->writePort ('?');
	return ret;
}

bool Reinhardt::isGoodWeather ()
{
	return SensorWeather::isGoodWeather ();
}

int main (int argc, char **argv)
{
	Reinhardt device (argc, argv);
	return device.run ();
}
