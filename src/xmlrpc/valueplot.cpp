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
#include "../utils/libnova_cpp.h"
#include "../utilsdb/records.h"

using namespace rts2xmlrpc;

ValuePlot::ValuePlot (int _recvalId, int w, int h)
{
	recvalId = _recvalId;
	size.width (w);
	size.height (h);

	image = NULL;
}

Magick::Image* ValuePlot::getPlot (double from, double to)
{
	// first load values..
	rts2db::RecordsSet rs (recvalId);

	rs.load (from, to);

	delete image;

	image = new Magick::Image (size, "white");
	image->strokeColor ("black");
	image->strokeWidth (1);

	// Y axis scaling
	double min = rs.getMin ();
	double max = rs.getMax ();

	double scaleY = size.height () / (max - min);
	double scaleX = size.width () / (to - from); 

	for (rts2db::RecordsSet::iterator iter = rs.begin (); iter != rs.end (); iter++)
	{
		double x = scaleX * (iter->getRecTime () - from);
		double y = size.height () - scaleY * (iter->getValue () - min);
		image->draw (Magick::DrawableCircle (x, y, x - 2, y));
	}

	image->font("helvetica");

	if (max > 0 && min < 0)
	{
		// draw 0 line
		image->fontPointsize (15);
		image->draw (Magick::DrawableText (0, size.height () - (scaleY * -min) - 4, "0"));
		image->strokeWidth (3);
		plotYGrid (size.height () - (scaleY * -min));
	}

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

	Magick::Image pat ("10x1", "white");
	pat.pixelColor (3,0, "red");
	pat.pixelColor (4,0, "red");
	pat.pixelColor (5,0, "red");
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
			crossed0 = true;
		}
		image->strokePattern (pat);
		plotYGrid (size.height () - scaleY * y);
	}

	return image;
}

void ValuePlot::plotYGrid (int y)
{
	image->draw (Magick::DrawableLine (0, y, size.width (), y));
}

void ValuePlot::plotXGrid (int x)
{
	image->draw (Magick::DrawableLine (x, 0, x, size.height ()));
}
