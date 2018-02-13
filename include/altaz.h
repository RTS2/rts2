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
		AltAz (int in_argc, char **in_argv, bool diffTrack = false, bool hasTracking = false, bool hasUnTelCoordinates = true, bool hasAltAzDiff = true, bool parkingBlock = true);
		virtual ~AltAz (void);

		virtual int infoUTCLST (const double utc1, const double utc2, double telLST);

	protected:
		int calculateMove (double JD, int32_t c_azc, int32_t c_altc, int32_t &t_azc, int32_t &t_altc);

		virtual int sky2counts (const double utc1, const double utc2, struct ln_equ_posn *pos, struct ln_hrz_posn *hrz_out, int32_t &azc, int32_t &altc, bool writeValue, double haMargin, bool forceShortest);

		/**
		 * Converts horizontal position into axis counts.
		 *
		 * @param az_zero - 0 don't care, -1 put az to -ticks/2..ticks/2 range, 1 put az to 0..ticks range
		 */
		virtual int hrz2counts (struct ln_hrz_posn *hrz, int32_t &azc, int32_t &altc, int used_flipping, bool &use_flipped, bool writeValue, double haMargin, int az_zero);

		void counts2hrz (int32_t azc, int32_t altc, double &az, double &alt, double &un_az, double &un_zd);

		void counts2sky (double JD, int32_t azc, int32_t altc, double &ra, double &dec);

		/**
		 * Returns parallactic angle and its change (in deg/h).
		 */
		void parallactic_angle (double ha, double dec, double &pa, double &parate) { parallacticAngle (ha, dec, sin_lat, cos_lat, tan_lat, pa, parate); }

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

		/**
		 * Sends updates to derotators.
		 */
		virtual void runTracking ();

		virtual int setTracking (int track, bool addTrackingTimer = false, bool send = true);

		/**
		 * Sends new target parallactic angle to 
		 * devices requiring it (rotators,..).
		 */
		virtual void parallacticTracking ();

		/**
		 * Sends updated PA data.
		 */
		void sendPA ();

		rts2core::ValueDouble *parallAngle;
		rts2core::ValueDouble *derRate;

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

		virtual void afterMovementStart ();
		virtual void afterParkingStart ();

	private:
		double nextParUpdate;
};

};
