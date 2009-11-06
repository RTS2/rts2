/* 
 * Astronomical Research Camera driver
 * Copyright (C) 2009 Petr Kubanek <petr@kubanek.net>
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
#include "CController/CController.h"

#define OPT_DSP     OPT_LOCAL + 42

namespace rts2camd
{

/**
 * Class for Astronomical Research Cameras (Leech controller) drivers.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Arc:public Camera
{
	public:
		Arc (int argc, char **argv);
		virtual ~Arc ();

		virtual int info ();

	protected:
		int processOption (int opt);
		int init ();

		virtual int startExposure ();
		virtual int doReadout ();

	private:
		arc::CController controller;
		long lDeviceNumber;

		int w;
		int h;

		Rts2ValueString *timFile;
};

}

using namespace rts2camd;

Arc::Arc (int argc, char **argv):Camera (argc, argv)
{
	lDeviceNumber = 0;

	createTempCCD ();
	createValue (timFile, "dsp_timing", "DSP timing file", false);

	addOption ('n', NULL, 1, "Device number (default 0)");
	addOption ('W', NULL, 1, "chip width - number of collumns");
	addOption ('H', NULL, 1, "chip height - number of rows/lines");
	addOption (OPT_DSP, "dsp-timing", 1, "DSP timing file");

	w = 1024;
	h = 1024;
}

Arc::~Arc ()
{
	controller.CloseDriver ();
}

int Arc::info ()
{
	try
	{
		tempCCD->setValueDouble (controller.GetArrayTemperature ());
	}
	catch (std::exception ex)
	{
		logStream (MESSAGE_ERROR) << "Failure while retrieving informations" << sendLog;
		return -1;
	}
	return Camera::info ();
}

int Arc::processOption (int opt)
{
	switch (opt)
	{
		case 'n':
			lDeviceNumber = atoi (optarg);
			break;
	        case 'H':
			h = atoi (optarg);
			break;
		case 'W':
			w = atoi (optarg);
			break;
		case OPT_DSP:
			timFile->setValueCharArr (optarg);
			break;
		default:
			return Camera::processOption (opt);
	}
	return 0;
}

int Arc::init ()
{
	int ret = Camera::init ();
	if (ret)
		return ret;
	try
	{
		controller.OpenDriver (0, h * w * sizeof (unsigned int16_t));
	        controller.SetupController( true,          // Reset Controller
	                                     true,          // Test Data Link ( TDL )
	                                     true,          // Power On
	                                     h,      // Image row size
	                                     w,      // Image col size
	                                     timFile->getValue ());    // DSP timing file
	}
	catch (std::exception ex)
	{
		logStream (MESSAGE_ERROR) << "Cannot initialize camera driver" << sendLog;
		controller.CloseDriver ();
		return -1;
	}
	return 0;
}

int Arc::startExposure ()
{
	return 0;
}

int Arc::doReadout ()
{
	return -2;
}

int main (int argc, char **argv)
{
	Arc device (argc, argv);
	return device.run ();
}
