/*
 * Driver for Servo-drive.com controller.
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
 * Servo drive controller.
 * http://www.servo-drive.com/pdf_catalog/manual_r272.pdf
 * @author Petr Kubanek <petr@kubanek.net>
 */
class ServoDrive:public Sensor
{
	public:
		ServoDrive (int in_argc, char **in_argv);
		virtual ~ ServoDrive (void);

		virtual int initHardware ();
		virtual int info ();

		virtual int commandAuthorized (rts2core::Connection * conn);

	protected:
		virtual int setValue (rts2core::Value * old_value, rts2core::Value * new_value);
		virtual int processOption (int in_opt);

	private:
		rts2core::ConnSerial *servoDev;
		const char *dev;
};

};

using namespace rts2sensord;

ServoDrive::ServoDrive (int argc, char **argv):Sensor (argc, argv)
{
	servoDev = NULL;
	dev = "/dev/ttyS0";

	addOption ('f', NULL, 1, "/dev/ttySx entry (defaults to /dev/ttyS0");
}

ServoDrive::~ServoDrive (void)
{
	delete servoDev;
}

int ServoDrive::setValue (rts2core::Value * old_value, rts2core::Value * new_value)
{
	return Sensor::setValue (old_value, new_value);
}

int ServoDrive::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'f':
			dev = optarg;
			break;
		default:
			return Sensor::processOption (in_opt);
	}
	return 0;
}

int ServoDrive::initHardware ()
{
	servoDev = new rts2core::ConnSerial (dev, this, rts2core::BS9600, rts2core::C8, rts2core::NONE, 200);
	int ret = servoDev->init ();
	if (ret)
		return ret;
	servoDev->flushPortIO ();
	
	return 0;
}

int ServoDrive::info ()
{
	return Sensor::info ();
}

int ServoDrive::commandAuthorized (rts2core::Connection * conn)
{
	return Sensor::commandAuthorized (conn);
}

int main (int argc, char **argv)
{
	ServoDrive device (argc, argv);
	return device.run ();
}
