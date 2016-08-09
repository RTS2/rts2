/* 
 * RTS2 telescope pointing model.
 * Copyright (C) 2015 Petr Kubanek <petr@kubanek.net>
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

#include "rts2model.h"
#include "libnova_cpp.h"

#include <fstream>

using namespace rts2telmodel;

RTS2Model::RTS2Model (rts2teld::Telescope * in_telescope, const char *in_modelFile):TelModel (in_telescope, in_modelFile)
{
	altaz = false;
	for (int i = 0; i < 9; i++)
		params[i] = 0;
}

RTS2Model::~RTS2Model (void)
{
}

int RTS2Model::load ()
{
	std::ifstream is (modelFile);
	load (is);
	if (is.fail ())
		return -1;
	return 0;
}

int RTS2Model::apply (struct ln_equ_posn *pos)
{
	double d_tar, r_tar;
	pos->ra = ln_deg_to_rad (pos->ra);
	pos->dec = ln_deg_to_rad (pos->dec);

	double lat_r = cond->getLatitudeRadians ();

	d_tar = pos->dec - params[0] - params[1] * cos (pos->ra) - params[2] * sin (pos->ra) - params[3] * (cos (lat_r) * sin (pos->dec) * cos (pos->ra) - sin (lat_r) * cos(pos->dec)) - params[8] * cos (pos->ra);
	r_tar = pos->ra - params[4] - params[5] / cos (pos->dec) - params[6] * tan (pos->dec) - (params[1] * sin (pos->ra) - params[2] * cos (pos->ra)) * tan (pos->dec) - params[3] * cos (lat_r) * sin (pos->ra) / cos (pos->dec) - params[7] * (sin (lat_r) * tan (pos->dec) + cos (lat_r) * cos (pos->ra));

	pos->ra = ln_rad_to_deg (r_tar);
	pos->dec = ln_rad_to_deg (d_tar);

	return 0;
}

int RTS2Model::applyVerbose (struct ln_equ_posn *pos)
{
	logStream (MESSAGE_DEBUG) << "Before: " << pos->ra << " " << pos->dec << sendLog;
	apply (pos);
	logStream (MESSAGE_DEBUG) << "After: " << pos->ra << " " << pos->dec << sendLog;
	return 0;
}

int RTS2Model::reverse (struct ln_equ_posn *pos)
{
	double d_tar, r_tar;
	pos->ra = ln_deg_to_rad (pos->ra);
	pos->dec = ln_deg_to_rad (pos->dec);

	double lat_r = cond->getLatitudeRadians ();

	d_tar = pos->dec + params[0] + params[1] * cos (pos->ra) + params[2] * sin (pos->ra) + params[3] * (cos (lat_r) * sin (pos->dec) * cos (pos->ra) - sin (lat_r) * cos(pos->dec)) + params[8] * cos (pos->ra);
	r_tar = pos->ra + params[4] + params[5] / cos (pos->dec) + params[6] * tan (pos->dec) + (params[1] * sin (pos->ra) - params[2] * cos (pos->ra)) * tan (pos->dec) + params[3] * cos (lat_r) * sin (pos->ra) / cos (pos->dec) + params[7] * (sin (lat_r) * tan (pos->dec) + cos (lat_r) * cos (pos->ra));

	pos->ra = ln_rad_to_deg (r_tar);
	pos->dec = ln_rad_to_deg (d_tar);

	return 0;
}

int RTS2Model::reverseVerbose (struct ln_equ_posn *pos)
{
	logStream (MESSAGE_DEBUG) << "Before: " << pos->ra << " " << pos->dec << sendLog;
	reverse (pos);
	logStream (MESSAGE_DEBUG) << "After: " << pos->ra << " " << pos->dec << sendLog;

	return 0;
}

int RTS2Model::reverse (struct ln_equ_posn *pos, double sid)
{
	return reverse (pos);
}

int RTS2Model::applyAltAz (struct ln_hrz_posn *hrz)
{
	double alt_tar, az_tar;
	hrz->az = ln_deg_to_rad (hrz->az);
	hrz->alt = ln_deg_to_rad (hrz->alt);

	az_tar = hrz->az - params[0] - params[1] * sin (hrz->az)  * tan (hrz->alt) - params[2] * cos (hrz->az) * tan (hrz->alt) - params[3] * tan (hrz->alt) + params[4] / cos (hrz->alt);
	alt_tar = hrz->alt - params[5] + params[1] * cos (hrz->az) - params[2] * sin (hrz->az) + params[6] * cos (hrz->alt);

	hrz->az = ln_rad_to_deg (az_tar);
	hrz->alt = ln_rad_to_deg (alt_tar);

	return 0;
}

int RTS2Model::reverseAltAz (struct ln_hrz_posn *hrz)
{
	double alt_tar, az_tar;
	hrz->az = ln_deg_to_rad (hrz->az);
	hrz->alt = ln_deg_to_rad (hrz->alt);

	az_tar = hrz->az + params[0] + params[1] * sin (hrz->az)  * tan (hrz->alt) + params[2] * cos (hrz->az) * tan (hrz->alt) + params[3] * tan (hrz->alt) - params[4] / cos (hrz->alt);
	alt_tar = hrz->alt - params[5] + params[1] * cos (hrz->az) - params[2] * sin (hrz->az) + params[6] * cos (hrz->alt);

	hrz->az = ln_rad_to_deg (az_tar);
	hrz->alt = ln_rad_to_deg (alt_tar);

	return 0;
}

std::istream & RTS2Model::load (std::istream & is)
{
	std::string name;

	is >> name;

	int pn = 9;

	if (name == "RTS2_ALTAZ")
	{
		altaz = true;
		pn = 7;
	}

	int i = 0;

	while (!is.eof () && i < pn)
	{
		is >> params[i];
		if (is.fail ())
		{
			logStream (MESSAGE_ERROR) << "cannot read parameter " << i << sendLog;
			return is;
		}
		i++;
	}
	return is;
}

std::ostream & RTS2Model::print (std::ostream & os)
{
	int pn = 9;
	if (altaz)
	{
		pn = 7;
		os << "RTS2_ALTAZ";
	}
	else
	{
		os << "RTS2_MODEL";
	}
	for (int i = 0; i < pn; i++)
		os << " " << params[i];
	return os;
}

inline std::istream & operator >> (std::istream & is, RTS2Model * model)
{
	return model->load (is);
}

inline std::ostream & operator << (std::ostream & os, RTS2Model * model)
{
	return model->print (os);
}
