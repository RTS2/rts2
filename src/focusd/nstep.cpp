/*
 * Copyright (C) 2013 Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#define FOCUSER_PORT "/dev/ttyACM0"

#include "focusd.h"
#include "connection/serial.h"

namespace rts2focusd
{

/**
 * Based on documentation kindly provided by Gene Chimahusky (A.K.A Astro Gene).
 *
 * @author Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
 */
class NStep:public Focusd
{
	public:
		NStep (int argc, char **argv);
		~NStep (void);

		virtual int init ();
		virtual int initValues ();
		virtual int info ();
		virtual int setTo (double num);
  		virtual double tcOffset () {return 0.;};

		virtual int commandAuthorized (rts2core::Connection * conn);
	protected:
		virtual int processOption (int in_opt);
		virtual int isFocusing ();

		virtual bool isAtStartPosition ()
		{
			return false;
		}

	private:
		char buf[15];
		const char *device_file;
                rts2core::ConnSerial *NSConn; // communication port with microfocuser
};

}

using namespace rts2focusd;

NStep::NStep (int argc, char **argv):Focusd (argc, argv)
{
	device_file = FOCUSER_PORT;
	NSConn = NULL;

	addOption ('f', NULL, 1, "device file (ussualy /dev/ttyACMx");
}

NStep::~NStep ()
{
  	delete NSConn;
}

int NStep::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'f':
			device_file = optarg;
			break;
		default:
			return Focusd::processOption (in_opt);
	}
	return 0;
}

/*!
 * Init focuser, connect on given port port, set manual regime
 *
 * @return 0 on succes, -1 & set errno otherwise
 */
int NStep::init ()
{
	int ret;

	ret = Focusd::init ();

	if (ret)
		return ret;

	NSConn = new rts2core::ConnSerial (device_file, this, rts2core::BS9600, rts2core::C8, rts2core::NONE, 40);
	NSConn->setDebug (true);
	ret = NSConn->init ();
	if (ret)
		return ret;

	NSConn->flushPortIO ();

	ret = NSConn->writeRead ("#:RT", 4, buf, 4);
	if (ret != 4 || !(buf[0] == '+' || buf[0] == '-'))
	{
		logStream (MESSAGE_ERROR) << "cannot switch focuser to PC mode" << sendLog;
		return -1;
	}

	if (strncmp (buf, "-888", 4))
	{
		createTemperature ();
	}

	ret = info ();
	if (ret)
		return ret;

	setIdleInfoInterval (60);

	return 0;
}

int NStep::initValues ()
{
	focType = std::string ("GCUSB-nStep");
	return Focusd::initValues ();
}

int NStep::info ()
{
	int ret;
	if (temperature)
	{
		ret = NSConn->writeRead ("#:RT", 4, buf, 4);
		if (ret != 4)
		{
			logStream (MESSAGE_ERROR) << "cannot read temperature" << sendLog;
			return -1;
		}
		buf[4] = '\0';
		temperature->setValueFloat (atof (buf) / 10);
	}

	ret = NSConn->writeRead ("#:RP", 4, buf, 7);
	if (ret != 7)
	{
		logStream (MESSAGE_ERROR) << "cannot read current position" << sendLog;
		return -1;
	}
	buf[7] = '\0';

	position->setValueDouble (atoi (buf));

	return Focusd::info ();
}

int NStep::commandAuthorized (rts2core::Connection * conn)
{
	return Focusd::commandAuthorized (conn);
}

int NStep::setTo (double num)
{
	double diff = num - position->getValueDouble ();
	// ignore sub-step requests
	if (fabs (diff) < 1)
		return 0;

	// compare as well strings we will send..
	snprintf (buf, 9, ":F%c%d%03d#", (diff > 0) ? '1' : '0', 1, (int) (fabs (diff)));

	if (NSConn->writePort (buf, 8))
		return -1;
	return 0;
}

int NStep::isFocusing ()
{
	int ret = info ();
	if (ret)
		return -1;
	sendValueAll (position);

	ret = NSConn->writeRead ("S", 1, buf, 1);
	if (ret != 1)
		return -1;
	if (buf[0] == '1')
		return USEC_SEC;
	return -2;
}

int main (int argc, char **argv)
{
	NStep device (argc, argv);
	return device.run ();
}
