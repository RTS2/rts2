/* 
 * Abstract class for GEMs.
 * Copyright (C) 2007-2008,2010 Petr Kubanek <petr@kubanek.net>
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

#include "gem.h"
#include "configuration.h"

#include "libnova_cpp.h"

using namespace rts2teld;

int GEM::sky2counts (int32_t & ac, int32_t & dc)
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

int GEM::sky2counts (struct ln_equ_posn *pos, int32_t & ac, int32_t & dc, double JD, int32_t homeOff)
{
	double ls, ha, dec;
	struct ln_hrz_posn hrz;
	struct ln_equ_posn model_change;
	struct ln_equ_posn u_pos;
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
	ha = ln_range_degrees (ls - pos->ra);
	if (ha > 180.0)
		ha -= 360.0;

	// pretend we are at north hemispehere.. at least for dec
	dec = pos->dec;
	if (telLatitude->getValueDouble () < 0)
		dec *= -1;

	// convert to count values
	ac = (int32_t) ((ha - haZero->getValueDouble ()) * haCpd->getValueDouble ());
	dc = (int32_t) ((dec - decZero->getValueDouble ()) * decCpd->getValueDouble ());

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
		while ((ac - acMargin) < acMin->getValueLong ())
			// ticks per revolution - don't have idea where to get that
		{
			ac += (int32_t) (ra_ticks->getValueLong () / 2.0);
			flip = !flip;
		}
	}
	while ((ac + acMargin) > acMax->getValueLong ())
	{
		ac -= (int32_t) (ra_ticks->getValueLong () / 2.0);
		flip = !flip;
	}
	// while on N we would like to see positive values
	if (telLatitude->getValueDouble () > 0)
	{
		while ((ac - acMargin) < acMin->getValueLong ())
			// ticks per revolution - don't have idea where to get that
		{
			ac += (int32_t) (ra_ticks->getValueLong () / 2.0);
			flip = !flip;
		}
	}

	if (flip)
		dc += (int32_t) ((90 - dec) * 2 * decCpd->getValueDouble ());

	// put dc to correct numbers
	while (dc < dcMin->getValueLong ())
		dc += dec_ticks->getValueLong ();
	while (dc > dcMax->getValueLong ())
		dc -= dec_ticks->getValueLong ();

	if ((dc < dcMin->getValueLong ()) || (dc > dcMax->getValueLong ()))
	{
		logStream (MESSAGE_ERROR) << "target declination position is outside limits. RA/DEC target "
			<< LibnovaRaDec (pos) << " dc:" << dc << " dcMin:" << dcMin << " dcMax:" << dcMax << sendLog;
		return -1;
	}

	if ((ac < acMin->getValueLong ()) || (ac > acMax->getValueLong ()))
	{
		logStream (MESSAGE_ERROR) << "target RA position is outside limits. RA/DEC target "
			<< LibnovaRaDec (pos) << " ac:" << ac << " acMin:" << acMin << " acMax:" << acMax << sendLog;
		return -1;
	}

	// apply model (some modeling components are not cyclic => we want to use real mount coordinates)
	u_pos.ra = ls - ((double) (ac / haCpd->getValueDouble ()) + haZero->getValueDouble ());
	u_pos.dec = (double) (dc / decCpd->getValueDouble ()) + decZero->getValueDouble ();
	if (telLatitude->getValueDouble () < 0)
		u_pos.dec *= -1;
	applyModel (&u_pos, &model_change, 0, JD);	// we give raw (unflipped) position => flip=0 for model computation

	// when on south, change sign (don't take care of flip - we use raw position, applyModel takes it into account)
	if (telLatitude->getValueDouble () < 0)
		model_change.dec *= -1;

	#ifdef DEBUG_EXTRA
	LibnovaRaDec lchange (&model_change);

	logStream (MESSAGE_DEBUG) << "Before model " << ac << dc << lchange << sendLog;
	#endif						 /* DEBUG_EXTRA */

	ac -= -1.0 * (int32_t) (model_change.ra * haCpd->getValueDouble ());	// -1* is because ac is in HA, not in RA
	dc -= (int32_t) (model_change.dec * decCpd->getValueDouble ());

	#ifdef DEBUG_EXTRA
	logStream (MESSAGE_DEBUG) << "After model" << ac << dc << sendLog;
	#endif						 /* DEBUG_EXTRA */

	ac -= homeOff;

	return 0;
}

int GEM::counts2sky (int32_t ac, int32_t dc, double &ra, double &dec, int &flip, double &un_ra, double &un_dec)
{
	double JD, ls, ha;
	int32_t homeOff;
	int ret;

	ret = getHomeOffset (homeOff);
	if (ret)
		return -1;

	JD = ln_get_julian_from_sys ();
	ls = getLstDeg (JD);

	ac += homeOff;

	ha = (double) (ac / haCpd->getValueDouble ()) + haZero->getValueDouble ();
	dec = (double) (dc / decCpd->getValueDouble ()) + decZero->getValueDouble ();

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

GEM::GEM (int in_argc, char **in_argv, bool diffTrack, bool hasTracking, bool hasUnTelCoordinates):Telescope (in_argc, in_argv, diffTrack, hasTracking, hasUnTelCoordinates)
{
	createValue (haZero, "_ha_zero", "HA zero offset", false);
	createValue (decZero, "_dec_zero", "DEC zero offset", false);

	createValue (haCpd, "_ha_cpd", "HA counts per degree", false);
	createValue (decCpd, "_dec_cpd", "DEC counts per degree", false);

	createValue (acMin, "_ac_min", "HA minimal count value", false);
	createValue (acMax, "_ac_max", "HA maximal count value", false);
	createValue (dcMin, "_dc_min", "DEC minimal count value", false);
	createValue (dcMax, "_dc_max", "DEC maximal count value", false);

	createValue (ra_ticks, "_ra_ticks", "RA ticks per full loop", false);
	createValue (dec_ticks, "_dec_ticks", "DEC ticks per full loop", false);
}

GEM::~GEM (void)
{

}

void GEM::unlockPointing ()
{
	haZero->setWritable ();
	decZero->setWritable ();
	haCpd->setWritable ();
	decCpd->setWritable ();

	acMin->setWritable ();
	acMax->setWritable ();
	dcMin->setWritable ();
	dcMax->setWritable ();

	ra_ticks->setWritable ();
	dec_ticks->setWritable ();
	
	updateMetaInformations (haZero);
	updateMetaInformations (decZero);
	updateMetaInformations (haCpd);	
	updateMetaInformations (decCpd);

	updateMetaInformations (acMin);	
	updateMetaInformations (acMax);	
	updateMetaInformations (dcMin);	
	updateMetaInformations (dcMax);

	updateMetaInformations (ra_ticks);
	updateMetaInformations (dec_ticks);
}
