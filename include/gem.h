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

		rts2core::ValueSelection *flipping; //* flipping strategy - shortest, preffer same, preffer opposite,..

		/**
		 * GEM parameters, in degrees (HA/Dec coordinates of hw-zero positions, decZero with inverted sign on south hemisphere).
		 */
		rts2core::ValueSelection *haZeroPos;
		rts2core::ValueDouble *haZero;
		rts2core::ValueDouble *decZero;

		rts2core::ValueDouble *haCpd;
		rts2core::ValueDouble *decCpd;

		rts2core::ValueLong *acMin;
		rts2core::ValueLong *acMax;
		rts2core::ValueLong *dcMin;
		rts2core::ValueLong *dcMax;

		int acMargin;

		// ticks per revolution
		rts2core::ValueLong *ra_ticks;
		rts2core::ValueLong *dec_ticks;

		virtual int updateLimits () = 0;

		/**
		 * Gets home offset.
		 */
		virtual int getHomeOffset (int32_t & off) = 0;

		int sky2counts (int32_t & ac, int32_t & dc);
		int sky2counts (struct ln_equ_posn *pos, int32_t & ac, int32_t & dc, double JD, int32_t homeOff, int actual_flip);

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
		int counts2sky (int32_t ac, int32_t dc, double &ra, double &dec, int &flip, double &un_ra, double &un_dec)
		{
			return counts2sky (ac, dc, ra, dec, flip, un_ra, un_dec, ln_get_julian_from_sys ());
		}

		int counts2sky (int32_t ac, int32_t dc, double &ra, double &dec, int &flip, double &un_ra, double &un_dec, double JD);

		/**
		 * Unlock basic pointing parameters. The parameters such as zero offsets etc. are made writable.
		 */
		void unlockPointing ();


		/**
		 * Check trajectory. If hardHorizon is present, make sure that the path between current and target coordinates does
		 * not approach to horizon with given alt/az margin.
		 *
		 *
		 * @param as			step in counts on RA/HA axe. Must be positive number
		 * @param ds			step in counts on DEC axe. Must be positive number
		 * @param steps			total number of steps the trajectory will check
		 * @param ignore_soft_beginning if true, algorithm will ignore fact that the mount is in soft limit at the beginning of the trajectory
		 * @param max_alt               maximum altitude mouont should go to
		 *
		 * @return 0 if trajectory can be run without restriction, -1 if the trajectory goal is currently outside pointing limits,
		 * 1 if the trajectory will hit horizon limit (at and dt contains maximum point where we can track safely)
		 * 2 if the trajectory will hit hard horizon (at and dt contains maximum point where we can track safely)
		 */
		int checkTrajectory (int32_t ac, int32_t dc, int32_t &at, int32_t &dt, int32_t as, int32_t ds, unsigned int steps, double alt_margin, double az_margin, bool ignore_soft_beginning, bool dont_flip);


	private:
		int checkCountValues (struct ln_equ_posn *pos, int32_t ac, int32_t dc, int32_t &t_ac, int32_t &t_dc, double JD, double ls, double dec);
};

};
