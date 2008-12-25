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

#include "genfoc.h"
#include "../utils/rts2config.h"

#include <iostream>
#include <iomanip>
#include <vector>

#define OPT_CHANGE_FILTER   OPT_LOCAL + 50

Rts2GenFocCamera::Rts2GenFocCamera (Rts2Conn * in_connection, Rts2GenFocClient * in_master):
Rts2DevClientCameraFoc (in_connection, in_master->getExePath ())
{
	master = in_master;

	average = 0;
	stdev = 0;
	bg_stdev = 0;

	low = med = hig = 0;

	autoSave = master->getAutoSave ();
}


Rts2GenFocCamera::~Rts2GenFocCamera (void)
{
	std::list < fwhmData * >::iterator fwhm_iter;
	for (fwhm_iter = fwhmDatas.begin (); fwhm_iter != fwhmDatas.end ();
		fwhm_iter++)
	{
		fwhmData *dat;
		dat = *fwhm_iter;
		delete dat;
	}
	fwhmDatas.clear ();
}


void
Rts2GenFocCamera::getPriority ()
{
	std::cout << "Get priority " << getName () << std::endl;
}


void
Rts2GenFocCamera::lostPriority ()
{
	std::cout << "Priority lost " << getName () << std::endl;
}


void
Rts2GenFocCamera::exposureStarted ()
{
	if (exe == NULL)
	{
		queCommand (new Rts2CommandExposure (getMaster (), this, BOP_EXPOSURE));
	}
	Rts2DevClientCameraFoc::exposureStarted ();
}


void
Rts2GenFocCamera::stateChanged (Rts2ServerState * state)
{
	std::cout << "State changed (" << getName () << "): "
		<< " value:" << getConnection()->getStateString ()
		<< " (" << state->getValue () << ")"
		<< std::endl;
	Rts2DevClientCameraFoc::stateChanged (state);
}


Rts2Image *
Rts2GenFocCamera::createImage (const struct timeval *expStart)
{
	char *filename;
	Rts2Image *image;
	if (autoSave)
	{
		image = Rts2DevClientCameraFoc::createImage (expStart);
		image->keepImage ();
		return image;
	}
	if (exe)
	{
		asprintf (&filename, "!/tmp/%s_%i.fits", connection->getName (), getpid ());
		image = new Rts2Image (filename, expStart);
		image->keepImage ();
		free (filename);
		return image;
	}
	// memory-only image
	image = new Rts2Image (expStart);
	return image;
}


imageProceRes
Rts2GenFocCamera::processImage (Rts2Image * image)
{
	imageProceRes res = Rts2DevClientCameraFoc::processImage (image);
	std::cout << "Camera " << getName () << " image_type:";
	switch (image->getShutter ())
	{
		case SHUT_CLOSED:
			std::cout << "dark";
			break;
		case SHUT_OPENED:
			std::cout << "object";
			break;
		default:
			std::cout << "unknow";
			break;
	}
	std::cout << std::endl;
	return res;
}


void
Rts2GenFocCamera::printFWHMTable ()
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


void
Rts2GenFocCamera::focusChange (Rts2Conn * focus)
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
	Rts2DevClientCameraFoc::focusChange (focus);
	queCommand (new Rts2CommandExposure (getMaster (), this, BOP_EXPOSURE));
}


void
Rts2GenFocCamera::center (int centerWidth, int centerHeight)
{
	connection->queCommand (new Rts2CommandCenter (this, centerWidth, centerHeight));
}


Rts2GenFocClient::Rts2GenFocClient (int in_argc, char **in_argv):
Rts2Client (in_argc, in_argv)
{
	defExposure = nan("f");
	defCenter = 0;
	defBin = -1;

	centerWidth = -1;
	centerHeight = -1;

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

	addOption (OPT_CONFIG, "config", 1, "configuration file");

	addOption ('d', "device", 1,
		"camera device name(s) (multiple for multiple cameras)");
	addOption ('e', "exposure", 1, "exposure (defaults to 10 sec)");
	addOption ('a', "dark", 0, "create dark images");
	addOption ('c', "center", 0, "takes only center images");
	addOption ('b', "binning", 1,
		"default binning (ussually 1, depends on camera setting)");
	addOption ('Q', "query", 0,
		"query after image end to user input (changing focusing etc..");
	addOption ('R', "ra", 1, "target ra (must come with dec - -d)");
	addOption ('D', "dec", 1, "target dec (must come with ra - -r)");
	addOption ('W', "width", 1, "center width");
	addOption ('H', "height", 1, "center height");
	addOption ('F', "imageprocess", 1,
		"image processing script (default to NULL - no image processing will be done");
	addOption ('o', "output", 1, "save results to given file");
	addOption ('T', "photometer_time", 1,
		"photometer integration time (in seconds); default to 1 second");
	addOption (OPT_CHANGE_FILTER, "change_filter", 1,
		"change filter on photometer after taking n counts; default to 0 (don't change)");
	addOption ('K', "skip_filter", 1, "Skip that filter number");
}


