/* 
 * Variable plotting library.
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

#include <time.h>

#include "valueplot.h"

#ifdef HAVE_LIBJPEG

#include "../utils/expander.h"
#include "../utils/libnova_cpp.h"

using namespace rts2xmlrpc;

ValuePlot::ValuePlot (int _recvalId, int _valType, int w, int h)
{
	recvalId = _recvalId;
	valueType = _valType;
	size.width (w);
	size.height (h);

	image = NULL;
}

Magick::Image* ValuePlot::getPlot (double _from, double _to, Magick::Image* _image, PlotType _plotType, int shadow)
{
	// first load values..
	rts2db::RecordsSet rs (recvalId);

	from = _from;
	to = _to;
	plotType = _plotType;

	rs.load (from, to);

	if (_image)
	{
		image = image;
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

	scaleY = size.height () / (max - min);
	scaleX = size.width () / (to - from); 

	image->font("helvetica");
	image->strokeAntiAlias (true);

	switch (valueType & RTS2_BASE_TYPE)
	{
		case RTS2_VALUE_BOOL:
			min = -0.1;
			max = 1.1;
			if (plotType == PLOTTYPE_AUTO)
				plotType = PLOTTYPE_FILL_SHARP;
			plotYBoolean ();
			break;
		case RTS2_VALUE_DOUBLE:
		default:
			if (max > 0 && min < 0)
			{
				// draw 0 line
				image->fontPointsize (15);
				image->draw (Magick::DrawableText (0, size.height () - (scaleY * -min) - 4, "0"));
				image->strokeWidth (3);
				plotYGrid (size.height () - (scaleY * -min));
			}
			if (plotType == PLOTTYPE_AUTO)
				plotType = PLOTTYPE_LINE;
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

	plotXDate ();

	if (!rs.empty ())
	{
		if (shadow)
			plotData (rs, Magick::Color (MaxRGB / 5, MaxRGB / 5, MaxRGB / 5, 3 * MaxRGB / 5), shadow);
		plotData (rs, Magick::Color (0, MaxRGB, 0, MaxRGB / 5), 0);
	}

	return image;
}

void ValuePlot::plotData (rts2db::RecordsSet &rs, Magick::Color col, int shadow)
{
	// reset stroke pattern
	image->strokePattern (Magick::Image (Magick::Geometry (1,1), col));
	image->strokeColor (col);
	image->strokeWidth (3);
	image->fillColor (col);

	rts2db::RecordsSet::iterator iter = rs.begin ();

	double x = scaleX * (iter->getRecTime () - from) + shadow;
	double y = size.height () - scaleY * (iter->getValue () - min) + shadow;

	for (; iter != rs.end (); )
	{
		iter++;
		double x_end = (iter == rs.end ()) ? size.width (): scaleX * (iter->getRecTime () - from) + shadow;
		double y_end = (iter == rs.end ()) ? y : size.height () - scaleY * (iter->getValue () - min) + shadow;
		switch (plotType)
		{
			case PLOTTYPE_AUTO:
			case PLOTTYPE_LINE:
				image->draw (Magick::DrawableLine (x, y, x_end, y_end));
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
		y = y_end;
	}
}

void ValuePlot::plotYGrid (int y)
{
	image->draw (Magick::DrawableLine (0, y, size.width (), y));
}

void ValuePlot::plotXGrid (int x)
{
	image->draw (Magick::DrawableLine (x, 0, x, size.height ()));
}


void ValuePlot::plotYDegrees ()
{
	// get difference and plot lines at interesting points
	double diff = max - min;

	double grid_y_step;

	// calculate scale in deg/pix - scaleY is in pix/deg
	double sy = 1 / scaleY;
	if (sy < 1 / 360000.0)
		grid_y_step = 10 / 360000.0;
	else if (sy < 1 / 3600.0)
		grid_y_step = 10 / 3600.0;
	else if (sy < 1 / 120.0)
		grid_y_step = 10 / 120.0;
	else if (sy < 1 / 60.0)
		grid_y_step = 10 / 60.0;
	else if (sy < 1 / 30.0)
		grid_y_step = 10 / 30.0;
	else if (sy < 1)
		grid_y_step = 10;
	else
		grid_y_step = 50;
	
	image->strokeWidth (1);
	image->fontPointsize (12);
	image->strokeColor ("red");

	bool crossed0 = false;

	// round start and plot grid..
	for (double y = ceil (min / grid_y_step) * grid_y_step - min; y < diff; y += grid_y_step)
	{
		if (fabs (y + min) < grid_y_step / 2.0)
			continue;
	  	std::ostringstream _os;
		_os << LibnovaDegDist (y + min);
		if (!crossed0 && y + min > 0)
		{
			image->strokeColor ("blue");
			crossed0 = true;
		}
		image->draw (Magick::DrawableText (0, size.height () - scaleY * y - 5, _os.str ().c_str ()));
	}

	Magick::Image pat (Magick::Geometry (9, 1), Magick::Color (MaxRGB, MaxRGB, MaxRGB, 0));
	pat.pixelColor (3,0, "red");
	pat.pixelColor (4,0, "red");
	pat.pixelColor (5,0, "red");
	pat.pixelColor (6,0, "red");
	image->strokePattern (pat);

	crossed0 = false;
		
	// round start and plot grid..
	for (double y = ceil (min / grid_y_step) * grid_y_step - min; y < diff; y += grid_y_step)
	{
		if (!crossed0 && y + min > 0)
		{
			pat.pixelColor (3,0, "blue");
			pat.pixelColor (4,0, "blue");
			pat.pixelColor (5,0, "blue");
			pat.pixelColor (6,0, "blue");
			crossed0 = true;
		}
		image->strokePattern (pat);
		plotYGrid (size.height () - scaleY * y);
	}
}

void ValuePlot::plotYDouble ()
{
	// get difference and plot lines at interesting points
	double diff = max - min;

	// plot roughly every 20 pixels..
	double grid_y_step = log(100.0 / scaleY) / log(10);

	// round up
	grid_y_step = pow (10, floor (grid_y_step));

	while (grid_y_step * scaleY > 100)
		grid_y_step /= 2.0;
	
	image->strokeWidth (1);
	image->fontPointsize (12);
	image->strokeColor ("red");

	bool crossed0 = false;

	// round start and plot grid..
	for (double y = ceil (min / grid_y_step) * grid_y_step - min; y < diff; y += grid_y_step)
	{
		// do not plot around 0
		if (fabs (y + min) < grid_y_step / 2.0)
			continue;
	  	std::ostringstream _os;
		_os << y + min;
		if (!crossed0 && y + min > 0)
		{
			image->strokeColor ("blue");
			crossed0 = true;
		}
		image->draw (Magick::DrawableText (0, size.height () - scaleY * y - 5, _os.str ().c_str ()));
	}

	Magick::Image pat ("2x1", Magick::Color ());
	pat.pixelColor (1,0, "red");
	image->strokePattern (pat);

	crossed0 = false;
		
	// round start and plot grid..
	for (double y = ceil (min / grid_y_step) * grid_y_step - min; y < diff; y += grid_y_step)
	{
		if (!crossed0 && y + min > 0)
		{
			pat.pixelColor (1,0, "blue");
			crossed0 = true;
		}
		image->strokePattern (pat);
		plotYGrid (size.height () - scaleY * y);
	}
}

void ValuePlot::plotYBoolean ()
{
	scaleY = size.height () / (max - min);

	Magick::Image pat ("2x1", Magick::Color ());

	// 0 label..
	pat.pixelColor (1,0, "red");
	image->strokePattern (pat);
	image->strokeColor ("red");

	plotYGrid (size.height () + min * scaleY);

	// true label..
	image->strokeColor ("green");
	pat.pixelColor (1,0, "green");
	image->strokePattern (pat);

	plotYGrid (size.height () + min * scaleY - scaleY);
}

void ValuePlot::plotXDate ()
{
	// scale per pixel..
	double p_scale = 1 / fabs (scaleX);
	double tick_scale;
	const char *tick_format;

	// seconds..
	if (p_scale <= 1)
	{
		tick_scale = 60;
		tick_format = "%M:%S";
	}
	// 1/2 minute..
	else if (p_scale <= 30)
	{
	  	tick_scale = 1800;
		tick_format = "%M:%S";
	}
	else if (p_scale <= 60)
	{
		tick_scale = 3600;
		tick_format = "%d %H:%M";
	}
	else if (p_scale <= 300)
	{
		tick_scale = 7200;
		tick_format = "%d %H:%M";
	}
	else if (p_scale <= 600)
	{
		tick_scale = 36000;
		tick_format = "c %d %H:%M";
	}
	else if (p_scale <= 1800)
	{
		tick_scale = 86400;
		tick_format = "%d %H:%M";
	}
	else if (p_scale <= 3600)
	{
		tick_scale = 172800;
		tick_format = "%d %H:%M";
	}
	else
	{
		tick_scale = 4320000;
		tick_format = "%m-%d";
	}

	struct timeval tv;
	tv.tv_usec = 0;
	rts2core::Expander ex = rts2core::Expander ();

	double t_diff = fabs (to - from);

	Magick::Image pat1 ("1x1", "black");
	image->strokePattern (pat1);
	image->strokeWidth (1);
	image->fontPointsize (12);
	image->strokeColor ("black");

	for (double x = ceil (from / tick_scale) * tick_scale - from; x < t_diff; x += tick_scale)
	{
		tv.tv_sec = from + x;
		ex.setExpandDate (&tv);
		image->draw (Magick::DrawableText (x * scaleX, size.height () - 10, ex.expand (tick_format).c_str ()));
	}

	Magick::Image pat2 ("1x2", Magick::Color ());
	pat2.pixelColor (0,0, "white");
	image->strokePattern (pat2);

	for (double x = ceil (from / tick_scale) * tick_scale - from; x < t_diff; x += tick_scale)
	{
		plotXGrid (x * scaleX);
	}
}

#endif /* HAVE_LIBJPEG */
