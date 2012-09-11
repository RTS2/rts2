/* 
 * Plotting library.
 * Copyright (C) 2010 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_PLOT__
#define __RTS2_PLOT__

#include <rts2-config.h>

#ifdef RTS2_HAVE_LIBJPEG

#include <Magick++.h>
#include "rts2db/records.h"

namespace rts2xmlrpc
{

typedef enum {PLOTTYPE_AUTO, PLOTTYPE_LINE, PLOTTYPE_LINE_SHARP, PLOTTYPE_CROSS, PLOTTYPE_CIRCLES, PLOTTYPE_SQUARES, PLOTTYPE_FILL, PLOTTYPE_FILL_SHARP} PlotType;

/**
 * General graph class.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Plot
{
	public:
		Plot (int w = 800, int h = 600);

	protected:
		double scaleX;
		double scaleY;

		double min;
		double max;

		double from;
		double to;

		PlotType plotType;

		Magick::Geometry size;
		Magick::Image *image;

		void plotYGrid (int y);
		void plotXGrid (int x);

		void plotYDegrees ();
		void plotYDouble ();
		void plotYBoolean ();

		/**
		 * Plot shadow for Sun areas.
		 */
		void plotXSunAlt ();

		/**
		 * Draw time labels, plot X date grid.
		 *
		 * @param shadowSun      shadow sun altitude
		 * @param localdate      draw localdate labels
		 */
		void plotXDate (bool shadowSun = true, bool localdate = true);

		// height of axis in pixels
		int x_axis_height;
		int y_axis_width;
};

}

#endif /* RTS2_HAVE_LIBJPEG */

#endif /* !__RTS2_PLOT__ */
