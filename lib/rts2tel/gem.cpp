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
	bool use_flipped;

	JD = ln_get_julian_from_sys ();

	ret = getHomeOffset (homeOff);
	if (ret)
		return -1;

	getTarget (&pos);

	int used_flipping = useParkFlipping ? parkFlip->getValueInteger () : flipping->getValueInteger ();

	return sky2counts (&pos, ac, dc, JD, homeOff, used_flipping, use_flipped, true);
}

int GEM::sky2counts (struct ln_equ_posn *pos, int32_t & ac, int32_t & dc, double JD, int32_t homeOff, int used_flipping, bool &use_flipped, bool writeValues)
{
	double ls, ha, dec;
	struct ln_hrz_posn hrz;
        struct ln_equ_posn tar_pos;
	int ret;

	// when set to true, will use flipped coordinates
	use_flipped = false;

	if (isnan(pos->ra) || isnan(pos->dec))
	{
		logStream (MESSAGE_ERROR) << "sky2counts called with nan ra/dec" << sendLog;
		return -1;
	}
	ls = getLstDeg (JD);

	ln_get_hrz_from_equ (pos, rts2core::Configuration::instance ()->getObserver (), JD, &hrz);
	if (hrz.alt < -5)
	{
		logStream (MESSAGE_ERROR) << "object is below horizon, azimuth is "
			<< hrz.az  << " and altitude " << hrz.alt  << " RA/DEC targets was " << LibnovaRaDec (pos)
			<< ", check observatory time and location (long & latitude)"
			<< sendLog;
		return -1;
	}

        tar_pos.ra = pos->ra;
        tar_pos.dec = pos->dec;

        // apply corrections
        applyCorrections (&tar_pos, JD, writeValues);
        applyCorrRaDec (&tar_pos, false, false);

	// get hour angle
	ha = ln_range_degrees (ls - tar_pos.ra);
	if (ha > 180.0)
		ha -= 360.0;

	// pretend we are at north hemispehere.. at least for dec
	dec = tar_pos.dec;

	// convert to count values
	int32_t tn_ac = (int32_t) ((ha - haZero->getValueDouble ()) * haCpd->getValueDouble ());
	int32_t tn_dc = (int32_t) ((dec - decZero->getValueDouble ()) * decCpd->getValueDouble ());

	// gets the limits
	ret = updateLimits ();
	if (ret)
	{
		return -1;
	}

	// apply model;
	struct ln_equ_posn f_model_change, n_model_change;
	struct ln_equ_posn tf_pos, tn_pos, tt_pos;

	// if we cannot move with those values, we cannot move with the any other more optional setting, so give up
	int ret_n = normalizeCountValues (ac, dc, tn_ac, tn_dc);

	if (ret_n == 0)
	{
                tn_pos.ra = pos->ra;
                tn_pos.dec = pos->dec;

		// to set telescope target
		tt_pos.ra = ls - ((double) (tn_ac / haCpd->getValueDouble ()) + haZero->getValueDouble ());
		tt_pos.dec = (double) (tn_dc / decCpd->getValueDouble ()) + decZero->getValueDouble ();

		// apply model (some modeling components are not cyclic => we want to use real mount coordinates)
		applyModel (&tn_pos, &tt_pos, &n_model_change, JD);

		tn_ac += (int32_t) (n_model_change.ra * haCpd->getValueDouble ());	// -1* is because ac is in HA, not in RA
		tn_dc -= (int32_t) (n_model_change.dec * decCpd->getValueDouble ());

		if ((tn_dc < dcMin->getValueLong ()) || (tn_dc > dcMax->getValueLong ()))
		{
			logStream (MESSAGE_ERROR) << "non-flipped target declination position is outside limits. RA/DEC target "
				<< LibnovaRaDec (pos) << " dc:" << tn_dc << " dcMin:" << dcMin->getValueLong () << " dcMax:" << dcMax->getValueLong () << sendLog;
			ret_n = -1;
		}

		if ((tn_ac < acMin->getValueLong ()) || (tn_ac > acMax->getValueLong ()))
		{
			logStream (MESSAGE_ERROR) << "non-flipped target RA position is outside limits. RA/DEC target "
				<< LibnovaRaDec (pos) << " ac:" << tn_ac << " acMin:" << acMin->getValueLong () << " acMax:" << acMax->getValueDouble () << sendLog;
			ret_n = -1;
		}
	}

	// let's see what will different flip do..
	int32_t tf_ac = tn_ac - (int32_t) (fabs(haCpd->getValueDouble () * 360.0)/2.0);
	int32_t tf_dc = tn_dc + (int32_t) ((90 - dec) * 2 * decCpd->getValueDouble ());

	int ret_f = normalizeCountValues (ac, dc, tf_ac, tf_dc);

	if (ret_f == 0)
	{
		tf_pos.ra = ln_range_degrees (pos->ra + 180);
		if (getLatitude () > 0)
			tf_pos.dec = 180 - pos->dec;
		else
			tf_pos.dec = -180 - pos->dec;

		// to set telescope target
		tt_pos.ra = ls - ((double) (tf_ac / haCpd->getValueDouble ()) + haZero->getValueDouble ());
		tt_pos.dec = (double) (tf_dc / decCpd->getValueDouble ()) + decZero->getValueDouble ();
	
		// apply model (some modeling components are not cyclic => we want to use real mount coordinates)
		applyModel (&tf_pos, &tt_pos, &f_model_change, JD);

		tf_ac += (int32_t) (f_model_change.ra * haCpd->getValueDouble ());	// -1* is because ac is in HA, not in RA
		tf_dc -= (int32_t) (f_model_change.dec * decCpd->getValueDouble ());

		if ((tf_dc < dcMin->getValueLong ()) || (tf_dc > dcMax->getValueLong ()))
		{
			logStream (MESSAGE_ERROR) << "non-flipped target declination position is outside limits. RA/DEC target "
				<< LibnovaRaDec (pos) << " dc:" << tf_dc << " dcMin:" << dcMin->getValueLong () << " dcMax:" << dcMax->getValueLong () << sendLog;
			ret_n = -1;
		}

		if ((tf_ac < acMin->getValueLong ()) || (tf_ac > acMax->getValueLong ()))
		{
			logStream (MESSAGE_ERROR) << "target RA position is outside limits. RA/DEC target "
				<< LibnovaRaDec (pos) << " ac:" << tf_ac << " acMin:" << acMin->getValueLong () << " acMax:" << acMax->getValueDouble () << sendLog;
			ret_n = -1;
		}
	}

	// there isn't path, give up...
	if (ret_n != 0 && ret_f != 0)
	{
		logStream (MESSAGE_WARNING) << "no way to reach for " << LibnovaRaDec (pos) << ", returning -1" << sendLog;
		return -1;
	}
	// only flipped..
	else if (ret_n != 0 && ret_f == 0)
	{
		use_flipped = true;
	}
	// both ways are possible, decide base on flipping parameter
	else if (ret_n == 0 && ret_f == 0)
	{
#define max(a,b) ((a) > (b) ? (a) : (b))
		switch (used_flipping)
		{
			// shortest
			case 0:
				{
					int32_t diff_nf = max (fabs (ac - tn_ac), fabs (dc - tn_dc));
					int32_t diff_f = max (fabs (ac - tf_ac), fabs (dc - tf_dc));

					if (diff_f < diff_nf)
						use_flipped = true;
				}
				break;
			// same
			case 1:
				if (flip_move_start == 1)
					use_flipped = true;
				break;
			// opposite
			case 2:
				if (flip_move_start == 0)
					use_flipped = true;
				break;
			// west
			case 3:
				break;
			// east
			case 4:
				use_flipped = true;
				break;
			// longest
			case 5:
				{
					switch (flip_longest_path)
					{
						case 0:
							break;
						case 1:
							use_flipped = true;
							break;
						default:
							{
								int32_t diff_nf = max (fabs (ac - tn_ac), fabs (dc - tn_dc));
								int32_t diff_f = max (fabs (ac - tf_ac), fabs (dc - tf_dc));

								if (diff_f > diff_nf)
								{
									use_flipped = true;
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
			// counterweight down
			case 6:
			// counterweight up
			case 7:
				// calculate distance towards "meridian" - counterweight down - position
				// normalize distances by ra_ticks
				double diff_nf = fabs (fmod (getHACWDAngle (tn_ac), 360));
				double diff_f = fabs (fmod (getHACWDAngle (tf_ac), 360));
				// normalize to degree distance to HA
				if (diff_nf > 180)
					diff_nf = 360 - diff_nf;
				if (diff_f > 180)
					diff_f = 360 - diff_f;
				logStream (MESSAGE_DEBUG) << "cw diffs flipped " << diff_f << " nf " << diff_nf << sendLog;
				if (used_flipping == 6)
				{
					if (diff_f < diff_nf)
					{
						use_flipped = true;
					}
				}
				else
				{
					if (diff_f > diff_nf)
					{
						use_flipped = true;
					}
				}
		}

	}
	// otherwise, non-flipped is the only way, stay on it..

	// when we will go to flipped..
	if (use_flipped == true)
	{
		ac = tf_ac;
		dc = tf_dc;
		if (writeValues)
		{
                        tar_pos.ra -= f_model_change.ra;
                        tar_pos.dec -= f_model_change.dec;
                        setTelTarget (tar_pos.ra, tar_pos.dec);
		}
	}
	else
	{
		ac = tn_ac;
		dc = tn_dc;
		if (writeValues)
		{
                        tar_pos.ra -= n_model_change.ra;
                        tar_pos.dec -= n_model_change.dec;
                        setTelTarget (tar_pos.ra, tar_pos.dec);
		}
	}

	ac -= homeOff;

	return 0;
}

int GEM::sky2counts (double JD, struct ln_equ_posn *pos, int32_t &ac, int32_t &dc, bool writeValues)
{
	int used_flipping = useParkFlipping ? parkFlip->getValueInteger () : flipping->getValueInteger ();
        bool use_flipped;

	// returns without home offset, which will be removed in future
	return sky2counts (pos, ac, dc, JD, 0, used_flipping, use_flipped, writeValues);
}

int GEM::counts2sky (int32_t ac, int32_t dc, double &ra, double &dec, int &flip, double &un_ra, double &un_dec, double JD)
{
	double ls, ha;
	int32_t homeOff;
	int ret;

	ret = getHomeOffset (homeOff);
	if (ret)
		return -1;

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
		while (fabs (dec) > 90)
		{
			flip = flip ? 0 : 1;
			if (dec > 0)
				dec = 180.0 - dec;
			else
				dec = -180.0 - dec;
			ra += 180.0;
		}
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
	flipping->addSelVal ("cw down");
	flipping->addSelVal ("cw up");

	createValue (haCWDAngle, "ha_cwd_angle", "[deg] angle between HA axis and local meridian", false, RTS2_DT_DEGREES);
	createValue (targetHaCWDAngle, "tar_ha_cwd_angle", "[deg] target angle between HA axis and local meridian", false, RTS2_DT_DEGREES);

	createValue (peekHaCwdAngle, "peek_ha_cwd_angle", "[deg] peek angle (on peek command) between HA axis and local meridian", false, RTS2_DT_DEGREES);

	createValue (haZeroPos, "_ha_zero_pos", "position of the telescope on zero", false);
	haZeroPos->addSelVal ("EAST");
	haZeroPos->addSelVal ("WEST");

	createValue (haZero, "_ha_zero", "HA zero offset", false);
	createValue (decZero, "_dec_zero", "DEC zero offset", false);

	createValue (haCpd, "_ha_cpd", "HA counts per degree", false);
	createValue (decCpd, "_dec_cpd", "DEC counts per degree", false);

	createValue (acMin, "_ac_min", "HA minimal count value", false);
	createValue (acMax, "_ac_max", "HA maximal count value", false);
	createValue (dcMin, "_dc_min", "DEC minimal count value", false);
	createValue (dcMax, "_dc_max", "DEC maximal count value", false);

	createValue (ra_ticks, "_ra_ticks", "RA ticks per full loop (no effect)", false);
	createValue (dec_ticks, "_dec_ticks", "DEC ticks per full loop (no effect)", false);
}

GEM::~GEM (void)
{

}

int GEM::peek (double ra, double dec)
{
	struct ln_equ_posn peekPos;
	peekPos.ra = ra;
	peekPos.dec = dec;

	bool use_flipped = false;

	int32_t ac;
	int32_t dc;

	double JD = ln_get_julian_from_sys ();

	int ret = calculateTarget (JD, &peekPos, ac, dc, false);
	if (ret)
		return ret;

	peekHaCwdAngle->setValueDouble (getHACWDAngle (ac));
	peekFlip->setValueInteger (use_flipped);

	sendValueAll (peekHaCwdAngle);
	sendValueAll (peekFlip);

	return 0;
}

void GEM::unlockPointing ()
{
	haZeroPos->setWritable ();
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

double GEM::getHACWDAngle (int32_t ha_count)
{
	// sign of haCpd
	int haCpdSign = haCpd->getValueDouble () > 0 ? 1 : -1;
	switch (haZeroPos->getValueInteger ())
	{
		// TODO west (haZeroPos == 1), haCpd >0 is the only proved combination (and haZero was negative, but that should not play any role..).
		// Other values must be confirmed
		case 1:
			return -360.0 * ((ha_count + (haZero->getValueDouble () + haCpdSign * 90) * haCpd->getValueDouble ()) / ra_ticks->getValueDouble ());
		case 0:
		default:
			return 360.0 * ((ha_count + (haZero->getValueDouble () - haCpdSign * 90) * haCpd->getValueDouble ()) / ra_ticks->getValueDouble ());
	}
}

int GEM::normalizeCountValues (int32_t ac, int32_t dc, int32_t &t_ac, int32_t &t_dc)
{
	int32_t full_ac = (int32_t) fabs(haCpd->getValueDouble () * 360.0);
	int32_t full_dc = (int32_t) fabs(decCpd->getValueDouble () * 360.0);

	int32_t half_ac = (int32_t) fabs(haCpd->getValueDouble () * 180.0);
	int32_t half_dc = (int32_t) fabs(decCpd->getValueDouble () * 180.0);

	// minimize movement from current position, don't rotate around axis more than once
	int32_t diff_ac = (ac - t_ac) % full_ac;
	int32_t diff_dc = (dc - t_dc) % full_dc;


	// still, when diff is bigger than half a circle, shorter path is opposite direction what's left
	if (diff_ac > half_ac)
		diff_ac -= full_ac;
	if (diff_dc < -half_ac)
		diff_ac += full_ac;

	if (diff_dc > half_dc)
		diff_dc -= full_dc;
	if (diff_dc < -half_dc)
		diff_dc += full_dc;

	t_ac = ac - diff_ac;
	t_dc = dc - diff_dc;

	// purpose of the following code is to get from west side of flip
	// on S, we prefer negative values
	if (telLatitude->getValueDouble () < 0)
	{
		while ((t_ac - acMargin) < acMin->getValueLong ())
		// ticks per revolution - don't have idea where to get that
		{
			t_ac += (int32_t)fabs(haCpd->getValueDouble () * 360.0);
		}
	}
	while ((t_ac + acMargin) > acMax->getValueLong ())
	{
		t_ac -= (int32_t)fabs(haCpd->getValueDouble () * 360.0);
	}
	// while on N we would like to see positive values
	if (telLatitude->getValueDouble () > 0)
	{
		while ((t_ac - acMargin) < acMin->getValueLong ())
			// ticks per revolution - don't have idea where to get that
		{
			t_ac += (int32_t) fabs(haCpd->getValueDouble () * 360.0);
		}
	}

	// put dc to correct numbers
	while (t_dc < dcMin->getValueLong ())
		t_dc += (int32_t) fabs(decCpd->getValueDouble () * 360.0);
	while (t_dc > dcMax->getValueLong ())
		t_dc -= (int32_t) fabs(decCpd->getValueDouble () * 360.0);

	if ((t_dc < dcMin->getValueLong ()) || (t_dc > dcMax->getValueLong ()))
	{
		return -1;
	}

	if ((t_ac < acMin->getValueLong ()) || (t_ac > acMax->getValueLong ()))
	{
		return -1;
	}

	return 0;
}

int GEM::checkTrajectory (int32_t ac, int32_t dc, int32_t &at, int32_t &dt, int32_t as, int32_t ds, unsigned int steps, double alt_margin, double az_margin, bool ignore_soft_beginning, bool dont_flip)
{
	// nothing to check
	if (hardHorizon == NULL)
		return 0;

	int32_t t_a = ac;
	int32_t t_d = dc;

	int32_t step_a = as;
	int32_t step_d = ds;

	int32_t soft_a;
	int32_t soft_d;

	if (ac > at)
		step_a = -as;

	if (dc > dt)
		step_d = -ds;

	int first_flip = telFlip->getValueInteger ();

	double JD = ln_get_julian_from_sys ();  // fixed time; assuming slew does not take too long, this will work

	// turned to true if we are in "soft" boundaries, e.g hit with margin applied
	bool soft_hit = false;

	for (; steps > 0; steps--)
	{
		// check if still visible
		struct ln_equ_posn pos, un_pos;
		struct ln_hrz_posn hrz;
		int flip = 0;
		int ret;

		int32_t n_a;
		int32_t n_d;

		// if we already reached destionation, e.g. currently computed position is within step to target, don't go further..
		if (labs (t_a - at) < as)
		{
			n_a = at;
			step_a = 0;
		}
		else
		{
			n_a = t_a + step_a;
		}

		if (labs (t_d - dt) < ds)
		{
			n_d = dt;
			step_d = 0;
		}
		else
		{
			n_d = t_d + step_d;
		}

		ret = counts2sky (n_a, n_d, pos.ra, pos.dec, flip, un_pos.ra, un_pos.dec);
		if (ret)
			return -1;

		if (dont_flip == true && first_flip != flip)
		{
			at = n_a;
			dt = n_d;
			return 4;
		}

		ln_get_hrz_from_equ (&pos, rts2core::Configuration::instance ()->getObserver (), JD, &hrz);

		if (soft_hit == true || ignore_soft_beginning == true)
		{
			// if we really cannot go further
			if (hardHorizon->is_good (&hrz) == 0)
			{
				logStream (MESSAGE_DEBUG) << "hit hard limit at alt az " << hrz.alt << " " << hrz.az << " " << soft_a << " " << soft_d << " " << n_a << " " << n_d << sendLog;
				if (soft_hit == true)
				{
					// then use last good position, and return we reached horizon..
					at = soft_a;
					dt = soft_d;
					return 2;
				}
				else
				{
					// case when moving within soft will lead to hard hit..we don't want this path
					at = t_a;
					dt = t_d;
					return 3;
				}
			}
		}

		// we don't need to move anymore, full trajectory is valid
		if (step_a == 0 && step_d == 0)
			return 0;

		if (soft_hit == false)
		{
			// check soft margins..
			if (hardHorizon->is_good_with_margin (&hrz, alt_margin, az_margin) == 0)
			{
				if (ignore_soft_beginning == false)
				{
					soft_hit = true;
					soft_a = t_a;
					soft_d = t_d;
				}
			}
			else
			{
				// we moved away from soft hit region
				if (ignore_soft_beginning == true)
				{
					ignore_soft_beginning = false;
					soft_a = t_a;
					soft_d = t_d;
				}
			}
		}

		t_a = n_a;
		t_d = n_d;
	}

	if (soft_hit == true)
	{
		at = soft_a;
		dt = soft_d;
	}
	else
	{
		at = t_a;
		dt = t_d;
	}

	// we are fine to move at least to the given coordinates
	return 1;
}
