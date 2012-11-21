/**
 * Copyright (C) 2005-2007 Petr Kubanek <petr@kubanek.net>
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

#include "focusd.h"

#include "libfli.h"

namespace rts2focusd
{

/**
 * FLI focuser driver. You will need FLIlib and option to ./configure
 * (--with-fli=<llibflidir>) to get that running. Please read ../../INSTALL.fli
 * for instructions.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Fli:public Focusd
{
	public:
		Fli (int argc, char **argv);
		virtual ~ Fli (void);

		virtual int commandAuthorized (rts2core::Connection * conn);

	protected:
		virtual int isFocusing ();
		virtual bool isAtStartPosition ();

		virtual int processOption (int in_opt);
		virtual int initHardware ();
		virtual int initValues ();
		virtual int info ();
		virtual int setTo (double num);
		virtual double tcOffset () { return 0.;};

	private:
		flidev_t dev;
		flidomain_t deviceDomain;
		const char *name;

		int fliDebug;
};

};

using namespace rts2focusd;

Fli::Fli (int argc, char **argv):Focusd (argc, argv)
{
	dev = -1;
	deviceDomain = FLIDEVICE_FOCUSER | FLIDOMAIN_USB;
	fliDebug = FLIDEBUG_NONE;
	name = NULL;

	addOption ('D', "domain", 1, "CCD Domain (default to USB; possible values: USB|LPT|SERIAL|INET)");
	addOption ('b', "fli_debug", 1, "FLI debug level (1, 2 or 3; 3 will print most error message to stdout)");
	addOption ('f', NULL, 1, "FLI device path");
}

Fli::~Fli (void)
{
	FLIClose (dev);
}

int Fli::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'D':
			deviceDomain = FLIDEVICE_FOCUSER;
			if (!strcasecmp ("USB", optarg))
				deviceDomain |= FLIDOMAIN_USB;
			else if (!strcasecmp ("LPT", optarg))
				deviceDomain |= FLIDOMAIN_PARALLEL_PORT;
			else if (!strcasecmp ("SERIAL", optarg))
				deviceDomain |= FLIDOMAIN_SERIAL;
			else if (!strcasecmp ("INET", optarg))
				deviceDomain |= FLIDOMAIN_INET;
			else if (!strcasecmp ("SERIAL_19200", optarg))
				deviceDomain |= FLIDOMAIN_SERIAL_19200;
			else if (!strcasecmp ("SERIAL_1200", optarg))
				deviceDomain |= FLIDOMAIN_SERIAL_1200;
			else
				return -1;
			break;
		case 'b':
			switch (atoi (optarg))
			{
				case 1:
					fliDebug = FLIDEBUG_FAIL;
					break;
				case 2:
					fliDebug = FLIDEBUG_FAIL | FLIDEBUG_WARN;
					break;
				case 3:
					fliDebug = FLIDEBUG_ALL;
					break;
			}
			break;
		case 'f':
			name = optarg;
			break;
		default:
			return Focusd::processOption (in_opt);
	}
	return 0;
}

int Fli::initHardware ()
{
	LIBFLIAPI ret;
	char **names;
	char *nam_sep;

	if (fliDebug)
		FLISetDebugLevel (NULL, FLIDEBUG_ALL);

	if (dev > 0)
	{
		FLIClose (dev);
		dev = -1;
	}

	if (name == NULL)
	{
		ret = FLIList (deviceDomain, &names);
		if (ret)
			return -1;

		if (names[0] == NULL)
		{
			logStream (MESSAGE_ERROR) << "Fli::init No device found!" << sendLog;
			return -1;
		}

		nam_sep = strchr (names[0], ';');
		if (nam_sep)
			*nam_sep = '\0';

		ret = FLIOpen (&dev, names[0], deviceDomain);
		FLIFreeList (names);
	}
	else
	{
		ret = FLIOpen (&dev, name, deviceDomain);
	}
	if (ret)
	{
		logStream (MESSAGE_ERROR) << "cannot open device " << (name == NULL ? names[0] : name) << ":" << strerror (errno) << sendLog;
		return -1;
	}

	if (ret)
		return -1;

	long extent;
	ret = FLIGetFocuserExtent (dev, &extent);
	if (ret)
		return -1;
	setFocusExtend (0, extent);

	if (!isnan (defaultPosition->getValueFloat ()))
	{
		float def = defaultPosition->getValueFloat ();
		if (def < 0)
		{
			ret = info ();
			if (ret)
				return -1;
			def = getPosition ();
		}
		// calibrate by moving to home position, then move to default position
		ret = FLIHomeFocuser (dev);
		if (ret)
		{
			logStream (MESSAGE_ERROR) << "Cannot home focuser, return value: " << ret << sendLog;
			return -1;
		}
		setPosition (defaultPosition->getValueInteger ());
	}
	else
	{
		long steps;
		ret = FLIGetStepperPosition (dev, &steps);
		if (ret)
		{
			logStream (MESSAGE_ERROR) << "cannot get focuser position during initialization: " << ret << sendLog;
			return -1;
		}
		if (steps == -1)
		{
			ret = FLIHomeFocuser (dev);
			if (ret)
			{
				logStream (MESSAGE_ERROR) << "cannot home focuser (in non-default init), return value: " << ret << sendLog;
				return -1;
			}
		}
	}

	return 0;
}

int Fli::initValues ()
{
	LIBFLIAPI ret;

	char ft[200];

	ret = FLIGetModel (dev, ft, 200);
	if (ret)
	{
		logStream (MESSAGE_ERROR) << "Cannot get focuser model, error: " << ret << sendLog;
		return -1;
	}
	focType = std::string (ft);

	rts2core::ValueString *sern = new rts2core::ValueString ("serial", "serial number", true);
	char serial[50];
	FLIGetSerialString (dev, serial, 50);
	sern->setValueCharArr (serial);
	addConstValue (sern);

	return Focusd::initValues ();
}

int Fli::info ()
{
	LIBFLIAPI ret;

	long steps;

	ret = FLIGetStepperPosition (dev, &steps);
	if (ret)
	{
		ret = initHardware ();
		if (ret)
			return -1;
		ret = FLIGetStepperPosition (dev, &steps);
		if (ret)
			return -1;
	}

	position->setValueInteger ((int) steps);

	return Focusd::info ();
}

int Fli::setTo (double num)
{
	LIBFLIAPI ret;
	ret = info ();
	if (ret)
		return ret;

	long s = num - position->getValueInteger ();

	ret = FLIStepMotorAsync (dev, s);
	if (ret)
		return -1;
	// wait while move starts..
	double timeout = getNow () + 2;
	do
	{
		ret = FLIGetStepsRemaining (dev, &s);
		if (ret)
			return -1;
		if (s != 0)
			return 0;

		ret = FLIGetStepperPosition (dev, &s);
		if (ret)
			return -1;
		if (s == num)
			return 0;
	} while (getNow () < timeout);
	
	logStream (MESSAGE_ERROR) << "timeout during moving focuser to " << num << ", actual position is " << s << sendLog;
	
	return -1;
}

int Fli::isFocusing ()
{
	LIBFLIAPI ret;
	long rem;

	ret = FLIGetStepsRemaining (dev, &rem);
	if (ret)
	{
		logStream (MESSAGE_ERROR) << "Cannot get FLI steps remaining, return value:" << ret << sendLog;
		return -1;
	}
	if (rem == 0)
		return -2;
	ret = FLIGetStepperPosition (dev, &rem);
	if (ret)
	{
		logStream (MESSAGE_ERROR) << "Cannot get focuser position during move: " << ret << sendLog;
		return -1;
	}
	position->setValueInteger ((int) rem);
	sendValueAll (position);
	return USEC_SEC / 100;
}

bool Fli::isAtStartPosition ()
{
	int ret;
	ret = info ();
	if (ret)
		return false;
	return getPosition () == defaultPosition->getValueFloat ();
}

int Fli::commandAuthorized (rts2core::Connection * conn)
{
	if (conn->isCommand ("home"))
	{
		LIBFLIAPI ret;
		ret = FLIHomeFocuser (dev);
		if (ret)
		{
			conn->sendCommandEnd (DEVDEM_E_HW, "cannot home focuser");
			return -1;
		}
		return 0;
	}
	return Focusd::commandAuthorized (conn);
}

int main (int argc, char **argv)
{
	Fli device = Fli (argc, argv);
	return device.run ();
}
