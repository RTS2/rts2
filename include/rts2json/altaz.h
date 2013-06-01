/* 
 * Alt-Az graph plotting capabilities.
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

#ifndef __RTS2_MAGICK_ALTAZ__
#define __RTS2_MAGICK_ALTAZ__

#include "rts2-config.h"

#if defined(RTS2_HAVE_LIBJPEG) && RTS2_HAVE_LIBJPEG == 1
#include <Magick++.h>
#include <libnova/libnova.h>

#define PLOT_TYPE_CROSS		1
#define PLOT_TYPE_POINT		2
#define PLOT_TYPE_CIRCLE	3
#define PLOT_TYPE_TELESCOPE	4

namespace rts2json
{

/**
 * Represents AltAz coordinates. Can be aplied ower exising image, or on a new image.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class AltAz
{
	public:
		AltAz (int w = 600, int h = 600) : image (Magick::Geometry (w, h), "white") { rotation = 180.0; mirror = true; setCenter (); }
		AltAz (Magick::Image &_image): image (_image) { rotation = 180.0; mirror = true; setCenter (); }

		/**
		 * Plot alt-az grid on associated image.
		 */
		void plotAltAzGrid (Magick::Color col = Magick::Color ("grey40"));

		/**
		 * Plot horizon boundaries on associated image.
		 */
		void plotAltAzHorizon (Magick::Color col = Magick::Color ("grey80"));

		void plotCross (struct ln_hrz_posn *hrz, const char* label = NULL, const char *color = "black") { plot (hrz, label, color, PLOT_TYPE_CROSS, 10.0); }

		void plot (struct ln_hrz_posn *hrz, const char* label = NULL, const char *color = "black", int type = PLOT_TYPE_CROSS, double size = 10.0);

		void write (Magick::Blob *blob, const char *type) { image.write (blob, type); }

		// angle of true north, in degrees. North is on bottom, if rotation = 0
		double rotation;
		// mirror of EW axis
		bool mirror;

	private:
		Magick::Image image;

		int c; // center position
		int l; // lenght

		void setCenter ();
};

}

#endif

#endif /* ! __RTS2_MAGICK_ALTAZ__ */
