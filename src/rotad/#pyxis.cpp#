/* 
 * Rotator for Optec Pyxis module.
 * Copyright (C) 2011 Petr Kubanek <petr@kubanek.net>
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

#include "rotad.h"
#include "connection/serial.h"

namespace rts2rotad
{

/**
 * Simple dummy rotator. Shows how to use rotad library.
 *
 * @author Petr Kubánek <petr@kubanek.net>
 */
class Pyxis:public Rotator
{
	public:
		Pyxis (int argc, char **argv);
		virtual ~Pyxis ();
	protected:
		virtual int processOption (int opt);
		virtual int init ();
		virtual int info ();

		virtual void setTarget (double tv);
		virtual long isRotating ();
	private:
		rts2core::ValueInteger *speed;

		const char *optecDevice;
		rts2core::ConnSerial *optecConn;

		char rbuf[1000];
};

}

using namespace rts2rotad;

Pyxis::Pyxis (int argc, char **argv):Rotator (argc, argv)
{
	optecDevice = "/dev/ttyS0";
	optecConn = NULL;

	createValue (speed, "speed", "focuser speed", false);

	addOption ('f', NULL, 1, "focuser serial port (default to /dev/ttyS0)");
}

Pyxis::~Pyxis ()
{
	delete optecConn;
}

int Pyxis::processOption (int opt)
{
	switch (opt)  
	{
		case 'f':
			optecDevice = optarg;
			break;
		default:
			return Rotator::processOption (opt);

	}
	return 0;
}

int Pyxis::init ()
{
	int ret;
	ret = Rotator::init ();
	if (ret)
		return ret;
	
	optecConn = new rts2core::ConnSerial (optecDevice, this, rts2core::BS19200, rts2core::C8, rts2core::NONE, 40);
	optecConn->setDebug (true);
	ret = optecConn->init ();
	if (ret)
		return ret;

	// check connection
	if (optecConn->writeRead ("CCLINK", 6, rbuf, 8, '\n') < 0)
		return -1;
	if (rbuf[0] != '!')
		return -1;

	return 0;
}

int Pyxis::info ()
{
	if ((getState () & ROT_MASK_ROTATING) != ROT_IDLE)
		return -1;
	int ret = optecConn->writeRead ("CGETPA", 6, rbuf, 1000, '\r');
	if (ret < 0)
		return -1;
	rbuf[ret - 2 ] = '\0';	
	setCurrentPosition (atoi (rbuf + ret - 5));
	return Rotator::info ();	
}

void Pyxis::setTarget (double vt)
{
	sprintf (rbuf, "CPA%03d", (int) ln_range_degrees (vt));
	if (optecConn->writePort (rbuf, 6) < 0)
		throw rts2core::Error ("cannot move to destination");
}

long Pyxis::isRotating ()
{
	ssize_t ret = optecConn->readPortNoBlock (rbuf, 49);
	if (ret < 0)
		return -1;
	if (ret > 0 && (rbuf[ret - 1] == 'F' || rbuf[ret-1] == '2'))
	{
		return -2;
	}
	// update information..
	setCurrentPosition (getCurrentPosition () + (double) ret / 128);
	return USEC_SEC / 10;
}

int main (int argc, char **argv)
{
	Pyxis device (argc, argv);
	return device.run ();
}
