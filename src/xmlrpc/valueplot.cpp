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

ValuePlot::ValuePlot (int _recvalId)
{
	recvalId = _recvalId;
}

Magick::Image ValuePlot::getPlot ()
{
	// first load values..
	rts2db::RecordsSet rs (recvalId);

	double to = time (NULL);
	double from = to - 86400;

	rs.load (from, to);

	Magick::Image image (Magick::Geometry (200, 200), "white");
	image.strokeColor ("red");

	// Y axis scaling
	double min = rs.getMin ();
	double max = rs.getMax ();

	double scaleY = (max - min) / 200;
	double scaleX = 86400 / 200;

	for (rts2db::RecordsSet::iterator iter = rs.begin (); iter != rs.end (); iter++)
	{
		
		image.draw (Magick::DrawableCircle (from + scaleX * (iter->getRecTime () - from)  ,min + scaleY * (iter->getValue () - min), 5, 2));
	}

	return image;
}
