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

// Limit on number of steps for trajectory check
#define TRAJECTORY_CHECK_LIMIT  2000

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

		int sky2counts (struct ln_equ_posn *pos, int32_t & ac, int32_t & dc, double JD, int used_flipping, bool &use_flipped, bool writeValues, double haMargin);

		double getHaZero () { return haZero->getValueDouble (); }
		double getDecZero () { return decZero->getValueDouble (); }
		double getRaTicks () { return ra_ticks->getValueDouble (); }
		double getDecTicks () { return dec_ticks->getValueDouble (); }

	protected:

		rts2core::ValueSelection *flipping;       //* flipping strategy - shortest, preffer same, preffer opposite,..

		rts2core::ValueDouble *haCWDAngle;        //* current HA counterweight down angle
		rts2core::ValueDouble *targetHaCWDAngle;  //* target HA counterweight down angle
		rts2core::ValueDouble *peekHaCwdAngle;    //* peek HA counterweight down angle

                rts2core::ValueDouble *haSlewMargin;      //* margin for ha axis during slew (used only on first call of startResync during new movement)

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

		virtual int sky2counts (double JD, struct ln_equ_posn *pos, int32_t &ac, int32_t &dc, bool writeValues, double haMargin);

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

		virtual int peek (double ra, double dec);

		/**
		 * Unlock basic pointing parameters. The parameters such as zero offsets etc. are made writable.
		 */
		void unlockPointing ();

		/**
		 * Returns angle of HA axis from counterweight down position.
		 *
		 * @param   ha_count    HA axis count (corrected for offset)
		 * @return  angle between HA axis and local meridian (counterweight down position).
		 */
		double getHACWDAngle (int32_t ha_count);

		/**
		 * Returns angle of DEC axis from pole position (where dec = 90).
		 */
		double getPoleAngle (int32_t dc);

		int counts2hrz (int32_t ac, int32_t dc, struct ln_hrz_posn *hrz, double JD);

		/**
		 * Check trajectory. If hardHorizon is present, make sure that the path between current and target coordinates does
		 * not approach to horizon with given alt/az margin.
		 *
		 * @param JD                    Julian day for which trajectory will be checked
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
		int checkTrajectory (double JD, int32_t ac, int32_t dc, int32_t &at, int32_t &dt, int32_t as, int32_t ds, unsigned int steps, double alt_margin, double az_margin, bool ignore_soft_beginning, bool dont_flip);

		/**
		 * Calculate move to reach given target ac/dc, keeping in mind horizon and other limits.
		 */
		int calculateMove (double JD, int32_t c_ac, int32_t c_dc, int32_t &t_ac, int32_t &t_dc, int pm);

		int checkMoveDEC (double JD, int32_t c_ac, int32_t &c_dc, int32_t &ac, int32_t &dc, int32_t move_d);

	private:
		int normalizeCountValues (int32_t ac, int32_t dc, int32_t &t_ac, int32_t &t_dc, double JD);

};

};
