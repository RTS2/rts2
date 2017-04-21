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

#include "rts2json/altaz.h"

#if defined(RTS2_HAVE_LIBJPEG) && RTS2_HAVE_LIBJPEG == 1

#include <math.h>
#include <iostream>

#include "configuration.h"

using namespace rts2json;

void AltAz::plotAltAzGrid (Magick::Color col)
{
	image.strokeColor (col);
	image.strokeWidth (1);
	image.fillColor (Magick::Color ());

	image.font ("helvetica");
	image.fontPointsize (12);

	// plot alt circles
	for (int i = 1; i <= 6; i ++)
	{
		image.draw (Magick::DrawableCircle (c, c, c - (i * l / 6.0), c));
	}

	// plot equaz lines
	int a = 0;
	int l_ann;
	double mCoeff=1.0;

	if (mirror)
		mCoeff=-1.0;

	static const char *labels[] = {"S", "30°", "60°", "W", "120°", "150°", "N", "210°", "240°", "E", "300°", "330°"};

	for (a = 0; a <= 11; a++)
	{
		double angle = ln_deg_to_rad (mCoeff * a * 30.0 + rotation);
		int x = c + l * sin (angle);
		int y = c - l * cos (angle);

		image.strokeColor (col);
		image.draw (Magick::DrawableLine (c, c, x, y));

		if (a % 3)
		{
			image.font ("helvetica");
			image.fontPointsize (9);
			l_ann = l + 5;
		}
		else
		{
			image.font ("helvetica");
			image.fontPointsize (12);
			l_ann = l - 5;
		}
		image.strokeColor (Magick::Color ("black"));
		image.fillColor (Magick::Color ("black"));

		double tr = (a % 3) ? mCoeff * a * 30.0 + rotation + 180.0 : 0;

		x = c + l_ann * sin (angle);
		y = c - l_ann * cos (angle);

		// not to cross boundaries (wouldn't annotate if yes)
		if (y - 6 < 0)
			y = 6;
		if (x - 8 < 0)
		{
			image.annotate (labels[a], Magick::Geometry (16, 12, 0, y - 6), Magick::WestGravity, tr);
		}
		else
			image.annotate (labels[a], Magick::Geometry (16, 12, x - 8, y - 6), Magick::CenterGravity, tr);

		image.fillColor (Magick::Color ());
	}

	image.strokeColor (col);
}

void AltAz::plotAltAzHorizon (Magick::Color col)
{
	image.strokeColor (col);
	image.strokeWidth (1);
	image.fillColor (col);

	struct ln_hrz_posn hrz;

	double az, zd;	// Azimuth, Zenith Distance : both in local image coordinates
	double stepAz = 1.0;
	double x, y, x_end, y_end;
	double x0, y0, x0_end, y0_end;

	hrz.az = 0;

	zd = l * (90.0 - rts2core::Configuration::instance ()->getObjectChecker ()->getHorizonHeight (&hrz, 0)) / 90.0; 
	if (!mirror)
		az = ln_deg_to_rad (hrz.az + rotation);
	else
		az = ln_deg_to_rad (- hrz.az + rotation);

	x = c + zd * sin (az);
	y = c - zd * cos (az);
	x0 = c + l * sin (az);
	y0 = c - l * cos (az);

	while (hrz.az < 360.0)
	{
		hrz.az += stepAz;
		zd = l * (90.0 - rts2core::Configuration::instance ()->getObjectChecker ()->getHorizonHeight (&hrz, 0)) / 90.0;
		if (!mirror)
			az = ln_deg_to_rad (hrz.az + rotation);
		else
			az = ln_deg_to_rad (- hrz.az + rotation);

		x_end = c + zd * sin (az);
		y_end = c - zd * cos (az);
		x0_end = c + l * sin (az);
		y0_end = c - l * cos (az);

		std::list <Magick::Coordinate> pol;
		pol.push_back (Magick::Coordinate (x0, y0));
		pol.push_back (Magick::Coordinate (x, y));
		pol.push_back (Magick::Coordinate (x_end, y_end));
		pol.push_back (Magick::Coordinate (x0_end, y0_end));
		image.draw (Magick::DrawablePolygon (pol));
		pol.clear ();

		x = x_end;
		y = y_end;
		x0 = x0_end;
		y0 = y0_end;
	}
}

void AltAz::plot (struct ln_hrz_posn *hrz, const char* label, const char* color, int type, double size)
{
	// calculate x and y
	double az;
	int x, y, ann_x, ann_y, ann_w, ann_h;

	if (std::isnan (hrz->alt) || std::isnan (hrz->az))
	{
		std::cerr << "Cannot plot nan coordinates: " << hrz->alt << " " << hrz->az << std::endl;
		return;
	}
	
	double zd = l * (90.0 - hrz->alt) / 90.0; 
	if (!mirror)
		az = ln_deg_to_rad (hrz->az + rotation);
	else
		az = ln_deg_to_rad (- hrz->az + rotation);

	x = c + zd * sin (az);
	y = c - zd * cos (az);

	image.strokeColor (color);
	image.strokeWidth (1);

	double semi_size = size / 2.0;
	double semi_size2;

	switch (type)
	{
		case PLOT_TYPE_TELESCOPE:
			semi_size2 = semi_size / 3.0;
			image.draw (Magick::DrawableCircle (x, y, x + semi_size, y));
			image.draw (Magick::DrawableLine (x - semi_size2, y, x + semi_size2, y));
			image.draw (Magick::DrawableLine (x, y - semi_size2, x, y + semi_size2));
			ann_x = x - 20;
			ann_y = y + semi_size;
			ann_w = 100;
			ann_h = 12;
			break;
		case PLOT_TYPE_CIRCLE:
			image.draw (Magick::DrawableCircle (x, y, x + semi_size, y));
			ann_x = x + 1 + (semi_size / 1.41421);
			ann_y = y - 12 - (semi_size / 1.41421);
			ann_w = 200;
			ann_h = 12;
			break;
		case PLOT_TYPE_POINT:
			image.fillColor (color);
			image.draw (Magick::DrawableCircle (x, y, x + semi_size, y));
			image.fillColor (Magick::Color ());
			ann_x = x + 1 + (semi_size / 1.41421);
			ann_y = y - 12 - (semi_size / 1.41421);
			ann_w = 200;
			ann_h = 12;
			break;
		case PLOT_TYPE_CROSS:
		default:
			image.draw (Magick::DrawableLine (x - semi_size, y, x + semi_size, y));
			image.draw (Magick::DrawableLine (x, y - semi_size, x, y + semi_size));
			ann_x = x + 1;
			ann_y = y - 12;
			ann_w = 200;
			ann_h = 12;
	}

	if (label)
	{
		if (label[0] == (char) 0xe2)
			image.font("/usr/share/fonts/truetype/freefont/FreeSerif.ttf");
		else
			image.font ("helvetica");
		if (type == PLOT_TYPE_TELESCOPE)
			image.fontPointsize (10);
		else
			image.fontPointsize (12);
		image.annotate (label, Magick::Geometry (ann_w, ann_h, ann_x, ann_y), Magick::WestGravity);
	}
}

void AltAz::setCenter ()
{
	unsigned int s = image.size ().width ();
	if (s > image.size ().height ())
		s = image.size ().height ();

	c = s / 2;
	l = c - 1;
}

#endif /* RTS2_HAVE_LIBJPEG */
