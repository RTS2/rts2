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
#include "../utilsdb/records.h"

using namespace rts2xmlrpc;

ValuePlot::ValuePlot (int _recvalId, int w, int h)
{
	recvalId = _recvalId;
	size.width (w);
	size.height (h);
}

Magick::Image ValuePlot::getPlot (double from, double to)
{
	// first load values..
	rts2db::RecordsSet rs (recvalId);

	rs.load (from, to);

	Magick::Image image (size, "white");
	image.strokeColor ("green");
	image.strokeWidth (1);

	// Y axis scaling
	double min = rs.getMin ();
	double max = rs.getMax ();

	double scaleY = size.height () / (max - min);
	double scaleX = size.width () / (to - from); 

	for (rts2db::RecordsSet::iterator iter = rs.begin (); iter != rs.end (); iter++)
	{
		double x = scaleX * (iter->getRecTime () - from);
		double y = scaleY * (iter->getValue () - min);
		image.draw (Magick::DrawableCircle (x, y, x - 2, y));
	}

	return image;
}
