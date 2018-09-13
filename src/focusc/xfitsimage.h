/*
 * Display RTS2 (and FITS images) in separate window.
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

#ifndef __RTS2_XFITSIMAGE__
#define __RTS2_XFITSIMAGE__

#include "rts2fits/image.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/keysymdef.h>

class Marker
{
	public:
		Marker (int _x, int _y) { x = _x; y = _y; }
		int x;
		int y;
};

/**
 * Class holding a single channel image.
 *
 * @author Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
 */
class XFitsImage
{
	public:
		XFitsImage (rts2core::Connection *_connection, rts2core::DevClient *_client, float _quantiles = 0.005, int _colourVariant = PSEUDOCOLOUR_VARIANT_BLUE);
		virtual ~XFitsImage ();

		void setCrossType (int in_crossType);

		// handle X input
		void XeventLoop ();

		/**
		 * Draw RTS2 image.
		 *
		 * @param image    RTS2 image to draw
		 * @param channel  Image channel
		 * @param GoNine   Make 9-style matrix of the image (center + borders)
		 */
		void drawImage (rts2image::Image * image, int channel, Display * _display, Visual *_visual, int _depth, double zoom, int _crossType, bool GoNine);

		int getChannelNumber () { return channelnum; }

		void setColourVariant (int _i) { colourVariant = _i; }

	private:
		double classical_median (void *q, int16_t dataType, int n, double *sigma, double sf = 0.6745);

		rts2core::Connection *connection;
		rts2core::DevClient *client;

		// X11 stuff
		Window window;

		Display * display;
		Visual * visual;
		int depth;

		XColor rgb[260];		 // <= 255 - images, 256 - red line, 257 - green
		Colormap colormap;

		GC gc;
		XGCValues gvc;
		Pixmap pixmap;
		XImage *ximage;
		XSetWindowAttributes xswa;

		int windowHeight;
		int windowWidth;

		int pixmapHeight;
		int pixmapWidth;

		void buildWindow ();
		void rebuildWindow ();
		void redraw ();
		void drawCenterCross (int xc, int yc);
		void drawCross1 ();
		void drawCross2 ();
		void drawCross3 ();
		void drawCross4 ();
		void drawMarkers ();
		void drawStars (rts2image::Image * image);
		void printInfo ();
		void printMouse ();
		void redrawMouse ();

		int crossType;

		long lastX;
		long lastY;
		long lastSizeX;
		long lastSizeY;
		int binningsX;
		int binningsY;

		int mouseX;
		int mouseY;

		int buttonX;
		int buttonY;
		struct timeval buttonImageTime;

		struct timeval exposureStart;

		int mouseTelChange_x, mouseTelChange_y;

		bool lastImage;

		// image statistics
		int low, high, max, min;
		double median, average;

		// marked pixels
		std::list <Marker> markers;

		// image channel
		int channelnum;

		// colormap
		int colourVariant;

		// Quantiles
		float quantiles;
};

#endif // __RTS2_XFITSIMAGE__
