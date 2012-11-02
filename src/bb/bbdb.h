/**
 * RTS2 BB Database API
 * Copyright (C) 2012 Petr Kubanek <petr@kubanek.net>
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

#include <libnova/libnova.h>
#include <string>

#ifndef __RTS2_BBDB__
#define __RTS2_BBDB__

namespace rts2bb
{

class Observatory
{
	public:
		Observatory (int id);
		void load ();

		struct ln_lnlat_posn * getPosition () { return &position; }
		double getAltitude () { return altitude; }
		const char * getURL () { return url.c_str (); }
	private:
		int observatory_id;
		struct ln_lnlat_posn position;
		double altitude;
		std::string url;
};

/***
 * Register new target mapping into BB database.
 *
 * @param observatory_id  ID of observatory requesting the change
 * @param tar_id          ID of target in central database
 * @param obs_tar_id      ID of target in observatory database
 */
void createMapping (int observatory_id, int tar_id, int obs_tar_id);

void reportObservation (int observatory_id, int obs_id, int tar_id, double obs_ra, double obs_dec, double obs_slew, double obs_start, double obs_end, double onsky, int good_images, int bad_images);

/**
 * Find mapping for a given target.
 */
int findMapping (int observatory_id, int obs_tar_id);

int findObservatoryMapping (int observatory_id, int tar_id);

}

#endif   // __RTS2_BBDB__
