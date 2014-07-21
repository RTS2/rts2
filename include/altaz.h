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
 * Class for computing coordinates on fork alt-az mount.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class AltAz: public Telescope
{
	public:
		AltAz (int in_argc, char **in_argv, bool diffTrack = false, bool hasTracking = false, bool hasUnTelCoordinates = true);
		virtual ~AltAz (void);

	protected:
		/**
		 * Fork parameters, in degrees (Alt/Az coordinates of hw-zero positions).
		 */
		double azZero;
		double altZero;

		double azCpd;
		double altCpd;

		int32_t acMin;
		int32_t acMax;
		int32_t dcMin;
		int32_t dcMax;

		int acMargin;

		// ticks per revolution
		int32_t alt_ticks;
		int32_t az_ticks;

		virtual int updateLimits () = 0;

		/**
		 * Gets home offset.
		 */
		virtual int getHomeOffset (int32_t & off) = 0;

		int sky2counts (int32_t & ac, int32_t & dc);
		int sky2counts (struct ln_equ_posn *pos, int32_t & ac, int32_t & dc, double JD, int32_t homeOff);
		int counts2sky (int32_t ac, int32_t dc, double &ra, double &dec, int &flip, double &un_ra, double &un_dec);
};

};
