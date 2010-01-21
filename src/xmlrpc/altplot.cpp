/* 
 * Altitude plotting library.
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

#include <time.h>

#include "altplot.h"

#ifdef HAVE_LIBJPEG

#include "../utils/expander.h"
#include "../utils/libnova_cpp.h"

#include "../utilsdb/target.h"

using namespace rts2xmlrpc;

AltPlot::AltPlot (int w, int h):Plot (w, h)
{
}

Magick::Image* AltPlot::getPlot (double _from, double _to, rts2db::TargetSet *tarset, Magick::Image* _image, PlotType _plotType, int linewidth, int shadow)
{
	from = _from;
	to = _to;
	plotType = _plotType;

	if (_image)
	{
		image = _image;
		size = image->size ();
	}
	else
	{
		delete image;

		image = new Magick::Image (size, "white");
	}
	image->strokeColor ("black");
	image->strokeWidth (1);

	// Y axis scaling
	min = 0;
	max = 90;

	scaleY = size.height () / (max - min);
	scaleX = size.width () / (to - from); 

	image->font("helvetica");
	image->strokeAntiAlias (true);

	// draw 0 line
	image->fontPointsize (15);
	image->draw (Magick::DrawableText (0, size.height () - (scaleY * -min) - 4, "0"));
	image->strokeWidth (3);
	plotYGrid (size.height () - (scaleY * -min));
			
	if (plotType == PLOTTYPE_AUTO)
		plotType = PLOTTYPE_LINE;

	plotYDegrees ();
	plotXDate ();

	for (rts2db::TargetSet::iterator iter = tarset->begin (); iter != tarset->end (); iter++)
	{
		if (shadow)
			plotTarget (iter->second, Magick::Color (MaxRGB / 5, MaxRGB / 5, MaxRGB / 5, 3 * MaxRGB / 5), linewidth, shadow);
		plotTarget (iter->second, Magick::Color (0, MaxRGB, 0, MaxRGB / 5), linewidth, 0);
	}

	return image;
}

void AltPlot::plotTarget (Target *tar, Magick::Color col, int linewidth, int shadow)
{
	// reset stroke pattern
	image->strokePattern (Magick::Image (Magick::Geometry (1,1), col));
	image->strokeColor (col);
	image->strokeWidth (linewidth);
	image->fillColor (col);

	struct ln_hrz_posn hrz;

	time_t t_from = (time_t) from;

	double JD = ln_get_julian_from_timet (&t_from);

	tar->getAltAz (&hrz, JD);

	double stepX = (to - from) / size.width () / 86400.0;

	double x = shadow;
	double x_end = x + 1;
	double y = size.height () - scaleY * (hrz.alt - min) + shadow;

	while (x < size.width ())
	{
		JD += stepX;
		tar->getAltAz (&hrz, JD);
		double y_end = size.height () - scaleY * (hrz.alt - min) + shadow;
		switch (plotType)
		{
			case PLOTTYPE_AUTO:
			case PLOTTYPE_LINE:
				image->draw (Magick::DrawableLine (x, y, x_end, y_end));
				break;
			case PLOTTYPE_LINE_SHARP:
				image->draw (Magick::DrawableLine (x, y, x_end, y));
				image->draw (Magick::DrawableLine (x_end, y, x_end, y_end));
				break;
			case PLOTTYPE_CROSS:
				image->draw (Magick::DrawableLine (x - 2, y, x + 2, y));
				image->draw (Magick::DrawableLine (x, y - 2, x, y + 2));
				break;
			case PLOTTYPE_CIRCLES:
				image->draw (Magick::DrawableCircle (x, y, x - 2, y));
				break;
			case PLOTTYPE_SQUARES:
				image->draw (Magick::DrawableRectangle (x - 1, y - 1, y + 1, x + 1));
				break;
			case PLOTTYPE_FILL:
			case PLOTTYPE_FILL_SHARP:
				std::list <Magick::Coordinate> pol;
				pol.push_back (Magick::Coordinate (x, size.height ()));
				pol.push_back (Magick::Coordinate (x, y));
				pol.push_back (Magick::Coordinate (x_end - 1, (plotType == PLOTTYPE_FILL_SHARP ? y : y_end)));
				pol.push_back (Magick::Coordinate (x_end - 1, size.height ()));
				image->draw (Magick::DrawablePolygon (pol));
		}
		x = x_end;
		x_end++;
		y = y_end;
	}
}

#endif /* HAVE_LIBJPEG */
