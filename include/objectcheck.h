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

#ifndef __RTS2__OBJECTCHECK__
#define __RTS2__OBJECTCHECK__

#include <vector>
#include <libnova/ln_types.h>

/**
 * This holds one value of the horizon file.
 */
class HorizonEntry
{
	public:
		struct ln_hrz_posn hrz;

		HorizonEntry (double in_az, double in_alt)
		{
			hrz.az = in_az;
			hrz.alt = in_alt;
		}
};

typedef
std::vector < struct HorizonEntry >
horizon_t;

/**
 * Class for checking, whenewer observation target is correct or no.
 *
 * @author Petr Kubanek <petr@lascaux.asu.cas.cz>
 */
class
ObjectCheck
{
	private:
		enum
		{
			HA_DEC,
			AZ_ALT
		} horType;

		horizon_t
			horizon;
		int
			load_horizon (const char *horizon_file);

		double
			getHorizonHeightAz (double az, horizon_t::iterator iter1,
			horizon_t::iterator iter2);

	public:
		ObjectCheck (const char *horizon_file);

		~ObjectCheck ();
		/**
		 * Check, if that target can be observerd.
		 *
		 * @param st		local sidereal time of the observation in hours (0-24)
		 * @param ra		target ra in deg (0-360)
		 * @param dec		target dec
		 * @param hardness	how many limits to ignore (Moon distance etc.)
		 *
		 * @return 0 if we can observe, <0 otherwise
		 */
		int
			is_good (const struct ln_hrz_posn *hrz, int hardness = 0);

		double
			getHorizonHeight (const struct ln_hrz_posn *hrz, int hardness);

		horizon_t::iterator
			begin ()
		{
			return horizon.begin ();
		}

		horizon_t::iterator
			end ()
		{
			return horizon.end ();
		}
};
#endif							 /* ! __RTS2__OBJECTCHECK__ */
