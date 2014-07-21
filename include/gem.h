/* 
 * Abstract class for GEMs.
 * Copyright (C) 2007-2008 Petr Kubanek <petr@kubanek.net>
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

#include "teld.h"

namespace rts2teld
{

/**
 * Abstract German Equatorial Mount class.
 *
 * This class solves various problems related to GEM proper modelling.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class GEM: public Telescope
{
	public:
		GEM (int in_argc, char **in_argv, bool diffTrack = false, bool hasTracking = false, bool hasUnTelCoordinates = true);
		virtual ~GEM (void);

	protected:
		/**
		 * GEM parameters, in degrees (HA/Dec coordinates of hw-zero positions, decZero with inverted sign on south hemisphere).
		 */
		double haZero;
		double decZero;

		double haCpd;
		double decCpd;

		int32_t acMin;
		int32_t acMax;
		int32_t dcMin;
		int32_t dcMax;

		int acMargin;

		// ticks per revolution
		int32_t ra_ticks;
		int32_t dec_ticks;

		virtual int updateLimits () = 0;

		/**
		 * Gets home offset.
		 */
		virtual int getHomeOffset (int32_t & off) = 0;

		int sky2counts (int32_t & ac, int32_t & dc);
		int sky2counts (struct ln_equ_posn *pos, int32_t & ac, int32_t & dc, double JD, int32_t homeOff);

		/**
		 * Convert counts to RA&Dec coordinates.
		 *
		 * @param ac      Alpha counts
		 * @param dc      Delta counts
		 * @param ra      Telescope RA
		 * @param dec     Telescope declination
		 * @param flip    flip status
		 * @param un_ra   unflipped (raw hardware) RA
		 * @param un_dec  unflipped (raw hardware) declination
		 */
		int counts2sky (int32_t ac, int32_t dc, double &ra, double &dec, int &flip, double &un_ra, double &un_dec);
};

};
