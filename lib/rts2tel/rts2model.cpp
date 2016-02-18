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

        std::cout << lat_r << " ";

	for (int i = 0; i < 9; i++)
		std::cout << params[i] << " ";

	d_tar = pos->dec - params[0] - params[1] * cos (pos->ra) - params[2] * sin (pos->ra) - params[3] * (cos (lat_r) * sin (pos->dec) * cos (pos->ra) - sin (lat_r) * cos(pos->dec)) - params[8] * cos (pos->ra);
	r_tar = pos->ra - params[4] - params[5] / cos (pos->dec) - params[6] * tan (pos->dec) - (params[1] * sin (pos->ra) - params[2] * cos (pos->ra)) * tan (pos->dec) - params[3] * cos (lat_r) * sin (pos->ra) / cos (pos->dec) - params[7] * (sin (lat_r) * tan (pos->dec) + cos (lat_r) * cos (pos->ra));

	std::cout << pos->ra << " " << pos->dec << " " << std::endl;

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

std::istream & RTS2Model::load (std::istream & is)
{
	std::string name;

	is >> name;

	int i = 0;

	while (!is.eof () && i < 9)
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
	os << "RTS2_MODEL";
	for (int i = 0; i < 9; i++)
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
