/* 
 * Variable plotting library.
 * Copyright (C) 2009 Petr Kubanek <petr@kubanek.net>
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

#include <time.h>

#include "valueplot.h"

#ifdef RTS2_HAVE_LIBJPEG

#include "expander.h"
#include "libnova_cpp.h"

using namespace rts2xmlrpc;

ValuePlot::ValuePlot (int _recvalId, int _valType, int w, int h):Plot (w, h)
{
	recvalId = _recvalId;
	valueType = _valType;
}

Magick::Image* ValuePlot::getPlot (double _from, double _to, Magick::Image* _image, rts2json::PlotType _plotType, int linewidth, int shadow, bool plotSun, bool plotShadow, bool localDate)
{
	// first load values..
	rts2db::RecordsSet rs (recvalId);

	from = _from;
	to = _to;
	plotType = _plotType;

	rs.load (from, to);

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
	min = rs.getMin ();
	max = rs.getMax ();

	if (min == max)
	{
		min -= 0.1;
		max += 0.1;
	}

	scaleY = (size.height () - x_axis_height) / (max - min);
	scaleX = (size.width () - y_axis_width) / (to - from); 

	image->font("helvetica");
	image->strokeAntiAlias (true);

	if (plotSun)
		plotXSunAlt ();

	switch (valueType & RTS2_BASE_TYPE)
	{
		case RTS2_VALUE_BOOL:
			min = -0.1;
			max = 1.1;
			if (plotType == rts2json::PLOTTYPE_AUTO)
				plotType = rts2json::PLOTTYPE_FILL_SHARP;
			plotYBoolean ();
			break;
		case RTS2_VALUE_DOUBLE:
		default:
			if (max > 0 && min < 0)
			{
				// draw 0 line
				image->fontPointsize (15);
				image->draw (Magick::DrawableText (0, size.height () - x_axis_height - (scaleY * -min) - 4, "0"));
				image->strokeWidth (3);
				plotYGrid (size.height () - x_axis_height - (scaleY * -min));
			}
			if (plotType == rts2json::PLOTTYPE_AUTO)
				plotType = rts2json::PLOTTYPE_LINE;
			switch (valueType & RTS2_TYPE_MASK)
			{
				case RTS2_DT_RA:
				case RTS2_DT_DEC:
				case RTS2_DT_DEGREES:
					plotYDegrees ();
					break;
				default:
					plotYDouble ();
					break;
			}
			break;
	}

	plotXDate (plotShadow, localDate);

	if (!rs.empty ())
	{
		if (shadow)
			plotData (rs, Magick::Color (MaxRGB / 5, MaxRGB / 5, MaxRGB / 5, 3 * MaxRGB / 5), linewidth, shadow);
		plotData (rs, Magick::Color (0, MaxRGB, 0, MaxRGB / 5), linewidth, 0);
	}

	return image;
}

void ValuePlot::plotData (rts2db::RecordsSet &rs, Magick::Color col, int linewidth, int shadow)
{
	// reset stroke pattern
	image->strokePattern (Magick::Image (Magick::Geometry (1,1), col));
	image->strokeColor (col);
	image->strokeWidth (linewidth);
	image->fillColor (col);

	rts2db::RecordsSet::iterator iter = rs.begin ();

	double x = scaleX * (iter->getRecTime () - from) + shadow;
	double y = size.height () - scaleY * (iter->getValue () - min) + shadow;
	while (iter != rs.end () && (isnan (x) || isnan (y)))
	{
		iter++;
		x = y_axis_width + scaleX * (iter->getRecTime () - from) + shadow;
		y = size.height () - x_axis_height - scaleY * (iter->getValue () - min) + shadow;
	}

	for (; iter != rs.end (); )
	{
		iter++;

		double x_end;
		double y_end;

		if (iter == rs.end ())
		{
			x_end = x + 1;
			y_end = y;
		}
		else
		{
			x_end = y_axis_width + scaleX * (iter->getRecTime () - from) + shadow;
			y_end = size.height () - x_axis_height - scaleY * (iter->getValue () - min) + shadow;
		}
		// don't accept nan values
		if (isnan (x_end) || isnan (y_end))
			continue;

		switch (plotType)
		{
			case rts2json::PLOTTYPE_AUTO:
			case rts2json::PLOTTYPE_LINE:
				image->draw (Magick::DrawableLine (x, y, x_end, y_end));
				break;
			case rts2json::PLOTTYPE_LINE_SHARP:
				image->draw (Magick::DrawableLine (x, y, x_end, y));
				image->draw (Magick::DrawableLine (x_end, y, x_end, y_end));
				break;
			case rts2json::PLOTTYPE_CROSS:
				image->draw (Magick::DrawableLine (x - 2, y, x + 2, y));
				image->draw (Magick::DrawableLine (x, y - 2, x, y + 2));
				break;
			case rts2json::PLOTTYPE_CIRCLES:
				image->draw (Magick::DrawableCircle (x, y, x - 2, y));
				break;
			case rts2json::PLOTTYPE_SQUARES:
				image->draw (Magick::DrawableRectangle (x - 1, y - 1, y + 1, x + 1));
				break;
			case rts2json::PLOTTYPE_FILL:
			case rts2json::PLOTTYPE_FILL_SHARP:
				std::list <Magick::Coordinate> pol;
				pol.push_back (Magick::Coordinate (x, size.height () - x_axis_height));
				pol.push_back (Magick::Coordinate (x, y));
				pol.push_back (Magick::Coordinate (x_end - 1, (plotType == rts2json::PLOTTYPE_FILL_SHARP ? y : y_end)));
				pol.push_back (Magick::Coordinate (x_end - 1, size.height () - x_axis_height));
				image->draw (Magick::DrawablePolygon (pol));
		}
		x = x_end;
		y = y_end;
	}
}

#endif /* RTS2_HAVE_LIBJPEG */
