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

#include "altaz.h"

#if defined(RTS2_HAVE_LIBJPEG) && RTS2_HAVE_LIBJPEG == 1

#include <math.h>
#include <iostream>

using namespace rts2xmlrpc;

void AltAz::plotAltAzGrid ()
{
	// plot equaz lines

	image.strokeColor ("red");
	image.strokeWidth (1);
	image.fillColor (Magick::Color ());

	image.font ("helvetica");
	image.fontPointsize (12);

	int a = 0;

	static const char *labels[] = {"S", "30°", "60°", "W", "120°", "150°", "N", "210°", "240°", "E", "300°", "330°"};

	for (double i = rotation; i < 2 * M_PI + rotation * (M_PI / 180); i += M_PI / 6.0)
	{
		int x = c + l * sin (i);
		int y = c - l * cos (i);

		image.strokeColor ("red");
		image.draw (Magick::DrawableLine (c, c, x, y));

		image.strokeColor ("black");

		double tr = (a % 3) ? a * 30 : 0;

		x = c + (l + 7) * sin (i);
		y = c - (l + 7) * cos (i);

		image.annotate (labels[a], Magick::Geometry (20, 12, x - 10, y - 6), Magick::CenterGravity, tr);

		a++;
	}

	image.strokeColor ("red");

	// plot alt circles
	for (int i = 0; i <= l; i += l / 6)
	{
		image.draw (Magick::DrawableCircle (c, c, c - i, c));
	}
}

void AltAz::plotCross (struct ln_hrz_posn *hrz, const char* label, const char* color)
{
	// calculate x and y

	if (isnan (hrz->alt) || isnan (hrz->az))
	{
		std::cerr << "Cannot plot nan coordinates: " << hrz->alt << " " << hrz->az << std::endl;
		return;
	}
	
	double alt = l * (90 - hrz->alt) / 90; 
	double az = ln_deg_to_rad (hrz->az) + rotation;

	int x = c + alt * sin (az);
	int y = c - alt * cos (az);

	image.strokeColor (color);
	image.draw (Magick::DrawableLine (x - 5, y, x + 5, y));
	image.draw (Magick::DrawableLine (x, y - 5, x, y + 5));

	if (label)
		image.annotate (label, Magick::Geometry (200, 12, x, y - 12), Magick::WestGravity);
}

void AltAz::setCenter ()
{
	unsigned int s = image.size ().width ();
	if (s > image.size ().height ())
		s = image.size ().height ();

	c = s / 2;
	l = ((c - 20 ) / 6) * 6;
}

#endif /* RTS2_HAVE_LIBJPEG */
