/*
 * Take an image, display it in X window (XImage).
 * Copyright (C) 2004-2007 Petr Kubanek <petr@kubanek.net>
 * Copyright (C) 2012 Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
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

#include "xfocusc.h"
#include "utilsfunc.h"
#include "xfitsimage.h"

#include <rts2-config.h>

#ifdef RTS2_HAVE_CURSES_H
#include <curses.h>
#elif defined(RTS2_HAVE_NCURSES_CURSES_H)
#include <ncurses/curses.h>
#endif

#include <math.h>

#include <iostream>
#include <iomanip>
#include <vector>
#include <stdlib.h>

#include <limits.h>

#define OPT_DISPLAY      OPT_LOCAL + 20
#define OPT_STARS        OPT_LOCAL + 21
#define OPT_SAVE         OPT_LOCAL + 22
#define OPT_CHANGE       OPT_LOCAL + 23
#define OPT_QUANTILES    OPT_LOCAL + 24
#define OPT_COLORMAP     OPT_LOCAL + 25

using namespace rts2core;

XFocusClientCamera::XFocusClientCamera (Connection * in_connection, double in_change_val, XFocusClient * in_master):FocusCameraClient (in_connection, in_master)
{
	master = in_master;

	change_val = in_change_val;

	quantiles = 0.05;
	colourVariant = PSEUDOCOLOUR_VARIANT_GREY;
}

XFocusClientCamera::~XFocusClientCamera ()
{
}

void XFocusClientCamera::postEvent (Event * event)
{
	switch (event->getType ())
	{
		case EVENT_XWIN_SOCK:
			{
				for (std::map <int, XFitsImage>::iterator iter = ximages.begin (); iter != ximages.end (); iter++)
					iter->second.XeventLoop ();
			}
			break;
	}
	FocusCameraClient::postEvent (event);
}

void XFocusClientCamera::cameraImageReady (rts2image::Image * image)
{
	for (int ch = 0; ch < image->getChannelSize (); ch++)
	{
		size_t channum = image->getChannelNumber (ch);
		std::map <int, XFitsImage>::iterator iter = ximages.find (channum);
		if (iter == ximages.end ())
		{
			ximages.insert (std::pair <int, XFitsImage> (channum, XFitsImage (getConnection (), this, quantiles, colourVariant)));
			iter = ximages.find (channum);
		}
		iter->second.drawImage (image, ch, master->getDisplay (), master->getVisual (), master->getDepth (), master->zoom, crossType, master->GoNine);
	}
}

void XFocusClientCamera::setCrossType (int in_crossType)
{
	crossType = in_crossType;
}

XFocusClient::XFocusClient (int in_argc, char **in_argv):FocusClient (in_argc, in_argv)
{
	displayName = NULL;

	crossType = 0;
	starsType = 0;

	changeVal = 15;
	zoom = 1.0;
	GoNine = false;

	quantiles = 0.05;
	colourVariant = PSEUDOCOLOUR_VARIANT_GREY;

	addOption (OPT_DISPLAY, "display", 1, "name of X display");
	addOption (OPT_STARS, "stars", 0, "draw stars over image (default to don't)");
	addOption ('x', NULL, 1, "cross type (default to 1; possible values 0 no cross, 1 rectangles 2 circles, 3 BOOTES special; 4 - crossboard");
	addOption (OPT_SAVE, "save", 0, "save filenames (default don't save");
	addOption (OPT_CHANGE, "change_val", 1, "change value (in arcseconds; default to 15 arcsec");
	addOption ('Z', NULL, 1, "Zoom (float number 0-xx)");
	addOption ('9', NULL, 0, "Nine sectors from different places of the CCD");

	addOption (OPT_QUANTILES, "quantiles", 1, "Display scaling quantiles");
	addOption (OPT_COLORMAP, "color", 1, "Image colormap");
}

XFocusClient::~XFocusClient ()
{
	if (display)
		XCloseDisplay (display);
}

void XFocusClient::usage ()
{
	std::cout
		<< "\t" << getAppName () << " -d C2 --nosync -e 10    .. takes 10 sec exposures on device C2. Exposures are not synchronize (blocked by unresposible devices,..)" << std::endl
		<< "\t" << getAppName () << " -d C0 -d C1 -e 20       .. takes 20 sec exposures on devices C0 and C1" << std::endl
		<< "\t" << getAppName () << " -d C2 -a -e 10          .. takes 10 sec exposures on device C2. Takes darks and use them" << std::endl
		<< "\t" << getAppName () << " -d C2 -a -e 10          .. takes 10 sec exposures on device C2. Takes darks and use them" << std::endl;
}

void XFocusClient::help ()
{
	Client::help ();
	std::cout << "Keys:" << std::endl
		<< "\t1,2,3 .. binning 1x1, 2x2, 3x3" << std::endl
		<< "\t9     .. split screen to squares containg corners of the image and its center" << std::endl
		<< "\tq,a   .. increase/decrease exposure 0.01 sec" << std::endl
		<< "\tw,s   .. increase/decrease exposure 0.1 sec" << std::endl
		<< "\te,d   .. increase/decrease exposure 1 sec" << std::endl
		<< "\tf     .. full frame exposure" << std::endl
		<< "\tc     .. center (1/2x1/2 chip size) exposure" << std::endl
		<< "\ty     .. save FITS file" << std::endl
		<< "\tu     .. don't save fits file" << std::endl
		<< "\thjkl, arrows .. move (change mount position)" << std::endl
		<< "\t+-    .. change zoom" << std::endl;
}

int XFocusClient::processOption (int in_opt)
{
	switch (in_opt)
	{
		case OPT_SAVE:
			autoSave = 1;
			break;
		case OPT_DISPLAY:
			displayName = optarg;
			break;
		case OPT_STARS:
			starsType = 1;
			break;
		case 'X':
			crossType = atoi (optarg);
			break;
		case 'Z':
			zoom = atof (optarg);
			if (zoom < 0.0625)
				zoom = 0.0625;
			if (zoom > 16)
				zoom = 16;
			break;
		case '9':
			GoNine = true;
			break;
		case OPT_CHANGE:
			changeVal = atof (optarg);
			break;
		case OPT_QUANTILES:
			quantiles = atof (optarg);
			break;
		case OPT_COLORMAP:
			colourVariant = atoi (optarg);
			break;
		default:
			return FocusClient::processOption (in_opt);
	}
	return 0;
}

int XFocusClient::init ()
{
	int ret;
	ret = FocusClient::init ();
	if (ret)
		return ret;

	// convert to degrees
	changeVal /= 3600.0;

	display = XOpenDisplay (displayName);
	if (!display)
	{
		std::cerr << "Cannot open display" << std::endl;
		return -1;
	}

	depth = DefaultDepth (display, DefaultScreen (display));
	visual = DefaultVisual (display, DefaultScreen (display));

	std::cout << "Display opened succesfully" << std::endl;

	setTimeout (USEC_SEC);

	return 0;
}

FocusCameraClient * XFocusClient::createFocCamera (Connection * conn)
{
	XFocusClientCamera *cam;
	cam = new XFocusClientCamera (conn, changeVal, this);
	cam->setCrossType (crossType);
	cam->quantiles = quantiles;
	cam->colourVariant = colourVariant;
	return cam;
}

void XFocusClient::addPollSocks ()
{
	FocusClient::addPollSocks ();
	addPollFD (ConnectionNumber (display), POLLIN | POLLPRI);
}

void XFocusClient::pollSuccess ()
{
	if (isForRead (ConnectionNumber (display)))
	{
		postEvent (new Event (EVENT_XWIN_SOCK));
	}
	FocusClient::pollSuccess ();
}

int main (int argc, char **argv)
{
	XFocusClient masterFocus = XFocusClient (argc, argv);

	logStream (MESSAGE_DEBUG) << "creating image " << NULL << sendLog;

	return masterFocus.run ();
}
