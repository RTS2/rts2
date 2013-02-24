/**
 * Copyright (C) 2005 Petr Kubanek <petr@kubanek.net>
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

#include "filterd.h"
#include "libfli.h"
#include "libfli-filter-focuser.h"

namespace rts2filterd
{

class Fli:public Filterd
{
	public:
		Fli (int in_argc, char **in_argv);
		virtual ~ Fli (void);
	protected:
		virtual int processOption (int in_opt);
		virtual int initHardware ();

		virtual void changeMasterState (int old_state, int new_state);

		virtual int getFilterNum (void);
		virtual int setFilterNum (int new_filter);

		virtual int homeFilter ();

	private:
		flidev_t dev;
		flidomain_t deviceDomain;
		const char *name;

		long filter_count;

		int fliDebug;
};

}

using namespace rts2filterd;

Fli::Fli (int in_argc, char **in_argv):Filterd (in_argc, in_argv)
{
	deviceDomain = FLIDEVICE_FILTERWHEEL | FLIDOMAIN_USB;
	fliDebug = FLIDEBUG_NONE;
	name = NULL;
	dev = -1;

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
			deviceDomain = FLIDEVICE_FILTERWHEEL;
			if (!strcasecmp ("USB", optarg))
				deviceDomain |= FLIDOMAIN_USB;
			else if (!strcasecmp ("LPT", optarg))
				deviceDomain |= FLIDOMAIN_PARALLEL_PORT;
			else if (!strcasecmp ("SERIAL", optarg))
				deviceDomain |= FLIDOMAIN_SERIAL;
			else if (!strcasecmp ("INET", optarg))
				deviceDomain |= FLIDOMAIN_INET;
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
				default:
					return -1;
			}
			break;
		case 'f':
			name = optarg;
			break;
		default:
			return Filterd::processOption (in_opt);
	}
	return 0;
}

int Fli::initHardware ()
{
	LIBFLIAPI ret;
	char **names;
	char *nam_sep;


	if (dev >= 0)
	{
		FLIClose (dev);
		dev = -1;
	}

	if (fliDebug)
		FLISetDebugLevel (NULL, fliDebug);
	
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
		dev = -1;
		return -1;
	}

	ret = FLIGetFilterCount (dev, &filter_count);
	if (ret)
		return -1;

	std::cerr << "filter count" << filter_count << std::endl;

	return 0;
}

void Fli::changeMasterState (int old_state, int new_state)
{
	if ((new_state & SERVERD_STATUS_MASK) == SERVERD_DAY)
		homeFilter ();
	Filterd::changeMasterState (old_state, new_state);
}

int Fli::getFilterNum (void)
{
	long ret_f = -1;
	LIBFLIAPI ret;

	if (dev < 0)
	{
		ret = initHardware ();
		if (ret)
			throw rts2core::Error ("cannot reinit hardware");
	}
	ret = FLIGetFilterPos (dev, &ret_f);
	if (ret || ret_f < 0)
	{
		FLIClose (dev);
		dev = -1;
		throw rts2core::Error ("invalid return");
	}
	return (int) ret_f;
}

int Fli::setFilterNum (int new_filter)
{
	LIBFLIAPI ret;
	if (new_filter < -1 || new_filter >= filter_count)
		return -1;

	if (dev < 0)
	{
		ret = initHardware ();
		if (ret)
			return -1;
	}

	ret = FLISetFilterPos (dev, new_filter);
	if (ret)
	{
		FLIClose (dev);
		dev = -1;
		return -1;
	}
	return Filterd::setFilterNum (new_filter);
}

int Fli::homeFilter ()
{
	return setFilterNum (FLI_FILTERPOSITION_HOME);
}

int main (int argc, char **argv)
{
	Fli device = Fli (argc, argv);
	return device.run ();
}
