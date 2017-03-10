/*
 * Copyright (C) 2011 Petr Kubanek <petr@kubanek.net>
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

#define FOCUSER_PORT "/dev/ttyS0"

#include "focusd.h"
#include "connection/serial.h"

// Microfocuser commands:
//     FMMODE    !                open PC connection
//     FFMODE    END              end PC connection
//     FInnnnn    * | E | W | S    move in
//     FOnnnnn    * | E | W | S    move out
//     FPOSRO    P=nnnnn          report position
//     FTMPRO    T=+-nn.n, F=+nnn report temperature
//     FCENTR    nn/mm            recalibrate
//     FJMODE    D                operation mode
namespace rts2focusd
{

class Microfocuser:public Focusd
{
	public:
		Microfocuser (int argc, char **argv);
		~Microfocuser (void);

		virtual int init ();
		virtual int initValues ();
		virtual int info ();
		virtual int setTo (double num);
  		virtual double tcOffset () {return 0.;};

		virtual int commandAuthorized (rts2core::Rts2Connection * conn);
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
                rts2core::ConnSerial *MConn; // communication port with microfocuser
};

}

using namespace rts2focusd;

Microfocuser::Microfocuser (int argc, char **argv):Focusd (argc, argv)
{
	device_file = FOCUSER_PORT;
	MConn = NULL;

	createTemperature (); // ??

	addOption ('f', NULL, 1, "device file (ussualy /dev/ttySx");
}

Microfocuser::~Microfocuser ()
{
	MConn->writeRead ("FFMODE", 6, buf, 1);
  	delete MConn;
}

int Microfocuser::processOption (int in_opt)
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
int Microfocuser::init ()
{
	int ret;

	ret = Focusd::init ();

	if (ret)
		return ret;

	MConn = new rts2core::ConnSerial (device_file, this, rts2core::BS19200, rts2core::C8, rts2core::NONE, 100);
	MConn->setDebug (true);
	ret = MConn->init ();
	if (ret)
		return ret;

	MConn->flushPortIO ();

	ret = MConn->writeRead ("FMMODE", 6, buf, 1);
	if (ret != 1 || buf[0] != '!')
	{
		logStream (MESSAGE_ERROR) << "cannot switch focuser to PC mode" << sendLog;
		return -1;
	}

	ret = info ();
	if (ret)
		return ret;

	setIdleInfoInterval (40);

	return 0;
}

int Microfocuser::initValues ()
{
	focType = std::string ("MICROFOCUS");
	return Focusd::initValues ();
}

int Microfocuser::info ()
{
	int ret;
	ret = MConn->writeRead ("FPOSRO", 6, buf, 7);
	if (ret != 7)
		return ret;

	position->setValueDouble (atoi (buf + 2));

	usleep (USEC_SEC);

	return Focusd::info ();
}

int Microfocuser::commandAuthorized (rts2core::Rts2Connection * conn)
{
	if (conn->isCommand ("home"))
	{
		int ret = MConn->writeRead ("FCENTR", 6, buf, 5);
		return ret != 5 ? -2 : 0;
	}
	return Focusd::commandAuthorized (conn);
}

int Microfocuser::setTo (double num)
{
	double diff = num - position->getValueDouble ();
	// ignore sub-step requests
	if (fabs (diff) < 1)
		return 0;

	// compare as well strings we will send..
	snprintf (buf, 8, "F%c%05d", (diff > 0) ? 'O' : 'I', (int) (fabs (diff)));

	if (MConn->writePort (buf, 7))
		return -1;
	MConn->setVTime (1000);
	if (MConn->readPort (buf, 1) != 1)
		return -1;
	MConn->setVTime (100);
	return 0;
}

int Microfocuser::isFocusing ()
{
	int ret = info ();
	if (ret)
		return -1;
	if (target->getValueInteger () - position->getValueInteger () == 0)
		return -2;
	sendValueAll (position);
	return USEC_SEC;
}

int main (int argc, char **argv)
{
	Microfocuser device (argc, argv);
	return device.run ();
}
