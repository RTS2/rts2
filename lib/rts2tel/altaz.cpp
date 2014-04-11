/*
 * Abstract class for Alt-Az mounts.
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

#include "altaz.h"
#include "configuration.h"

#include "libnova_cpp.h"

using namespace rts2teld;

int AltAZ::sky2counts (int32_t & ac, int32_t & dc)
{
	double JD;
	int32_t homeOff;
	struct ln_equ_posn pos;
	int ret;

	JD = ln_get_julian_from_sys ();

	ret = getHomeOffset (homeOff);
	if (ret)
		return -1;

	getTarget (&pos);

	return sky2counts (&pos, ac, dc, JD, homeOff);
}

int AltAz::sky2counts (struct ln_equ_posn *pos, int32_t & ac, int32_t & dc, double JD, int32_t homeOff)
{
	double ls, ra, dec;
	struct ln_hrz_posn hrz;
	struct ln_equ_posn model_change;
	int ret;
	bool flip = false;

	if (isnan(pos->ra) || isnan(pos->dec))
	{
		logStream (MESSAGE_ERROR) << "sky2counts called with nan ra/dec" << sendLog;
		return -1;
	}
	ls = getLstDeg (JD);

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
	ra = ln_range_degrees (ls - pos->ra);
	if (ra > 180.0)
		ra -= 360.0;

	// pretend we are at north hemispehere.. at least for dec
	dec = pos->dec;
	if (telLatitude->getValueDouble () < 0)
		dec *= -1;

	dec = dec - decZero;

	// convert to ac; ra now holds HA
	ac = (int32_t) ((ra + haZero) * haCpd);
	dc = (int32_t) (dec * decCpd);

	// apply model
	applyModel (pos, &model_change, flip, JD);

	// when fliped, change sign
	if ((flip && telLatitude->getValueDouble () < 0)
		|| (!flip && telLatitude->getValueDouble () > 0))
		model_change.dec *= -1;

	#ifdef DEBUG_EXTRA
	LibnovaRaDec lchange (&model_change);

	logStream (MESSAGE_DEBUG) << "Before model " << ac << dc << lchange <<
		sendLog;
	#endif						 /* DEBUG_EXTRA */

	ac += (int32_t) (model_change.ra * haCpd);
	dc += (int32_t) (model_change.dec * decCpd);

	#ifdef DEBUG_EXTRA
	logStream (MESSAGE_DEBUG) << "After model" << ac << dc << sendLog;
	#endif						 /* DEBUG_EXTRA */

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

	ac -= homeOff;

	return 0;
}

int Fork::counts2sky (int32_t & ac, int32_t dc, double &ra, double &dec)
{
	double JD, ls;
	int32_t homeOff;
	int ret;

	ret = getHomeOffset (homeOff);
	if (ret)
		return -1;

	JD = ln_get_julian_from_sys ();
	ls = getLstDeg (JD);

	ac += homeOff;

	ra = (double) (ac / haCpd) - haZero;
	dec = (double) (dc / decCpd) + decZero;

	ra = ls - ra;

	// flipped
	if (fabs (dec) > 90)
	{
		telFlip->setValueInteger (1);
		if (dec > 0)
			dec = 180 - dec;
		else
			dec = -180 - dec;
		ra += 180;
		ac += (int32_t) (ra_ticks / 2.0);
	}
	else
	{
		telFlip->setValueInteger (0);
	}

	while (ac < acMin)
		ac += ra_ticks;
	while (ac > acMax)
		ac -= ra_ticks;

	dec = ln_range_degrees (dec);
	if (dec > 180.0)
		dec -= 360.0;

	ra = ln_range_degrees (ra);

	if (telLatitude->getValueDouble () < 0)
		dec *= -1;

	return 0;
}

Fork::Fork (int in_argc, char **in_argv, bool diffTrack, bool hasTracking):Telescope (in_argc, in_argv, diffTrack, hasTracking)
{
	haZero = decZero = haCpd = decCpd = NAN;

	ra_ticks = dec_ticks = 0;
}

Fork::~Fork (void)
{

}
