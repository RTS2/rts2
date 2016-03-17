/*
 * Class for targets representing arteficiall satellites (two line elements).
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

#ifndef __RTS2_TARGETTLE__
#define __RTS2_TARGETTLE__

#include "target.h"

#include "pluto/norad.h"

namespace rts2db
{

/**
 * Represent target of body moving on elliptical orbit.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class TLETarget:public Target
{
	public:
		TLETarget (int in_tar_id, struct ln_lnlat_posn *in_obs, double in_altitude);
		TLETarget (std::string _tar_info):Target () { setTargetInfo (_tar_info); }
		TLETarget ():Target () { }
		virtual void load ();

		/**
		 * Get orbit from TLE, separated with |
		 *
		 * @param tle      two line element, separated with |
		 */
		void orbitFromTLE (std::string tle);

		virtual void getPosition (struct ln_equ_posn *pos, double JD);
		virtual int getRST (struct ln_rst_time *rst, double jd, double horizon);

		virtual moveType startSlew (struct ln_equ_posn *position, std::string &p1, std::string &p2, bool update_position, int plan_id = -1);

		virtual void printExtra (Rts2InfoValStream & _os, double JD);

		virtual void writeToImage (rts2image::Image * image, double JD);

		double getEarthDistance (double JD);
		double getSolarDistance (double JD);

	private:
		std::string tle1;
		std::string tle2;

		tle_t tle;

		int ephem;
		int is_deep;

		void getPosition (struct ln_equ_posn *pos, double JD, struct ln_equ_posn *parallax);
};

}
#endif							 /* !__RTS2_TARGETTLE__ */
