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
#include "../utils/rts2connserial.h"

namespace rts2focusd
{

/**
 * Class for Optec focuser.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 * @author Stanislav Vitek
 */
class Optec:public Focusd
{
	private:
		const char *device_file;
		Rts2ConnSerial *optecConn;
		
		int status;
		bool damagedTempSens;

		Rts2ValueFloat *focTemp;

		// high-level I/O functions
		int getPos (Rts2ValueInteger * position);
		int getTemp ();
	protected:
		virtual bool isAtStartPosition ();
	public:
		Optec (int argc, char **argv);
		~Optec (void);
		virtual int processOption (int in_opt);
		virtual int init ();
		virtual int initValues ();
		virtual int info ();
		virtual int stepOut (int num);
		virtual int isFocusing ();
};

};

using namespace rts2focusd;

Optec::Optec (int argc, char **argv):Focusd (argc, argv)
{
	device_file = FOCUSER_PORT;
	damagedTempSens = false;
	focTemp = NULL;

	addOption ('f', "device_file", 1, "device file (ussualy /dev/ttySx");
	addOption ('D', "damaged_temp_sensor", 0,
		"if focuser have damaged temp sensor");
}


Optec::~Optec ()
{
	delete optecConn;
}


int
Optec::processOption (int in_opt)
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
int
Optec::init ()
{
	char rbuf[10];
	int ret;

	ret = Focusd::init ();

	if (ret)
		return ret;

	if (!damagedTempSens)
	{
		createFocTemp ();
		createValue (focTemp, "FOC_TEMP", "focuser temperature");
	}

	optecConn = new Rts2ConnSerial (device_file, this, BS19200, C8, NONE, 40);
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

	ret = checkStartPosition ();

	return ret;
}


int
Optec::getPos (Rts2ValueInteger * position)
{
	char rbuf[9];

	if (optecConn->writeRead ("FPOSRO", 6, rbuf, 8, '\r') < 1)
	{
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


int
Optec::getTemp ()
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
		focTemp->setValueFloat (atof ((rbuf + 2)));
	}
	return 0;
}


bool Optec::isAtStartPosition ()
{
	int ret;
	ret = getPos (focPos);
	if (ret)
		return false;
	return (getFocPos () == 3500);
}


int
Optec::initValues ()
{
	focType = std::string ("OPTEC_TCF");
	return Focusd::initValues ();
}


int
Optec::info ()
{
	int ret;
	ret = getPos (focPos);
	if (ret)
		return ret;
	ret = getTemp ();
	if (ret)
		return ret;
	return Focusd::info ();
}


int
Optec::stepOut (int num)
{
	char command[7], rbuf[7];
	char add = ' ';
	int ret;

	ret = getPos (focPos);
	if (ret)
		return ret;

	if (getFocPos () + num > 7000 || getFocPos () + num < 0)
		return -1;

	if (num < 0)
	{
		add = 'I';
		num *= -1;
	}
	else
	{
		add = 'O';
	}

	// maximal time fore move is +- 40 sec

	sprintf (command, "F%c%04d", add, num);

	optecConn->setVTime (400);

	ret = optecConn->writeRead (command, 6, rbuf, 6, '\r');

	optecConn->setVTime (40);

	if (ret < 0)
		return -1;

	if (rbuf[0] != '*')
		return -1;

	return 0;
}


int
Optec::isFocusing ()
{
	// stepout command waits till focusing end
	return -2;
}


int
main (int argc, char **argv)
{
	Optec device = Optec (argc, argv);
	return device.run ();
}
