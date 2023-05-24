/**
 * Copyright (C) 2005-2008 Petr Kubanek <petr@kubanek.net>
 * Copyright (C) 2005-2007 Stanislav Vitek
 * Copyright (C) 2021 Ronan Cunniffe
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

/**
 * This file is basically the Optec driver with little bits re-written for the astromechanics
 * adapter for Canon autofocus lenses.
 *
 */

// Higher numbers mean longer focal distance, and Astromech adapter initializes to 5k steps.
#define ASTROMECH_INIT_POS 5000

#include "focusd.h"
#include "connection/serial.h"

namespace rts2focusd
{

/**
 * Class for Astromechanics adapter for Canon EF (electronic - i.e. auto - focus) lenses.
 *
 * http://www.astromechanics.com
 *
 * @author Petr Kubanek <petr@kubanek.net>
 * @author Stanislav Vitek
 * @author Ronan Cunniffe
 */

class Astromech:public Focusd
{
	public:
		Astromech (int argc, char **argv);
		~Astromech (void);

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
		unsigned int nsteps;
		rts2core::ConnSerial *amConn;

		int status;

		// high-level I/O functions
		int updatePos ();
		int resetPos ();
};

};

using namespace rts2focusd;

Astromech::Astromech (int argc, char **argv):Focusd (argc, argv)
{
	device_file = "";
	nsteps = 32767;

	addOption ('f', "device_file", 1, "device file (usually /dev/ttyUSBx");
	addOption ('n', "nsteps", 1, "number of physical steps the focuser has");
}

Astromech::~Astromech ()
{
	delete amConn;
}

int Astromech::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'f':
			device_file = optarg;
			break;
		case 'n':
		{
			nsteps = atoi(optarg);	// *FIXME* conversion error check
			break;
		}
		default:
			return Focusd::processOption (in_opt);
	}
	return 0;
}

/*!
 * Init focuser, connect on given port, set manual regime
 *
 * @return 0 on succes, -1 & set errno otherwise
 */
int Astromech::initHardware ()
{
	int ret;

	amConn = new rts2core::ConnSerial (device_file, this, rts2core::BS38400, rts2core::C8, rts2core::NONE, 2);
	amConn->setDebug (true);
	ret = amConn->init ();
	if (ret)
		return ret;

	// setIdleInfoInterval (1);

	amConn->flushPortIO ();

	// Is this an astromech?  Ask for position....
	ret = updatePos();
	if (ret < 0)
	{
		logStream (MESSAGE_ERROR) << "Is device on " << device_file << " really an astromech?  Got a garbled response... quitting." << sendLog;
		return -1;
	}

	// when adapter/lens is powered on, it has *no* idea where it is, and reports 5k.
	// so if we see 5k.... sweep to min, then back to 5k
	if (getPosition() == ASTROMECH_INIT_POS || true)
	{
		if (resetPos() != 0)
		{
			logStream (MESSAGE_WARNING) << "Focuser zero position reset failed." << sendLog;
			return -1;
		}

		if (updatePos() != 0 || (int)getPosition() != 0)
		{
			logStream (MESSAGE_WARNING) << "Focuser not at zero after 2 secs." << sendLog;
			return -1;
		}
	}

	return 0;
}

int Astromech::commandAuthorized (rts2core::Connection * conn)
{
	if (conn->isCommand ("reset"))
	{
		return resetPos ();
	}

	return Focusd::commandAuthorized (conn);
}

int Astromech::updatePos ()
{
	int i;
	char rbuf[60];

	memset(rbuf, 0, 6);
	if (amConn->writeRead ("P#", 2, rbuf, 10, '#') < 1)
	{
		amConn->flushPortIO ();
		return -1;
	}
#ifdef DEBUG_EXTRA
	logStream (MESSAGE_DEBUG) << "0: " << rbuf[0] << sendLog;
#endif

	for (i = 0; i <= 50; i++)
	{
		if (rbuf[i] == 0)
		{
			i = 50;
			break;
		}

		if (rbuf[i] == '#')
			break;
	}

	if (i >= 50) {
		logStream (MESSAGE_WARNING) << "Garbled response? Expected n..n#, got " << rbuf << "." << sendLog;
		return -1;
	}

	rbuf[i] = '\0';

	int canon_value = atoi(rbuf);
	position->setValueInteger (canon_value);	//Convert Canon to canonical

	return 0;
}

int Astromech::initValues ()
{
	focType = std::string ("Astromechanics_Canon_EF_Adapter");
	return Focusd::initValues ();
}

int Astromech::info ()
{
	int ret;
	ret = updatePos ();
	if (ret)
		return ret;
	return Focusd::info ();
}

int Astromech::setTo (double num)
{
	char command[21];
	int ret;

	ret = updatePos ();
	if (ret)
		return ret;

	snprintf(command, 20, "M%d#", int(num));

	ret = amConn->writePort (command, strlen(command));

	if (ret < 0)
		return -1;

	sleep(2);

	return 0;
}

int Astromech::resetPos ()
{
	int reset_pos = 0x7fff;
	int zero_pos = 0;

	int pos = getPosition ();

	if (setTo(reset_pos) != 0)
	{
		return -1;
	}

	int iter = 100;

	while (--iter > 0)
	{
		if (updatePos () != 0 || getPosition() != reset_pos)
			usleep(USEC_SEC/10);
		else
			break;
	}

	if (setTo(zero_pos) != 0)
	{
		return -1;
	}

	iter = 100;
	while (--iter > 0)
	{
		if (updatePos () != 0 || getPosition() != zero_pos)
			usleep(USEC_SEC/10);
		else
			break;
	}

	return 0;
}

bool Astromech::isAtStartPosition ()
{
	int ret;

	ret = updatePos();
	if (ret)
		return false;		//Actually, we shouldn't say "no" when it's really "cpu on fire"

	return (getPosition() == ASTROMECH_INIT_POS);
}

int Astromech::isFocusing ()
{
	int pos0 = getPosition ();

	if (updatePos() != 0)
	{
		// It seems it does not respond until the refocusing is finished?..
		return 1;
	}

	if (getPosition() != pos0)
	{
		return 1;
	}

	return -2;
}

int main (int argc, char **argv)
{
	Astromech device (argc, argv);

	return device.run ();
}
