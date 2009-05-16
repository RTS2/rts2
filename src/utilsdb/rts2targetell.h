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

/**
 * Represent target of body moving on elliptical orbit.
 */
class EllTarget:public Target
{
	private:
		struct ln_ell_orbit orbit;
		void getPosition (struct ln_equ_posn *pos, double JD, struct ln_equ_posn *parallax);
	public:
		EllTarget (int in_tar_id, struct ln_lnlat_posn *in_obs);
		EllTarget ():Target ()
		{
		};
		virtual int load ();
		virtual void getPosition (struct ln_equ_posn *pos, double JD);
		virtual int getRST (struct ln_rst_time *rst, double jd, double horizon);

		virtual void printExtra (Rts2InfoValStream & _os, double JD);

		virtual void writeToImage (Rts2Image * image, double JD);

		double getEarthDistance (double JD);
		double getSolarDistance (double JD);
};
#endif							 /* !__RTS2_TARGETELL__ */
