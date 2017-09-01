/*
 * Abstract class for fork mounts.
 * Copyright (C) 2008 Petr Kubanek <petr@kubanek.net>
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

#include "fork.h"
#include "configuration.h"

#include "libnova_cpp.h"

using namespace rts2teld;

int Fork::sky2counts (int32_t & ac, int32_t & dc)
{
	double JD;
	struct ln_equ_posn pos;

	JD = ln_get_julian_from_sys ();

	getTarget (&pos);

	return sky2counts (&pos, ac, dc, JD);
}

int Fork::sky2counts (struct ln_equ_posn *pos, int32_t & ac, int32_t & dc, double JD)
{
	double ls, ha, dec;
	struct ln_hrz_posn hrz;
	struct ln_equ_posn model_change;
	struct ln_equ_posn u_pos, tt_pos;
	int ret;
	bool flip = false;

	if (std::isnan(pos->ra) || std::isnan(pos->dec))
	{
		logStream (MESSAGE_ERROR) << "sky2counts called with nan ra/dec" << sendLog;
		return -1;
	}
	ls = getLstDeg (JD, 0);

	ln_get_hrz_from_equ (pos, rts2core::Configuration::instance ()->getObserver (), JD, &hrz);
	if (hrz.alt < -1)
	{
		logStream (MESSAGE_ERROR) << "object is below horizon, azimuth is "
			<< hrz.az  << " and altitude " << hrz.alt  << " RA/DEC targets was " << LibnovaRaDec (pos)
			<< ", check observatory time and location (long & latitude)"
			<< sendLog;
		return -1;
	}

	// get hour angle
	ha = ln_range_degrees (ls - pos->ra);
	if (ha > 180.0)
		ha -= 360.0;

	// pretend we are at north hemispehere.. at least for dec
	dec = pos->dec;
	if (telLatitude->getValueDouble () < 0)
		dec *= -1;

	// convert to count values
	ac = (int32_t) ((ha - haZero) * haCpd);
	dc = (int32_t) ((dec - decZero) * decCpd);

	// gets the limits
	ret = updateLimits ();
	if (ret)
	{
		return -1;
	}

	// purpose of following code is to get from west side of flip
	// on S, we prefer negative values
	if (telLatitude->getValueDouble () < 0)
	{
		while ((ac - acMargin) < acMin)
			// ticks per revolution - don't have idea where to get that
		{
			ac += (int32_t) (ra_ticks / 2.0);
			flip = !flip;
		}
	}
	while ((ac + acMargin) > acMax)
	{
		ac -= (int32_t) (ra_ticks / 2.0);
		flip = !flip;
	}
	// while on N we would like to see positive values
	if (telLatitude->getValueDouble () > 0)
	{
		while ((ac - acMargin) < acMin)
		{
			ac += (int32_t) (ra_ticks / 2.0);
			flip = !flip;
		}
	}

	if (flip)
		dc += (int32_t) ((90 - dec) * 2 * decCpd);

	if ( (dc < dcMin) || (dc > dcMax) || (ac < acMin) || (ac > acMax))
	{
		logStream (MESSAGE_ERROR) << "didn't find mount position within limits, RA/DEC target "
			<< LibnovaRaDec (pos) << sendLog;
		return -1;
	}
	
	// apply model (some modeling components are not cyclic => we want to use real mount)
	u_pos.ra = ls - ((double) (ac / haCpd) + haZero);
	u_pos.dec = (double) (dc / decCpd) + decZero;

        tt_pos.ra = u_pos.ra;
        tt_pos.dec = u_pos.dec;

	applyModel (&u_pos, &hrz, &tt_pos, &model_change, JD, 0);

        // when on south, change sign (don't take care of flip - we use raw position, applyModel takes it into account)
	if (telLatitude->getValueDouble () < 0)
		model_change.dec *= -1;

	#ifdef DEBUG_EXTRA
	LibnovaRaDec lchange (&model_change);

	logStream (MESSAGE_DEBUG) << "Before model " << ac << dc << lchange << sendLog;
	#endif						 /* DEBUG_EXTRA */

	ac -= -1.0 * (int32_t) (model_change.ra * haCpd);	// -1* is because ac is in HA, not in RA
	dc -= (int32_t) (model_change.dec * decCpd);

	#ifdef DEBUG_EXTRA
	logStream (MESSAGE_DEBUG) << "After model" << ac << dc << sendLog;
	#endif						 /* DEBUG_EXTRA */

	return 0;
}

int Fork::counts2sky (int32_t ac, int32_t dc, double &ra, double &dec, int &flip, double &un_ra, double &un_dec)
{
	double JD, ls, ha;

	JD = ln_get_julian_from_sys ();
	ls = getLstDeg (JD, 0);

	ha = (double) (ac / haCpd) + haZero;
	dec = (double) (dc / decCpd) + decZero;

	ra = ls - ha;

	un_ra = ra;
	un_dec = dec;

	// flipped
	if (fabs (dec) > 90.0)
	{
		flip = 1;
		if (dec > 0)
			dec = 180.0 - dec;
		else
			dec = -180.0 - dec;
		ra += 180.0;
	}
	else
	{
		flip = 0;
	}

	dec = ln_range_degrees (dec);
	if (dec > 180.0)
		dec -= 360.0;

	ra = ln_range_degrees (ra);

	if (telLatitude->getValueDouble () < 0)
	{
		dec *= -1;
		un_dec *= -1;
	}

	return 0;
}

Fork::Fork (int in_argc, char **in_argv, bool diffTrack, bool hasTracking, bool hasUnTelCoordinates, bool parkingBlock):Telescope (in_argc, in_argv, diffTrack, hasTracking, hasUnTelCoordinates ? 1 : 0, false, parkingBlock)
{
	haZero = decZero = haCpd = decCpd = NAN;

	ra_ticks = dec_ticks = 0;
}

Fork::~Fork (void)
{

}
