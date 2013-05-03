/**
 * Copyright (C) 2005-2008 Petr Kubanek <petr@kubanek.net>
 * Copyright (C) 2005-2007 Stanislav Vitek
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

namespace rts2focusd
{

/**
 * Class for Optec focuser.
 *
 * http://www.optecinc.com/astronomy/catalog/tcf/images/17670_manual.pdf
 *
 * @author Petr Kubanek <petr@kubanek.net>
 * @author Stanislav Vitek
 */
class Optec:public Focusd
{
	public:
		Optec (int argc, char **argv);
		~Optec (void);

		virtual int commandAuthorized (rts2core::Connection * conn);

	protected:
		virtual int info ();
		virtual int setTo (double num);
		virtual double tcOffset () {return 0.;};
		virtual int isFocusing ();

		virtual bool isAtStartPosition ();
		virtual int processOption (int in_opt);
		virtual int initHardware ();
		virtual int initValues ();
	private:
		const char *device_file;
		rts2core::ConnSerial *optecConn;
		
		int status;
		bool damagedTempSens;

		// high-level I/O functions
		int getPos ();
		int getTemp ();

};

};

using namespace rts2focusd;

Optec::Optec (int argc, char **argv):Focusd (argc, argv)
{
	device_file = FOCUSER_PORT;
	damagedTempSens = false;

	addOption ('f', "device_file", 1, "device file (ussualy /dev/ttySx");
	addOption ('D', "damaged_temp_sensor", 0,
		"if focuser have damaged temp sensor");
}

Optec::~Optec ()
{
	delete optecConn;
}

int Optec::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'f':
			device_file = optarg;
			break;
		case 'D':
			damagedTempSens = true;
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
int Optec::initHardware ()
{
	char rbuf[10];
	int ret;

	if (!damagedTempSens)
	{
		createTemperature ();
	}

	optecConn = new rts2core::ConnSerial (device_file, this, rts2core::BS19200, rts2core::C8, rts2core::NONE, 40);
	optecConn->setDebug (true);
	ret = optecConn->init ();
	if (ret)
		return ret;

	optecConn->flushPortIO ();

	// set manual
	if (optecConn->writeRead ("FMMODE", 6, rbuf, 10, '\r') < 0)
		return -1;
	if (rbuf[0] != '!')
		return -1;

	return ret;
}

int Optec::commandAuthorized (rts2core::Connection * conn)
{
	if (conn->isCommand ("home"))
	{
		char rbuf[10];
		if (optecConn->writeRead ("FHOME", 5, rbuf, 10, '\r') < 0)
		{
			conn->sendCommandEnd (DEVDEM_E_HW, "cannot home focuser");
			return -1;
		}
		return 0;
	}
	return Focusd::commandAuthorized (conn);
}

int Optec::getPos ()
{
	char rbuf[9];

	if (optecConn->writeRead ("FPOSRO", 6, rbuf, 8, '\r') < 1)
	{
		optecConn->flushPortIO ();
		return -1;
	}
	else
	{
		rbuf[6] = '\0';
		#ifdef DEBUG_EXTRA
		logStream (MESSAGE_DEBUG) << "0: " << rbuf[0] << sendLog;
		#endif
		position->setValueInteger (atoi ((rbuf + 2)));
	}
	return 0;
}

int Optec::getTemp ()
{
	char rbuf[10];

	if (damagedTempSens)
		return 0;

	if (optecConn->writeRead ("FTMPRO", 6, rbuf, 9, '\r') < 1)
	{
		return -1;
	}
	else
	{
		rbuf[7] = '\0';
		temperature->setValueFloat (atof ((rbuf + 2)));
	}
	return 0;
}

bool Optec::isAtStartPosition ()
{
	int ret;
	ret = getPos ();
	if (ret)
		return false;
	return (getPosition () == 3500);
}

int Optec::initValues ()
{
	focType = std::string ("OPTEC_TCF");
	return Focusd::initValues ();
}

int Optec::info ()
{
	int ret;
	ret = getPos ();
	if (ret)
		return ret;
	ret = getTemp ();
	if (ret)
		return ret;
	return Focusd::info ();
}

int Optec::setTo (double num)
{
	char command[7], rbuf[7];
	char add = ' ';
	int ret;

	ret = getPos ();
	if (ret)
		return ret;

	if (num > 7000 || num < 0)
		return -1;

	if (position->getValueInteger () > num)
	{
		add = 'I';
		num = position->getValueInteger () - num;
	}
	else
	{
		add = 'O';
		num = num - position->getValueInteger ();
	}

	// maximal time fore move is +- 40 sec

	sprintf (command, "F%c%04d", add, (int)num);

	optecConn->setVTime (400);

	ret = optecConn->writeRead (command, 6, rbuf, 6, '\r');

	optecConn->setVTime (40);

	if (ret < 0)
		return -1;

	if (rbuf[0] != '*')
		return -1;

	return 0;
}

int Optec::isFocusing ()
{
	// stepout command waits till focusing end
	return -2;
}

int main (int argc, char **argv)
{
	Optec device (argc, argv);
	return device.run ();
}
