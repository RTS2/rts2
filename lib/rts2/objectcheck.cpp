/* 
 * Check if object is above horizon.
 * Copyright (C) 2003-2007 Petr Kubanek <petr@kubanek.net>
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

#include "configuration.h"

#include <stdio.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <string.h>

#include <algorithm>

#include <libnova/libnova.h>
#include <libnova/utility.h>
#include "objectcheck.h"
#include "libnova_cpp.h"

using namespace rts2core;

ObjectCheck::ObjectCheck (const char *horizon_file)
{
	horType = HA_DEC;
	load_horizon (horizon_file);
}

ObjectCheck::~ObjectCheck (void)
{
	horizon.clear ();
}

bool RAcomp (HorizonEntry hor1, HorizonEntry hor2)
{
	return hor1.hrz.az < hor2.hrz.az;
}

int ObjectCheck::load_horizon (const char *horizon_file)
{
	std::ifstream inf;

	struct ln_equ_posn pos;
	struct ln_hrz_posn hrz;

	// - is without horizon file
	if (!strcmp (horizon_file, "-"))
		return 0;

	inf.open (horizon_file);

	struct ln_lnlat_posn *observer = Configuration::instance ()->getObserver ();

	if (inf.fail ())
	{
		std::cerr << "cannot open horizon file '" << horizon_file << "'" << std::endl;
		return -1;
	}

	while (!inf.eof ())
	{
		char buf[80];

		inf.getline (buf, 80);

		if (inf.fail ())
		{
			if (inf.eof ())
				break;
			std::cerr << "Error getting line from " << horizon_file << std::
				endl;
			return -1;
		}

		// now parse the line..
		for (char *top = buf; *top && *top != '\n'; top++)
		{
			if (isspace (*top))
				continue;
			// comment
			if (*top == ';' || *top == '#')
				break;
			if (isalpha (*top))
			{
				if (!strcasecmp (top, "AZ-ALT"))
				{
					horType = AZ_ALT;
				}
				else if (!strcasecmp (top, "HA-DEC"))
				{
					horType = HA_DEC;
				}
				else
				{
					std::
						cerr << "Unknow horizon file type: " << horType << std::
						endl;
					return -1;
				}
				// exit from the loop, get next line
				break;
			}
			// otherwise get val1 and val2
			LibnovaDeg val1;
			LibnovaDeg val2;
			std::istringstream is;
			is.str (std::string (buf));
			is >> val1 >> val2;
			if (!is.fail () || is.eof ())
			{
				switch (horType)
				{
					case AZ_ALT:
						horizon.
							push_back (HorizonEntry (val1.getDeg (), val2.getDeg ()));
						break;
					case HA_DEC:
						pos.ra = val1.getDeg () * 15.0;
						pos.dec = val2.getDeg ();

						ln_get_hrz_from_equ_sidereal_time (&pos, observer,
							observer->lng / -15.0,
							&hrz);
						if (hrz.az == 360.0)
							hrz.az = 0.0;

						horizon.push_back (HorizonEntry (hrz.az, hrz.alt));
						break;
				}
				break;
			}
			else
			{
				std::cerr << "Error converting " << buf
					<< " in " << horizon_file
					<< " to val1 val2 pair" << std::endl;
				return -1;
			}
		}
	}

	// sort horizon file
	sort (horizon.begin (), horizon.end (), RAcomp);

	return 0;
}

int ObjectCheck::is_good (const struct ln_hrz_posn *hrz, int hardness)
{
	return hrz->alt > getHorizonHeight (hrz, hardness);
}

int ObjectCheck::is_good_with_margin (struct ln_hrz_posn *hrz, double alt_margin, double az_margin, int hardness)
{
	// check margin one..
	hrz->alt = hrz->alt - alt_margin;
	if (hrz->alt > 90)
		hrz->alt = 90;

	hrz->az = ln_range_degrees (hrz->az - az_margin);

	int ret = is_good (hrz, hardness);
	if (ret != 0)
		return ret;

	hrz->az = ln_range_degrees (hrz->az + 2 * az_margin);

	return is_good (hrz, hardness);
}

double ObjectCheck::getHorizonHeightAz (double az, horizon_t::iterator iter1, horizon_t::iterator iter2)
{
	double az1;
	if ((*iter1).hrz.az > (*iter2).hrz.az)
		az1 = (*iter1).hrz.az - 360.0;
	else
		az1 = (*iter1).hrz.az;
	return (*iter1).hrz.alt + ln_range_degrees (az - az1) * ((*iter2).hrz.alt -
		(*iter1).hrz.alt) /
		((*iter2).hrz.az - az1);
}

double ObjectCheck::getHorizonHeight (const struct ln_hrz_posn *hrz, __attribute__ ((unused)) int hardness)
{
	if (horizon.size () == 0)
		return 0;

	horizon_t::iterator iter = horizon.begin ();

	if (hrz->az < (*iter).hrz.az)
		return getHorizonHeightAz (hrz->az, iter, --horizon.end ());

	horizon_t::iterator iter_last = iter;

	iter++;

	for (; iter != horizon.end (); iter++)
	{
		if ((*iter).hrz.az > hrz->az)
			return getHorizonHeightAz (hrz->az, iter_last, iter);
		iter_last = iter;
	}
	return getHorizonHeightAz (hrz->az, iter_last, horizon.begin ());
}
