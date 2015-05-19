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
	int ret;

	int32_t t_ac, t_dc;

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
	t_ac = (int32_t) ((ha - haZero->getValueDouble ()) * haCpd->getValueDouble ());
	t_dc = (int32_t) ((dec - decZero->getValueDouble ()) * decCpd->getValueDouble ());

	// gets the limits
	ret = updateLimits ();
	if (ret)
	{
		return -1;
	}

	// if we cannot move with those values, we cannot move with the any other more optiomal setting, so give up
	ret = checkCountValues (pos, ac, dc, t_ac, t_dc, JD, ls, dec);

	// let's see what will different flip do..
	int32_t tf_ac = t_ac - ra_ticks->getValueLong () / 2.0;
	int32_t tf_dc = t_dc + (int32_t) ((90 - dec) * 2 * decCpd->getValueDouble ());

	int ret_f = checkCountValues (pos, ac, dc, tf_ac, tf_dc, JD, ls, dec);

	// there isn't path, give up...
	if (ret != 0 && ret_f != 0)
		return -1;
	// only flipped..
	else if (ret != 0 && ret_f == 0)
	{
		t_ac = tf_ac;
		t_dc = tf_dc;
	}
	// both ways are possible, decide base on flipping parameter
	else if (ret == 0 && ret_f == 0)
	{
#define max(a,b) ((a) > (b) ? (a) : (b))
		switch (flipping->getValueInteger ())
		{
			// shortest
			case 0:
				{
					int32_t diff_nf = max (fabs (ac - t_ac), fabs (dc - t_dc));
					int32_t diff_f = max (fabs (ac - tf_ac), fabs (dc - tf_dc));

					if (diff_f < diff_nf)
					{
						t_ac = tf_ac;
						t_dc = tf_dc;
					}
				}
				break;
			// same
			case 1:
				if (flip_move_start == 1)
				{
					t_ac = tf_ac;
					t_dc = tf_dc;
				}
				break;
			// opposite
			case 2:
				if (flip_move_start == 0)
				{
					t_ac = tf_ac;
					t_dc = tf_dc;
				}
				break;
			// west
			case 3:
				break;
			// east
			case 4:
				t_ac = tf_ac;
				t_dc = tf_dc;
				break;
			// longest
			case 5:
				{
					switch (flip_longest_path)
					{
						case 0:
							break;
						case 1:
							t_ac = tf_ac;
							t_dc = tf_dc;
							break;
						default:
							{
								int32_t diff_nf = max (fabs (ac - t_ac), fabs (dc - t_dc));
								int32_t diff_f = max (fabs (ac - tf_ac), fabs (dc - tf_dc));

								if (diff_f > diff_nf)
								{
									t_ac = tf_ac;
									t_dc = tf_dc;
									flip_longest_path = 1;
								}
								else
								{
									flip_longest_path = 0;
								}
							}
					}
				}
				break;
		}
	}
	// otherwise, non-flipped is the only way, stay on it..

	t_ac -= homeOff;

	ac = t_ac;
	dc = t_dc;

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
	createValue (flipping, "FLIPPING", "mount flipping strategy", false, RTS2_VALUE_WRITABLE);
	flipping->addSelVal ("shortest");
	flipping->addSelVal ("same");
	flipping->addSelVal ("opposite");
	flipping->addSelVal ("west");
	flipping->addSelVal ("east");
	flipping->addSelVal ("longest");

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

int GEM::checkCountValues (struct ln_equ_posn *pos, int32_t ac, int32_t dc, int32_t &t_ac, int32_t &t_dc, double JD, double ls, double dec)
{
	struct ln_equ_posn model_change;
	struct ln_equ_posn u_pos;

	// minimize movement from current position, don't rotate around axis more then once
	int32_t diff_ac = (ac - t_ac) % ra_ticks->getValueLong ();
	int32_t diff_dc = (dc - t_dc) % dec_ticks->getValueLong ();

	t_ac = ac - diff_ac;
	t_dc = dc - diff_dc;

	// purpose of the following code is to get from west side of flip
	// on S, we prefer negative values
	if (telLatitude->getValueDouble () < 0)
	{
		while ((t_ac - acMargin) < acMin->getValueLong ())
		// ticks per revolution - don't have idea where to get that
		{
			t_ac += ra_ticks->getValueLong ();
		}
	}
	while ((t_ac + acMargin) > acMax->getValueLong ())
	{
		t_ac -= (int32_t) ra_ticks->getValueLong ();
	}
	// while on N we would like to see positive values
	if (telLatitude->getValueDouble () > 0)
	{
		while ((t_ac - acMargin) < acMin->getValueLong ())
			// ticks per revolution - don't have idea where to get that
		{
			t_ac += (int32_t) ra_ticks->getValueLong ();
		}
	}

	// put dc to correct numbers
	while (t_dc < dcMin->getValueLong ())
		t_dc += dec_ticks->getValueLong ();
	while (t_dc > dcMax->getValueLong ())
		t_dc -= dec_ticks->getValueLong ();

	if ((t_dc < dcMin->getValueLong ()) || (t_dc > dcMax->getValueLong ()))
	{
		logStream (MESSAGE_ERROR) << "target declination position is outside limits. RA/DEC target "
			<< LibnovaRaDec (pos) << " dc:" << t_dc << " dcMin:" << dcMin->getValueLong () << " dcMax:" << dcMax->getValueLong () << sendLog;
		return -1;
	}

	if ((t_ac < acMin->getValueLong ()) || (t_ac > acMax->getValueLong ()))
	{
		logStream (MESSAGE_ERROR) << "target RA position is outside limits. RA/DEC target "
			<< LibnovaRaDec (pos) << " ac:" << t_ac << " acMin:" << acMin->getValueLong () << " acMax:" << acMax->getValueDouble () << sendLog;
		return -1;
	}

	// apply model (some modeling components are not cyclic => we want to use real mount coordinates)
	u_pos.ra = ls - ((double) (t_ac / haCpd->getValueDouble ()) + haZero->getValueDouble ());
	u_pos.dec = (double) (t_dc / decCpd->getValueDouble ()) + decZero->getValueDouble ();
	if (telLatitude->getValueDouble () < 0)
		u_pos.dec *= -1;
	applyModel (&u_pos, &model_change, 0, JD);	// we give raw (unflipped) position => flip=0 for model computation

	// when on south, change sign (don't take care of flip - we use raw position, applyModel takes it into account)
	if (telLatitude->getValueDouble () < 0)
		model_change.dec *= -1;

	#ifdef DEBUG_EXTRA
	LibnovaRaDec lchange (&model_change);

	logStream (MESSAGE_DEBUG) << "Before model " << t_ac << t_dc << lchange << sendLog;
	#endif						 /* DEBUG_EXTRA */

	t_ac -= -1.0 * (int32_t) (model_change.ra * haCpd->getValueDouble ());	// -1* is because ac is in HA, not in RA
	t_dc -= (int32_t) (model_change.dec * decCpd->getValueDouble ());

	#ifdef DEBUG_EXTRA
	logStream (MESSAGE_DEBUG) << "After model" << t_ac << t_dc << sendLog;
	#endif						 /* DEBUG_EXTRA */

	return 0;
}
