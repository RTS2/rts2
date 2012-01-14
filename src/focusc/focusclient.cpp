/* 
 * Generic class for focusing.
 * Copyright (C) 2005-2007 Petr Kubanek <petr@kubanek.net>
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

#include "focusclient.h"
#include "configuration.h"
#include "../../lib/rts2fits/memimage.h"

#include <iostream>
#include <iomanip>
#include <vector>

#define OPT_CHANGE_FILTER   OPT_LOCAL + 50
#define OPT_SKIP_FILTER     OPT_LOCAL + 51
#define OPT_PHOTOMETER_TIME OPT_LOCAL + 52
#define OPT_NOSYNC          OPT_LOCAL + 53
#define OPT_DARK            OPT_LOCAL + 54

#define CHECK_TIMER         0.1

FocusCameraClient::FocusCameraClient (rts2core::Connection * in_connection, FocusClient * in_master):rts2image::DevClientCameraFoc (in_connection, in_master->getExePath ())
{
	master = in_master;

	average = 0;

	low = med = hig = 0;

	bop = BOP_EXPOSURE;

	autoSave = master->getAutoSave ();
}


FocusCameraClient::~FocusCameraClient (void)
{
	std::list < fwhmData * >::iterator fwhm_iter;
	for (fwhm_iter = fwhmDatas.begin (); fwhm_iter != fwhmDatas.end (); fwhm_iter++)
	{
		fwhmData *dat;
		dat = *fwhm_iter;
		delete dat;
	}
	fwhmDatas.clear ();
}

void FocusCameraClient::exposureStarted ()
{
	if (exe == NULL)
	{
		queCommand (new rts2core::CommandExposure (getMaster (), this, bop));
	}
	rts2image::DevClientCameraFoc::exposureStarted ();
}

void FocusCameraClient::postEvent (Rts2Event *event)
{
	switch (event->getType ())
	{
		case EVENT_EXP_CHECK:
			if (getConnection ()->getState () & CAM_EXPOSING)
			{
				double fr = getConnection ()->getProgress (getMaster ()->getNow ());
				std::cout << "EXPOSING " << ProgressIndicator (fr) << std::fixed << std::setprecision (1) << std::setw (5) << fr << "% \r";
				std::cout.flush ();
			}
			break;
	}
	rts2image::DevClientCameraFoc::postEvent (event);
}

void FocusCameraClient::stateChanged (Rts2ServerState * state)
{
	std::cout << "State changed (" << getName () << "): "
		<< " value:" << getConnection()->getStateString ()
		<< " (" << state->getValue () << ")"
		<< std::endl;
	rts2image::DevClientCameraFoc::stateChanged (state);
}

rts2image::Image *FocusCameraClient::createImage (const struct timeval *expStart)
{
	rts2image::Image *image;
	if (autoSave)
	{
		image = rts2image::DevClientCameraFoc::createImage (expStart);
		image->keepImage ();
		return image;
	}
	if (exe)
	{
	  	std::ostringstream _os;
		_os << "!/tmp/" << connection->getName () << "_" << getpid () << ".fits";
		image = new rts2image::Image (expStart);
		image->openFile (_os.str ().c_str ());
		image->keepImage ();
		return image;
	}
	// memory-only image
	image = new rts2image::MemImage (expStart);
	return image;
}

rts2image::imageProceRes FocusCameraClient::processImage (rts2image::Image * image)
{
	rts2image::imageProceRes res = rts2image::DevClientCameraFoc::processImage (image);
	std::cout << "Camera " << getName () << " image_type:";
	switch (image->getShutter ())
	{
		case rts2image::SHUT_CLOSED:
			std::cout << "dark";
			break;
		case rts2image::SHUT_OPENED:
			std::cout << "object";
			break;
		default:
			std::cout << "unknow";
			break;
	}
	std::cout << std::endl;
	return res;
}

void FocusCameraClient::printFWHMTable ()
{
	std::list < fwhmData * >::iterator dat;
	std::cout << "=======================" << std::endl;
	std::cout << "# stars | focPos | fwhm" << std::endl;
	for (dat = fwhmDatas.begin (); dat != fwhmDatas.end (); dat++)
	{
		fwhmData *d;
		d = *dat;
		std::cout << std::setw (8) << d->num << "| "
			<< std::setw (7) << d->focPos << "| " << d->fwhm << std::endl;
	}
	std::cout << "=======================" << std::endl;
}

void FocusCameraClient::focusChange (rts2core::Connection * focus)
{
	if (getActualImage()->sexResultNum)
	{
		double fwhm;
		int focPos;
		fwhm = getActualImage ()->getFWHM ();
		focPos = getActualImage ()->getFocPos ();
		fwhmDatas.push_back (new fwhmData (getActualImage ()->sexResultNum, focPos, fwhm));
	}

	printFWHMTable ();

	// if we should query..
	if (master->getFocusingQuery ())
	{
		int change;
		int cons_change;
		char c[12];
		char *end_c;
		change = focConn->getChange ();
		if (change == INT_MAX)
		{
			std::
				cout << "Focusing algorithm for camera " << getName () <<
				" did not converge." << std::endl;
			std::
				cout << "Write new value, otherwise hit enter for no change " <<
				std::endl;
			change = 0;
		}
		else
		{
			std::cout << "Focusing algorithm for camera " << getName () <<
				" recommends to change focus by " << change << std::endl;
			std::cout << "Hit enter to confirm, or write new value." << std::
				endl;
		}
		std::cin.getline (c, 200);
		cons_change = strtol (c, &end_c, 10);
		if (end_c != c)
			change = cons_change;
		std::cout << std::endl;
		focConn->setChange (change);
		if (change != 0)
		{
			std::cout << "Will change by: " << change << std::endl;
		}
	}
	rts2image::DevClientCameraFoc::focusChange (focus);
	queCommand (new rts2core::CommandExposure (getMaster (), this, bop));
}

void FocusCameraClient::center (int centerWidth, int centerHeight)
{
	connection->queCommand (new rts2core::CommandCenter (this, centerWidth, centerHeight));
}

FocusClient::FocusClient (int in_argc, char **in_argv):rts2core::Client (in_argc, in_argv)
{
	defExposure = rts2_nan ("f");
	defCenter = 0;
	defBin = -1;

	xOffset = -1;
	yOffset = -1;
	imageWidth = -1;
	imageHeight = -1;

	autoSave = 0;

	darks = false;

	focExe = NULL;

	query = 0;
	tarRa = -999.0;
	tarDec = -999.0;

	photometerFile = NULL;
	photometerTime = 1;
	photometerFilterChange = 0;
	configFile = NULL;

	bop = BOP_EXPOSURE;

	addOption (OPT_CONFIG, "config", 1, "configuration file");

	addOption ('d', NULL, 1, "camera device name(s) (multiple for multiple cameras)");
	addOption ('e', NULL, 1, "exposure (defaults to 10 sec)");
	addOption (OPT_NOSYNC, "nosync", 0, "do not synchronize camera with telescope (don't block)");
	addOption (OPT_DARK, "dark", 0, "create dark images");
	addOption ('c', NULL, 0, "takes only center images");
	addOption ('b', NULL, 1, "default binning (ussually 1, depends on camera setting)");
	addOption ('Q', NULL, 0, "query after image end to user input (changing focusing etc..");
	addOption ('R', NULL, 1, "target ra (must come with dec - -D)");
	addOption ('D', NULL, 1, "target dec (must come with ra - -R)");
	addOption ('X', NULL, 1, "x pixel offset");
	addOption ('Y', NULL, 1, "y pixel offset");
	addOption ('W', NULL, 1, "image width");
	addOption ('H', NULL, 1, "image height");
	addOption ('F', NULL, 1, "image processing script (default to NULL - no image processing will be done");
	addOption ('o', NULL, 1, "save results to given file");
	addOption (OPT_PHOTOMETER_TIME, "photometer_time", 1, "photometer integration time (in seconds); default to 1 second");
	addOption (OPT_CHANGE_FILTER, "change_filter", 1, "change filter on photometer after taking n counts; default to 0 (don't change)");
	addOption (OPT_SKIP_FILTER, "skip_filter", 1, "Skip that filter number");
}

FocusClient::~FocusClient (void)
{

}

int FocusClient::processOption (int in_opt)
{
	switch (in_opt)
	{
		case OPT_CONFIG:
			configFile = optarg;
			break;
		case 'd':
			cameraNames.push_back (optarg);
			break;
		case 'e':
			defExposure = atof (optarg);
			break;
		case OPT_NOSYNC:
			bop = 0;
			break;
		case OPT_DARK:
			darks = true;
			break;
		case 'b':
			defBin = atoi (optarg);
			break;
		case 'Q':
			query = 1;
			break;
		case 'R':
			tarRa = atof (optarg);
			break;
		case 'D':
			tarDec = atof (optarg);
			break;
		case 'X':
			xOffset = atoi (optarg);
			break;
		case 'Y':
			yOffset = atoi (optarg);
			break;
		case 'W':
			imageWidth = atoi (optarg);
			break;
		case 'H':
			imageHeight = atoi (optarg);
			break;
		case 'c':
			defCenter = 1;
			break;
		case 'F':
			focExe = optarg;
			break;
		case 'o':
			photometerFile = optarg;
			break;
		case OPT_PHOTOMETER_TIME:
			photometerTime = atof (optarg);
			break;
		case OPT_CHANGE_FILTER:
			photometerFilterChange = atoi (optarg);
			break;
		case OPT_SKIP_FILTER:
			skipFilters.push_back (atoi (optarg));
			break;
		default:
			return rts2core::Client::processOption (in_opt);
	}
	return 0;
}

FocusCameraClient * FocusClient::createFocCamera (rts2core::Connection * conn)
{
	return new FocusCameraClient (conn, this);
}

FocusCameraClient *FocusClient::initFocCamera (FocusCameraClient * cam)
{
	std::vector < char *>::iterator cam_iter;
	cam->setSaveImage (autoSave || focExe);
	if (defCenter)
	{
		cam->center (imageWidth, imageHeight);
	}
	else if (xOffset >= 0 || yOffset >= 0 || imageWidth >= 0 || imageHeight >= 0)
	{
		cam->queCommand (new rts2core::CommandBox (cam, xOffset, yOffset, imageWidth, imageHeight));
	}
	if (!isnan (defExposure))
	{
		cam->queCommand (new rts2core::CommandChangeValue (cam, "exposure", '=', defExposure));

	}
	if (darks)
	{
		cam->queCommand (new rts2core::CommandChangeValue (cam, "SHUTTER", '=', 1));
	}
	if (defBin >= 0)
	{
		cam->queCommand (new rts2core::CommandChangeValue (cam, "binning", '=', defBin));
	}
	// post exposure event..if name agree
	for (cam_iter = cameraNames.begin (); cam_iter != cameraNames.end (); cam_iter++)
	{
		if (!strcmp (*cam_iter, cam->getName ()))
		{
			printf ("Get conn: %s\n", cam->getName ());
			cam->queCommand (new rts2core::CommandExposure (this, cam, bop));
		}
	}
	return cam;
}

rts2core::DevClient *FocusClient::createOtherType (rts2core::Connection * conn, int other_device_type)
{
	switch (other_device_type)
	{
		case DEVICE_TYPE_CCD:
			FocusCameraClient * cam;
			cam = createFocCamera (conn);
			return initFocCamera (cam);
		case DEVICE_TYPE_MOUNT:
			return new rts2image::DevClientTelescopeImage (conn);
		case DEVICE_TYPE_FOCUS:
			return new rts2image::DevClientFocusFoc (conn);
		case DEVICE_TYPE_PHOT:
			return new rts2image::DevClientPhotFoc (conn, photometerFile, photometerTime, photometerFilterChange, skipFilters);
		case DEVICE_TYPE_DOME:
		case DEVICE_TYPE_MIRROR:
		case DEVICE_TYPE_SENSOR:
			return new rts2image::DevClientWriteImage (conn);
		default:
			return rts2core::Client::createOtherType (conn, other_device_type);
	}
}

int FocusClient::init ()
{
	rts2core::Configuration *config;
	int ret;

	ret = rts2core::Client::init ();
	if (ret)
		return ret;

	config = rts2core::Configuration::instance ();
	ret = config->loadFile (configFile);
	if (ret)
	{
		std::cerr << "Cannot load configuration file '"
			<< (configFile ? configFile : "/etc/rts2/rts2.ini")
			<< ")" << std::endl;
		return ret;
	}
	addTimer (CHECK_TIMER, new Rts2Event (EVENT_EXP_CHECK));
	return 0;
}

void FocusClient::postEvent (Rts2Event *event)
{
	switch (event->getType ())
	{
		case EVENT_EXP_CHECK:
			addTimer (CHECK_TIMER, new Rts2Event (event));
			break;
	}
	rts2core::Client::postEvent (event);
}