Rts2GenFocClient::~Rts2GenFocClient (void)
{

}


int
Rts2GenFocClient::processOption (int in_opt)
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
		case 'a':
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
		case 'W':
			centerWidth = atoi (optarg);
			break;
		case 'H':
			centerHeight = atoi (optarg);
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
		case 'T':
			photometerTime = atof (optarg);
			break;
		case OPT_CHANGE_FILTER:
			photometerFilterChange = atoi (optarg);
			break;
		case 'K':
			skipFilters.push_back (atoi (optarg));
			break;
		default:
			return Rts2Client::processOption (in_opt);
	}
	return 0;
}


Rts2GenFocCamera *
Rts2GenFocClient::createFocCamera (Rts2Conn * conn)
{
	return new Rts2GenFocCamera (conn, this);
}


Rts2GenFocCamera *Rts2GenFocClient::initFocCamera (Rts2GenFocCamera * cam)
{
	std::vector < char *>::iterator cam_iter;
	cam->setSaveImage (autoSave || focExe);
	if (defCenter)
	{
		cam->center (centerWidth, centerHeight);
	}
	if (!isnan (defExposure))
	{
		cam->queCommand (new Rts2CommandChangeValue (cam, "exposure", '=', defExposure));

	}
	if (darks)
	{
		cam->queCommand (new Rts2CommandChangeValue (cam, "SHUTTER", '=', 1));
	}
	if (defBin >= 0)
	{
		cam->queCommand (new Rts2CommandChangeValue (cam, "binning", '=', defBin));
	}
	// post exposure event..if name agree
	for (cam_iter = cameraNames.begin (); cam_iter != cameraNames.end (); cam_iter++)
	{
		if (!strcmp (*cam_iter, cam->getName ()))
		{
			printf ("Get conn: %s\n", cam->getName ());
			cam->queCommand (new Rts2CommandExposure (this, cam, BOP_EXPOSURE));
		}
	}
	return cam;
}


Rts2DevClient *
Rts2GenFocClient::createOtherType (Rts2Conn * conn, int other_device_type)
{
	switch (other_device_type)
	{
		case DEVICE_TYPE_CCD:
			Rts2GenFocCamera * cam;
			cam = createFocCamera (conn);
			return initFocCamera (cam);
		case DEVICE_TYPE_MOUNT:
			return new Rts2DevClientTelescopeImage (conn);
		case DEVICE_TYPE_FOCUS:
			return new Rts2DevClientFocusFoc (conn);
		case DEVICE_TYPE_PHOT:
			return new Rts2DevClientPhotFoc (conn, photometerFile, photometerTime,
				photometerFilterChange, skipFilters);
		case DEVICE_TYPE_DOME:
		case DEVICE_TYPE_MIRROR:
		case DEVICE_TYPE_SENSOR:
			return new Rts2DevClientWriteImage (conn);
		default:
			return Rts2Client::createOtherType (conn, other_device_type);
	}
}


int
Rts2GenFocClient::init ()
{
	Rts2Config *config;
	int ret;

	ret = Rts2Client::init ();
	if (ret)
		return ret;

	config = Rts2Config::instance ();
	ret = config->loadFile (configFile);
	if (ret)
	{
		std::cerr << "Cannot load configuration file '"
			<< (configFile ? configFile : "/etc/rts2/rts2.ini")
			<< ")" << std::endl;
		return ret;
	}
	return 0;
}


void
Rts2GenFocClient::centraldConnRunning (Rts2Conn * conn)
{
	Rts2Client::centraldConnRunning (conn);
	conn->queCommand (new Rts2Command (this, "priority 137"));
}
