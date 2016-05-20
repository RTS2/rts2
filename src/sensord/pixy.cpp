/* 
 * Class for PIXY lightening detector.
 * 
 * Copyright (C) 2007 Stanislav Vitek
 * Copyright (C) 2008 Petr Kubanek <petr@kubanek.net>
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

class Pixy;

/**
 * Sensor connection.
 *
 * As pixy sensor put character on port every second (=it is not polled), we
 * have to prepare special connection, which will handle processLine its own way..
 */
class Rts2ConnPixy:public rts2core::ConnSerial
{
	public:
		Rts2ConnPixy (const char *in_devName, Pixy * in_master, rts2core::bSpeedT in_baudSpeed = rts2core::BS9600, rts2core::cSizeT in_cSize = rts2core::C8, rts2core::parityT in_parity = rts2core::NONE, int in_vTime = 40);

	protected:
		virtual int receive (rts2core::Block *block);

	private:
		/**
		 * Last value from the connection.
		 */
		char lastVal;
};

/**
 * Class for lighting detector.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Pixy:public Sensor
{
	public:
		Pixy (int argc, char **argv);
		virtual ~Pixy (void);

		/**
		 * Called when PIXY receives some data..
		 */
		void pixyReceived (char lastVal);

	private:
		char *device_port;
		Rts2ConnPixy *pixyConn;

		rts2core::ValueInteger *lightening;

		virtual int processOption (int in_opt);
		virtual int init ();
		virtual int info ();
};

};

using namespace rts2sensord;

int Rts2ConnPixy::receive (rts2core::Block *block)
{
	if (sock < 0 || !block->isForRead (sock))
	 	return 0;
	// get one byte..
	if (readPort (lastVal) != 1)
		return 0;

	((Pixy *)getMaster ())->pixyReceived (lastVal);
		
	return 1;
}

Rts2ConnPixy::Rts2ConnPixy (const char *in_devName, Pixy * in_master, rts2core::bSpeedT in_baudSpeed, rts2core::cSizeT in_cSize, rts2core::parityT in_parity, int in_vTime):ConnSerial (in_devName, in_master, in_baudSpeed, in_cSize, in_parity, in_vTime)
{
}

int Pixy::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'f':
			device_port = optarg;
			break;
		default:
			return Sensor::processOption (in_opt);
	}
	return 0;
}

int Pixy::init ()
{
	int ret;
	ret = Sensor::init ();
	if (ret)
		return ret;
	
	pixyConn = new Rts2ConnPixy (device_port, this, rts2core::BS19200, rts2core::C8, rts2core::NONE, 20);
	addConnection (pixyConn);
	ret = pixyConn->init ();
	if (ret)
		return ret;
	return 0;
}

int Pixy::info ()
{
	// do not set infotime
	return 0;
}

Pixy::Pixy (int argc, char **argv):Sensor (argc, argv)
{
	pixyConn = NULL;

	createValue (lightening, "lightening_index", "0-9 value describing number of detected strokes", true);

	addOption ('f', NULL, 1, "device port (default to /dev/ttyS0");
}

Pixy::~Pixy (void)
{
}

void Pixy::pixyReceived (char lastVal)
{
	lightening->setValueInteger (lastVal - '0');
	sendValueAll (lightening);
	updateInfoTime ();
}

int main (int argc, char **argv)
{
	Pixy device = Pixy (argc, argv);
	return device.run ();
}
