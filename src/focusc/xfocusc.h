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

#ifndef __RTS2_XFOCUSC__
#define __RTS2_XFOCUSC__

#include "focusclient.h"

class XFitsImage;

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/keysymdef.h>

/**
 * X-focusing camera client class.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class XFocusClient:public FocusClient
{
	public:
		XFocusClient (int argc, char **argv);
		virtual ~XFocusClient ();

		virtual int processOption (int in_opt);

		virtual int init ();

		/**
		 * Add XWin connection socket, obtained by ConnectionNumber macro.
		 */
		virtual void addPollSocks ();

		/**
		 * Query and process possible XWin event from top of XWin event loop.
		 */
		virtual void pollSuccess ();

		virtual void usage ();
		virtual void help ();

		Display *getDisplay ()
		{
			return display;
		}
		int getDepth ()
		{
			return depth;
		}
		Visual * getVisual ()
		{
			return visual;
		}

		int getStarsType ()
		{
			return starsType;
		}

		double zoom;
		bool GoNine;

		float quantiles;
		int colourVariant;

	private:
		char * displayName;

		// X11 stuff
		Display * display;
		Visual * visual;
		int depth;

		int crossType;
		int starsType;

		// initially in arcsec, but converted (and used) in degrees
		double changeVal;

		virtual FocusCameraClient * createFocCamera (rts2core::Connection * conn);
};

class XFocusClientCamera:public FocusCameraClient
{
	public:
		XFocusClientCamera (rts2core::Connection * in_connection, double in_change_val, XFocusClient * in_master);
		virtual ~XFocusClientCamera ();

		int getCrossType () { return crossType; }
		void setCrossType (int in_crossType);

		virtual void postEvent (rts2core::Event * event);

		float quantiles;
		int colourVariant;

		XFocusClient * getMaster () { return master; }

	protected:
		virtual void cameraImageReady (rts2image::Image * image);

	private:
		XFocusClient * master;
		// key is channel number
		std::map <int, XFitsImage> ximages;

		int lastImage;

		double change_val;		 // change value in degrees

		int crossType;
};

#endif // __RTS2_XFOCUSC__
