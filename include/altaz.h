/* 
 * Abstract class for Alt-AZ mounts.
 * Copyright (C) 2014 Petr Kubanek <petr@kubanek.net>
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
 * Abstract AltAz fork mount.
 *
 * Class for computing coordinates on Alt-Az mount.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class AltAz: public Telescope
{
	public:
		AltAz (int in_argc, char **in_argv, bool diffTrack = false, bool hasTracking = false, bool hasUnTelCoordinates = true);
		virtual ~AltAz (void);

	protected:
		int calculateMove (double JD, int32_t c_azc, int32_t c_altc, int32_t &t_azc, int32_t &t_altc);

		virtual int sky2counts (double JD, struct ln_equ_posn *pos, int32_t &azc, int32_t &altc, int used_flippingm, bool &use_flipped, bool writeValue, double haMargin, bool forceShortest);

		virtual int hrz2counts (struct ln_hrz_posn *hrz, int32_t &azc, int32_t &altc, int used_flipping, bool &use_flipped, bool writeValue, double haMargin);

		void counts2hrz (int32_t azc, int32_t altc, double &az, double &alt, double &un_az, double &un_zd);

		void counts2sky (double JD, int32_t azc, int32_t altc, double &ra, double &dec);

		/**
		 * Returns deratotor rate, in degrees/h.
		 */
		double derotator_rate (double az, double alt);

		/**
		 * Check trajectory. If hardHorizon is present, make sure that the path between current and target coordinates does
		 * not approach to horizon with given alt/az margin.
		 *
		 * @param JD                    Julian day for which trajectory will be checked
		 * @param azs			step in counts on AZ axe. Must be positive number
		 * @param alts			step in counts on ALT axe. Must be positive number
		 * @param steps			total number of steps the trajectory will check
		 * @param ignore_soft_beginning if true, algorithm will ignore fact that the mount is in soft limit at the beginning of the trajectory
		 *
		 * @return 0 if trajectory can be run without restriction, -1 if the trajectory goal is currently outside pointing limits,
		 * 1 if the trajectory will hit horizon limit (at and dt contains maximum point where we can track safely)
		 * 2 if the trajectory will hit hard horizon (at and dt contains maximum point where we can track safely)
		 */
		int checkTrajectory (double JD, int32_t azc, int32_t altc, int32_t &azt, int32_t &altt, int32_t azs, int32_t alts, unsigned int steps, double alt_margin, double az_margin, bool ignore_soft_beginning);

		/**
		 * Unlock basic pointing parameters. The parameters such as zero offsets etc. are made writable.
		 */
		void unlockPointing ();

                rts2core::ValueDouble *azSlewMargin;      //* margin for az axis during slew (used only on first call of startResync during new movement)

		rts2core::ValueLong *az_ticks;
		rts2core::ValueLong *alt_ticks;

		rts2core::ValueDouble *azZero;
		rts2core::ValueDouble *zdZero;

		rts2core::ValueDouble *azCpd;
		rts2core::ValueDouble *altCpd;

		rts2core::ValueLong *azMin;
		rts2core::ValueLong *azMax;
		rts2core::ValueLong *altMin;
		rts2core::ValueLong *altMax;

	private:
		double cos_lat;
};

};
