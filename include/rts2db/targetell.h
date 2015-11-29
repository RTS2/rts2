/*
 * Class for targets representing bodies on elliptical orbits.
 * Copyright (C) 2005-2007,2009 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_TARGETELL__
#define __RTS2_TARGETELL__

#include "target.h"

namespace rts2db
{

/**
 * Represent target of body moving on elliptical orbit.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class EllTarget:public Target
{
	public:
		EllTarget (int in_tar_id, struct ln_lnlat_posn *in_obs, double in_altitude);
		EllTarget (std::string _tar_info):Target () { setTargetInfo (_tar_info); }
		EllTarget ():Target () { }
		virtual void load ();

		/**
		 * Get orbit structure from target info.
		 *
		 * @param mpc MPC 1-line element.
		 *
		 * @return -1 on error, 0 on success
		 */
		int orbitFromMPC (const char *mpc);

		virtual void getPosition (struct ln_equ_posn *pos, double JD);
		virtual int getRST (struct ln_rst_time *rst, double jd, double horizon);

		virtual void printExtra (Rts2InfoValStream & _os, double JD);

		virtual void writeToImage (rts2image::Image * image, double JD);

		double getEarthDistance (double JD);
		double getSolarDistance (double JD);

	private:
		struct ln_ell_orbit orbit;

		std::string designation;
		void getPosition (struct ln_equ_posn *pos, double JD, struct ln_equ_posn *parallax);
};

}
#endif							 /* !__RTS2_TARGETELL__ */
