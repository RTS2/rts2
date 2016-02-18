/* 
 * Starlight Express CCD driver.
 * Copyright (C) 2016 Petr Kubanek <petr@kubanek.net>
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

#include "camd.h"
#include "sxccd/sxccdusb.h"

#include <libusb-1.0/libusb.h>

#define MAX_CAM   5

namespace rts2camd
{

/**
 * Class for Starlight Express camera.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class SX:public Camera
{
	public:
		SX (int in_argc, char **in_argv);
		virtual ~SX (void);

		virtual int processOption (int in_opt);
		virtual int initHardware ();
		virtual int initChips ();
		virtual int info ();
		virtual int startExposure ();
		virtual long isExposing ();
		virtual int doReadout ();

	private:
		const char *sxName;
		bool listNames;

		HANDLE sxHandle;
};

};

using namespace rts2camd;

SX::SX (int in_argc, char **in_argv):Camera (in_argc, in_argv)
{
	sxName = NULL;
	listNames = false;

	sxHandle = NULL;

	addOption ('n', NULL, 1, "camera name (for systems with multiple CCDs)");
	addOption ('l', NULL, 0, "list available camera names");
	
	createExpType ();
}

SX::~SX ()
{
	if (sxHandle != NULL)
		sxClose (&sxHandle);
}

int SX::processOption (int in_opt)
{
	return Camera::processOption (in_opt);
}

int SX::initHardware ()
{
	if (getDebug ())
		sxDebug (true);

	DEVICE devices[MAX_CAM];
	const char* names[MAX_CAM];

	int camNum = sxList (devices, names, MAX_CAM);
	if (camNum <= 0)
	{
		logStream (MESSAGE_ERROR) << "cannot find any SX camera" << sendLog;
		return -1;
	}

	if (listNames)
	{
		for (int i = 0; i < camNum; i++)
			std::cerr << "camera " << i << " name '" << names[i] << "'" << std::endl;
	}

	int cn = 0;

	if (sxName != NULL)
	{
		int i;
		for (i = 0; i < camNum; i++)
		{
			if (strcmp (names[i], sxName) == 0)
			{
				cn = i;
				break;
			}
		}
		if (i == camNum)
		{
			logStream (MESSAGE_ERROR) << "cannot find camera with name " << sxName << sendLog;
			return -1;
		}
	}

	int ret = sxOpen (devices[0], &sxHandle);
	if (ret)
		return -1;

	return 0;
}

int SX::initChips ()
{
}

int SX::info ()
{
}

int SX::startExposure ()
{
	return 0;
}

long SX::isExposing ()
{
	return 0;
}

int SX::doReadout ()
{
	return -2;
}

int main (int argc, char **argv)
{
	SX device (argc, argv);
	return device.run ();
}
