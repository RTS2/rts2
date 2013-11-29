/* 
 * Various targets classes.
 * Copyright (C) 2003-2007 Petr Kubanek <petr@kubanek.net>
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

#include <libnova/libnova.h>
#include "configuration.h"
#include "timestamp.h"
#include "infoval.h"
#include "utilsfunc.h"

#include "rts2db/plan.h"
#include "rts2db/target.h"
#include "rts2db/sqlerror.h"

#include <iomanip>
#include <sstream>

EXEC SQL include sqlca;

using namespace rts2db;

// ConstTarget
ConstTarget::ConstTarget () : Target ()
{
	position.ra = position.dec = 0;
	proper_motion.ra = proper_motion.dec = NAN;
}

ConstTarget::ConstTarget (int in_tar_id, struct ln_lnlat_posn *in_obs):Target (in_tar_id, in_obs)
{
	position.ra = position.dec = 0;
	proper_motion.ra = proper_motion.dec = NAN;
}

ConstTarget::ConstTarget (int in_tar_id, struct ln_lnlat_posn *in_obs, struct ln_equ_posn *pos):Target (in_tar_id, in_obs)
{
	position.ra = pos->ra;
	position.dec = pos->dec;
	proper_motion.ra = proper_motion.dec = NAN;
}

void ConstTarget::load ()
{
	EXEC SQL BEGIN DECLARE SECTION;
	double d_ra;
	double d_dec;
	double d_pm_ra;
	double d_pm_dec;
	int d_ra_ind;
	int d_dec_ind;
	int d_pm_ra_ind;
	int d_pm_dec_ind;
	int db_tar_id = getObsTargetID ();
	EXEC SQL END DECLARE SECTION;

	EXEC SQL
	SELECT
		tar_ra,
		tar_dec,
		tar_pm_ra,
		tar_pm_dec
	INTO
		:d_ra :d_ra_ind,
		:d_dec :d_dec_ind,
		:d_pm_ra :d_pm_ra_ind,
		:d_pm_dec :d_pm_dec_ind
	FROM
		targets
	WHERE
		tar_id = :db_tar_id;
	if (sqlca.sqlcode)
	{
	  	std::ostringstream err;
		err << "cannot load constant target with ID " << db_tar_id;
		throw SqlError (err.str ().c_str ());
	}

	if (d_ra_ind)
		d_ra = NAN;
	if (d_dec_ind)
	  	d_dec = NAN;
	if (d_pm_ra_ind)
	  	d_pm_ra = NAN;
	if (d_pm_dec_ind)
		d_pm_dec = NAN;

	position.ra = d_ra;
	position.dec = d_dec;

	proper_motion.ra = d_pm_ra;
	proper_motion.dec = d_pm_dec;

	Target::load ();
}

int ConstTarget::saveWithID (bool overwrite, int tar_id)
{
	EXEC SQL BEGIN DECLARE SECTION;
	double d_tar_ra;
	double d_tar_dec;
	double d_tar_pm_ra;
	double d_tar_pm_dec;
	int d_tar_id;
	EXEC SQL END DECLARE SECTION;

	int ret;

	ret = Target::saveWithID (overwrite, tar_id);
	if (ret)
		return ret;

	d_tar_ra = position.ra;
	d_tar_dec = position.dec;

	d_tar_pm_ra = proper_motion.ra;
	d_tar_pm_dec = proper_motion.dec;

	d_tar_id = tar_id;

	EXEC SQL
		UPDATE
			targets
		SET
			tar_ra = :d_tar_ra,
			tar_dec = :d_tar_dec,
			tar_pm_ra = :d_tar_pm_ra,
			tar_pm_dec = :d_tar_pm_dec
		WHERE
			tar_id = :d_tar_id;

	if (sqlca.sqlcode)
	{
		logMsgDb ("ConstTarget::saveWithID", MESSAGE_ERROR);
		EXEC SQL ROLLBACK;
		return -1;
	}
	EXEC SQL COMMIT;
	return 0;
}

void ConstTarget::getPosition (struct ln_equ_posn *pos, double JD)
{
	*pos = position;
	if (!isnan (proper_motion.ra) && !isnan (proper_motion.dec))
		ln_get_equ_pm (pos, &proper_motion, JD, pos);
}

void ConstTarget::getProperMotion (struct ln_equ_posn *pm)
{
	pm->ra = proper_motion.ra;
	pm->dec = proper_motion.dec;
}

int ConstTarget::getRST (struct ln_rst_time *rst, double JD, double horizon)
{
	struct ln_equ_posn pos;

	getPosition (&pos, JD);
	return ln_get_object_next_rst_horizon (JD, observer, &pos, horizon, rst);
}

int ConstTarget::selectedAsGood ()
{
	EXEC SQL BEGIN DECLARE SECTION;
		int d_tar_id = target_id;
		double d_tar_ra;
		double d_tar_dec;
	EXEC SQL END DECLARE SECTION;
	// check if we are still enabled..
	EXEC SQL
		SELECT
			tar_ra,
			tar_dec
		INTO
			:d_tar_ra,
			:d_tar_dec
		FROM
			targets
		WHERE
			tar_id = :d_tar_id;
	if (sqlca.sqlcode)
	{
		logStream (MESSAGE_ERROR) << "error while loading target positions for target with it ID " << d_tar_id << " observation target ID " << getObsTargetID () << sendLog;
		logMsgDb ("ConstTarget::selectedAsGood", MESSAGE_ERROR);
		return -1;
	}
	position.ra = d_tar_ra;
	position.dec = d_tar_dec;
	return Target::selectedAsGood ();
}

int ConstTarget::compareWithTarget (Target * in_target, double in_sep_limit)
{
	struct ln_equ_posn other_position;
	in_target->getPosition (&other_position);
	return (getDistance (&other_position) < in_sep_limit);
}

void ConstTarget::printExtra (Rts2InfoValStream &_os, double JD)
{
	Target::printExtra (_os, JD);
	_os
		<< InfoVal <LibnovaDegArcMin> ("PROPER MOTION RA", LibnovaDegArcMin (proper_motion.ra)) 
		<< InfoVal <LibnovaDegArcMin> ("PROPER MOTION DEC", LibnovaDegArcMin (proper_motion.dec));
}

DarkTarget::DarkTarget (int in_tar_id, struct ln_lnlat_posn *in_obs): Target (in_tar_id, in_obs)
{
	currPos.ra = NAN;
	currPos.dec = NAN;
	moveFailed ();
}

DarkTarget::~DarkTarget ()
{
}

bool DarkTarget::getScript (const char *deviceName, std::string &buf)
{
 	try
	{
		getDBScript (deviceName, buf);
		return false;
	}
	catch (rts2core::Error &er)
	{
	}
	// otherwise, take bias frames
	buf = std::string ("D 0");
	return false;
}

void DarkTarget::getPosition (struct ln_equ_posn *pos, double JD)
{
	*pos = currPos;
}

moveType DarkTarget::startSlew (struct ln_equ_posn *position, bool update_position, int plan_id)
{
	currPos.ra = position->ra;
	currPos.dec = position->dec;
	// when telescope isn't initialized to point when we get ra | dec - put some defaults
	if (isnan (currPos.ra))
		currPos.ra = -999;
	if (isnan (currPos.dec))
		currPos.dec = -999;
	Target::startSlew (position, update_position, plan_id);
	return OBS_DONT_MOVE;
}

FlatTarget::FlatTarget (int in_tar_id, struct ln_lnlat_posn *in_obs): ConstTarget (in_tar_id, in_obs)
{
	setTargetName ("flat target");
}

void FlatTarget::getAntiSolarPos (struct ln_equ_posn *pos, double JD)
{
	struct ln_equ_posn eq_sun;
	struct ln_hrz_posn hrz;
	ln_get_solar_equ_coords (JD, &eq_sun);
	ln_get_hrz_from_equ (&eq_sun, observer, JD, &hrz);
	hrz.alt = 60;
	hrz.az = ln_range_degrees (hrz.az + 180);
	ln_get_equ_from_hrz (&hrz, observer, JD, pos);
}

bool FlatTarget::getScript (const char *deviceName, std::string &buf)
{
	try
	{
		ConstTarget::getDBScript (deviceName, buf);
		return false;
	}
	catch (rts2core::Error &er)
	{
	}
	// default string
	buf = std::string ("E 1");
	return false;
}

// we will try to find target, that is among empty fields, and is at oposite location from sun
// that target will then become our target_id, so entries in observation log
// will refer to that id, not to generic flat target_id
void FlatTarget::load ()
{
	EXEC SQL BEGIN DECLARE SECTION;
		double d_tar_ra;
		double d_tar_dec;
		int d_tar_id;
		int db_target_flat = TARGET_FLAT;
	EXEC SQL END DECLARE SECTION;

	if (getTargetID () != TARGET_FLAT)
	{
		ConstTarget::load ();
		return;
	}

	double minAntiDist = 1000;
	double curDist;
	struct ln_equ_posn d_tar;
	struct ln_equ_posn antiSolarPosition;
	struct ln_hrz_posn hrz;
	double JD;

	JD = ln_get_julian_from_sys ();

	getAntiSolarPos (&antiSolarPosition, JD);

	EXEC SQL DECLARE flat_targets CURSOR FOR
		SELECT
			tar_ra,
			tar_dec,
			tar_id
		FROM
			targets
		WHERE
			type_id = 'f'
		AND (tar_bonus_time is NULL OR tar_bonus > 0)
		AND tar_enabled = true
		AND tar_id <> :db_target_flat;
	EXEC SQL OPEN flat_targets;
	while (1)
	{
		EXEC SQL FETCH next FROM flat_targets INTO
				:d_tar_ra,
				:d_tar_dec,
				:d_tar_id;
		if (sqlca.sqlcode)
			break;
		d_tar.ra = d_tar_ra;
		d_tar.dec = d_tar_dec;
		// we should be at least 10 deg above horizon to be considered..
		ln_get_hrz_from_equ (&d_tar, observer, JD, &hrz);
		if (hrz.alt < rts2core::Configuration::instance ()->getMinFlatHeigh ())
			continue;
		// and of course we should be above horizon..
		if (!isAboveHorizon (&hrz))
			continue;
		// test if we found the best target..
		curDist = ln_get_angular_separation (&d_tar, &antiSolarPosition);
		if (curDist < minAntiDist)
		{
			// check if it is too far from moon..
			obs_target_id = d_tar_id;
			minAntiDist = curDist;
		}
	}
	if ((sqlca.sqlcode && sqlca.sqlcode != ECPG_NOT_FOUND)
		|| obs_target_id <= 0)
	{
		logMsgDb ("FlatTarget::load", MESSAGE_DEBUG);
		EXEC SQL CLOSE flat_targets;
		//in that case, we will simply use generic flat target..
		return;
	}
	EXEC SQL CLOSE flat_targets;
	ConstTarget::load ();
}

void FlatTarget::getPosition (struct ln_equ_posn *pos, double JD)
{
	// we were loaded, or we aren't generic flat target
	if (obs_target_id > 0 || getTargetID () != TARGET_FLAT)
		return ConstTarget::getPosition (pos, JD);
	// generic flat target observations
	getAntiSolarPos (pos, JD);
}

int FlatTarget::considerForObserving (double JD)
{
	// get new position..when new is available..
	load ();
	// still return considerForObserving..is there is nothing, then get -1 so selector will delete us
	return ConstTarget::considerForObserving (JD);
}

void FlatTarget::printExtra (Rts2InfoValStream & _os, double JD)
{
	ConstTarget::printExtra (_os, JD);
	struct ln_equ_posn antisol;
	getAntiSolarPos (&antisol, JD);
	struct ln_equ_posn pos;
	getPosition (&pos, JD);
	_os
		<< InfoVal<LibnovaDeg180> ("ANTISOL_DIST", ln_get_angular_separation (&pos, &antisol))
		<< std::endl;
}

CalibrationTarget::CalibrationTarget (int in_tar_id, struct ln_lnlat_posn *in_obs):ConstTarget (in_tar_id, in_obs)
{
	airmassPosition.ra = airmassPosition.dec = 0;
	time (&lastImage);
	needUpdate = 1;
}

// the idea is to cover uniformly whole sky.
// in airmass_cal_images table we have recorded previous observations
// for frames with astrometry which contains targeted airmass
void CalibrationTarget::load ()
{
	EXEC SQL BEGIN DECLARE SECTION;
	double d_airmass_start;
	double d_airmass_end;
	long d_airmass_last_image;

	double db_tar_ra;
	double db_tar_dec;
	int db_tar_id;
	char db_type_id;
	// cannot use TARGET_NAME_LEN, as it does not work with some ecpg veriosn
	VARCHAR db_tar_name[150];
	EXEC SQL END DECLARE SECTION;

	double JD = ln_get_julian_from_sys ();

	time_t now;
	time_t valid;

	PosCalibration *fallback_obs_calib = NULL;
	time_t fallback_last_image = 0;
	double fallback_last_time = 0;

	std::list <PosCalibration *> cal_list;
	std::list <PosCalibration *> bad_list;
	std::list <PosCalibration *>::iterator cal_iter, cal_iter2;
	if (getTargetID () != TARGET_CALIBRATION)
	{
		ConstTarget::load ();
		return;
	}

	// create airmass & target_id pool (I dislike idea of creating
	// target object, as that will cost me a lot of resources
	EXEC SQL DECLARE pos_calibration CURSOR FOR
	SELECT
		tar_ra,
		tar_dec,
		tar_id,
		type_id,
		tar_name
	FROM
		targets
	WHERE
		tar_enabled = true
		AND (tar_bonus_time is NULL OR tar_bonus > 0)
		AND ((tar_next_observable is null) OR (tar_next_observable < now()))
		AND tar_id != 6
		AND (
			type_id = 'c'
			OR type_id = 'M'
		)
	ORDER BY
		tar_priority + tar_bonus desc;
	EXEC SQL OPEN pos_calibration;
	while (1)
	{
		EXEC SQL FETCH next FROM pos_calibration
		INTO
			:db_tar_ra,
			:db_tar_dec,
			:db_tar_id,
			:db_type_id,
			:db_tar_name;
		if (sqlca.sqlcode)
			break;
		struct ln_equ_posn pos;
		pos.ra = db_tar_ra;
		pos.dec = db_tar_dec;
		struct ln_hrz_posn hrz;
		ln_get_hrz_from_equ (&pos, observer, JD, &hrz);
		if (rts2core::Configuration::instance ()->getObjectChecker ()->is_good (&hrz))
		{
			PosCalibration *newCal = new PosCalibration (db_tar_id, db_tar_ra, db_tar_dec, db_type_id,
				db_tar_name.arr, observer, JD);
			cal_list.push_back (newCal);
		}
	}
	if (sqlca.sqlcode != ECPG_NOT_FOUND)
	{
		// free cal_list..
		for (cal_iter = cal_list.begin (); cal_iter != cal_list.end ();)
		{
			cal_iter2 = cal_iter;
			cal_iter++;
			delete *cal_iter2;
		}
		cal_list.clear ();
		EXEC SQL CLOSE pos_calibration;
		EXEC SQL ROLLBACK;
		throw SqlError ("cannot find any possible callibration target");
	}
	if (cal_list.size () == 0)
	{
		logStream (MESSAGE_ERROR) << "there aren't any calibration targets; either create them or delete target with ID 6" << sendLog;
		EXEC SQL CLOSE pos_calibration;
		EXEC SQL ROLLBACK;
		return;
	}
	EXEC SQL CLOSE pos_calibration;
	EXEC SQL COMMIT;

	// center airmass is 1.5 - when we don't have any images in airmass_cal_images,
	// order us by distance from such center distance
	EXEC SQL DECLARE cur_airmass_cal_images CURSOR FOR
	SELECT
		air_airmass_start,
		air_airmass_end,
		EXTRACT (EPOCH FROM air_last_image)
	FROM
		airmass_cal_images
	ORDER BY
		air_last_image asc,
		abs (1.5 - (air_airmass_start + air_airmass_end) / 2) asc;
	EXEC SQL OPEN cur_airmass_cal_images;
	obs_target_id = -1;
	time (&now);
	valid = now - rts2core::Configuration::instance()->getCalibrationValidTime();
	while (1)
	{
		EXEC SQL FETCH next FROM cur_airmass_cal_images
		INTO
			:d_airmass_start,
			:d_airmass_end,
			:d_airmass_last_image;
		if (sqlca.sqlcode)
			break;
		// find any target which lies within requested airmass range
		for (cal_iter = cal_list.begin (); cal_iter != cal_list.end (); cal_iter++)
		{
			PosCalibration *calib = *cal_iter;
			if (calib->getCurrAirmass () >= d_airmass_start
				&& calib->getCurrAirmass () < d_airmass_end)
			{
				if (calib->getLunarDistance (JD) < rts2core::Configuration::instance()->getCalibrationLunarDist())
				{
					bad_list.push_back (calib);
				}
				// if that target was already observerd..
				else if (calib->getNumObs (&valid, &now) > 0)
				{
					// if we do not have any target, pick that on
					double calib_last_time = calib->getLastObsTime ();
					if (fallback_obs_calib == NULL ||
						calib_last_time > fallback_last_time)
					{
						fallback_obs_calib = calib;
						fallback_last_image = (time_t) d_airmass_last_image;
						fallback_last_time = calib_last_time;
					}
				}
				else
				{
					// switch current target coordinates..
					obs_target_id = calib->getTargetID ();
					calib->getPosition (&airmassPosition, JD);
					lastImage = (time_t) d_airmass_last_image;
					break;
				}
			}
		}
		if (obs_target_id != -1)
			break;
		std::ostringstream _os;
		_os << "CalibrationTarget::load cannot find any target for airmass between "
			<< d_airmass_start << " and " << d_airmass_end ;
		logMsgDb (_os.str ().c_str (), MESSAGE_DEBUG);
	}
	// change priority for bad targets..
	for (cal_iter = bad_list.begin (); cal_iter != bad_list.end (); cal_iter++)
	{
		(*cal_iter)->setNextObservable (JD + 1.0/24.0);
	}
	bad_list.clear ();
	// no target found..try fallback
	if (obs_target_id == -1 && fallback_obs_calib != NULL)
	{
		obs_target_id = fallback_obs_calib->getTargetID ();
		fallback_obs_calib->getPosition (&airmassPosition, JD);
		lastImage = fallback_last_image;
	}
	// free cal_list..
	for (cal_iter = cal_list.begin (); cal_iter != cal_list.end ();)
	{
		cal_iter2 = cal_iter;
		cal_iter++;
		delete *cal_iter2;
	}
	cal_list.clear ();

	// SQL test..
	if (sqlca.sqlcode)
	{
		logMsgDb ("CalibrationTarget::load cannot find any airmass_cal_images entry",
			(sqlca.sqlcode == ECPG_NOT_FOUND) ? MESSAGE_DEBUG : MESSAGE_ERROR);
		// don't return, as we can have fallback target, which loaded
		// properly
	}
	EXEC SQL CLOSE cur_airmass_cal_images;
	EXEC SQL COMMIT;
	if (obs_target_id != -1)
	{
		needUpdate = 0;
		ConstTarget::load ();
		return;
	}
	throw SqlError ("cannot locate any calibration target");
}

int CalibrationTarget::beforeMove ()
{
	// as calibration target can change between time we select it, let's reload us
	if (getTargetID () == TARGET_CALIBRATION && needUpdate)
		load ();
	return ConstTarget::beforeMove ();
}

int CalibrationTarget::endObservation (int in_next_id)
{
	needUpdate = 1;
	return ConstTarget::endObservation (in_next_id);
}

void CalibrationTarget::getPosition (struct ln_equ_posn *pos, double JD)
{
	if (obs_target_id <= 0)
	{
		ConstTarget::getPosition (pos, JD);
		return;
	}
	*pos = airmassPosition;
}

int CalibrationTarget::considerForObserving (double JD)
{
	// load (possibly new target) before considering us..
	load ();
	return ConstTarget::considerForObserving (JD);
}

float CalibrationTarget::getBonus (double JD)
{
	time_t now;
	time_t t_diff;

	int validTime = rts2core::Configuration::instance()->getCalibrationValidTime();
	int maxDelay = rts2core::Configuration::instance()->getCalibrationMaxDelay();
	float minBonus = rts2core::Configuration::instance()->getCalibrationMinBonus();
	float maxBonus = rts2core::Configuration::instance()->getCalibrationMaxBonus();
	if (obs_target_id <= 0)
		return -1;
	ln_get_timet_from_julian (JD, &now);
	t_diff = now - lastImage;
	// smaller then valid_time is not interesting..
	if (t_diff < validTime)
		return minBonus;
	// greater then MaxDelay is interesting
	else if (t_diff > maxDelay)
		return maxBonus;
	// otherwise linear increase
	return minBonus + ((maxBonus - minBonus) * t_diff / (maxDelay - validTime));
}

ModelTarget::ModelTarget (int in_tar_id, struct ln_lnlat_posn *in_obs):ConstTarget (in_tar_id, in_obs)
{
	modelStepType = 2;
	rts2core::Configuration::instance ()->getInteger ("observatory", "model_step_type", modelStepType);
}

void ModelTarget::load ()
{
	EXEC SQL BEGIN DECLARE SECTION;
	int d_tar_id = getTargetID ();
	float d_alt_start;
	float d_alt_stop;
	float d_alt_step;
	float d_az_start;
	float d_az_stop;
	float d_az_step;
	float d_noise;
	int d_step;
	EXEC SQL END DECLARE SECTION;

	EXEC SQL
	SELECT
		alt_start,
		alt_stop,
		alt_step,
		az_start,
		az_stop,
		az_step,
		noise,
		step
	INTO
		:d_alt_start,
		:d_alt_stop,
		:d_alt_step,
		:d_az_start,
		:d_az_stop,
		:d_az_step,
		:d_noise,
		:d_step
	FROM
		target_model
	WHERE
		tar_id = :d_tar_id;
	if (sqlca.sqlcode)
	{
	  	std::ostringstream err;
		err << "cannot load model target for ID " << d_tar_id;
		throw SqlError (err.str ().c_str ());
	}
	if (d_alt_start < d_alt_stop)
	{
		alt_start = d_alt_start;
		alt_stop = d_alt_stop;
	}
	else
	{
		alt_start = d_alt_stop;
		alt_stop = d_alt_start;
	}
	alt_step = d_alt_step;
	if (d_az_start < d_az_stop)
	{
		az_start = d_az_start;
		az_stop = d_az_stop;
	}
	else
	{
		az_start = d_az_stop;
		az_stop = d_az_start;
	}
	az_step = d_az_step;
	noise = d_noise;
	step = d_step;

	alt_size = (int) (fabs (alt_stop - alt_start) / alt_step);
	alt_size++;

	srandom (time (NULL));
	calPosition ();
	ConstTarget::load ();
}

ModelTarget::~ModelTarget (void)
{
}

int ModelTarget::writeStep ()
{
	EXEC SQL BEGIN DECLARE SECTION;
		int d_tar_id = getTargetID ();
		int d_step = step;
	EXEC SQL END DECLARE SECTION;
	EXEC SQL
		UPDATE
			target_model
		SET
			step = :d_step
		WHERE
			tar_id = :d_tar_id;
	if (sqlca.sqlcode)
	{
		logMsgDb ("ModelTarget::writeStep", MESSAGE_ERROR);
		EXEC SQL ROLLBACK;
		return -1;
	}
	EXEC SQL COMMIT;
	return 0;
}

int ModelTarget::getNextPosition ()
{
	switch (modelStepType)
	{
		case -2:
			// completly random model
			step = -2;
			break;
		case -1:
			// linear model
			step++;
			break;
		case 0:
			// random model
			step += (int) (((double) random () * ((fabs (360.0 / az_step) + 1) * fabs((alt_stop - alt_start) / alt_step) + 1)) / RAND_MAX);
			break;
		default:
			step += modelStepType * ((int) fabs ((alt_stop - alt_start) / alt_step) + 1) + 1;
	}
	return calPosition ();
}

int ModelTarget::calPosition ()
{
	double m_alt;
	switch (modelStepType)
	{
		case -2:
			hrz_poz.az = (az_stop - az_start) * random_num ();
			hrz_poz.alt = 0;
			m_alt = rts2core::Configuration::instance ()->getObjectChecker ()->getHorizonHeight (&hrz_poz, 0);
			if (m_alt < alt_start)
				m_alt = alt_start;
			if (m_alt < getMinObsAlt ())
				m_alt = getMinObsAlt ();
 			hrz_poz.alt = m_alt + (alt_stop - m_alt) * asin (random_num ()) / (M_PI / 2.0);
			if (!isAboveHorizon (&hrz_poz))
				hrz_poz.alt = rts2core::Configuration::instance ()->getObjectChecker ()->getHorizonHeight (&hrz_poz, 0) + 2;
			ra_noise = 0;
			dec_noise = 0;
			break;
		default:
			hrz_poz.az = az_start + az_step * (step / alt_size);
			hrz_poz.alt = alt_start + alt_step * (step % alt_size);
			ra_noise = 2 * noise * random_num ();
			ra_noise -= noise;
			dec_noise = 2 * noise * random_num ();
			dec_noise -= noise;
			if (!isAboveHorizon (&hrz_poz))
				hrz_poz.alt = rts2core::Configuration::instance ()->getObjectChecker ()->getHorizonHeight (&hrz_poz, 0) + 2 * noise;
	}
	// null ra + dec .. for recurent call do getPosition (JD..)
	equ_poz.ra = -1000;
	equ_poz.dec = -1000;
	return 0;
}

int ModelTarget::beforeMove ()
{
	endObservation (-1);		 // we will not observe same model target twice
	nullAcquired ();
	return getNextPosition ();
}

moveType ModelTarget::afterSlewProcessed ()
{
	EXEC SQL BEGIN DECLARE SECTION;
	int d_obs_id;
	int d_step = step;
	EXEC SQL END DECLARE SECTION;

	d_obs_id = getObsId ();

	EXEC SQL
		INSERT INTO
			model_observation
			(
			obs_id,
			step
			) VALUES (
			:d_obs_id,
			:d_step
			);
	if (sqlca.sqlcode)
	{
		logMsgDb ("ModelTarget::endObservation", MESSAGE_ERROR);
		EXEC SQL ROLLBACK;
	}
	else
	{
		EXEC SQL COMMIT;
	}
	//  return OBS_MOVE_UNMODELLED;
	return OBS_MOVE;
}

int ModelTarget::endObservation (int in_next_id)
{
	if (getObsId () > 0)
		writeStep ();
	return ConstTarget::endObservation (in_next_id);
}

void ModelTarget::getPosition (struct ln_equ_posn *pos, double JD)
{
	if (equ_poz.ra < -10)
	{
		ln_get_equ_from_hrz (&hrz_poz, observer, JD, &equ_poz);
		// put to right range
		equ_poz.ra += ra_noise;
		equ_poz.ra = ln_range_degrees (equ_poz.ra);
		equ_poz.dec += dec_noise;
		if (equ_poz.dec > 90)
			equ_poz.dec = 90;
		else if (equ_poz.dec < -90)
			equ_poz.dec = -90;
	}
	*pos = equ_poz;
}

// pick up some opportunity target; don't pick it too often
OportunityTarget::OportunityTarget (int in_tar_id, struct ln_lnlat_posn *in_obs):ConstTarget (in_tar_id, in_obs)
{
}

float OportunityTarget::getBonus (double JD)
{
	double retBonus = 0;
	struct ln_hrz_posn hrz;
	double lunarDist;
	double ha;
	double lastObs;
	time_t now;
	time_t start_t;
	getAltAz (&hrz, JD);
	lunarDist = getLunarDistance (JD);
	ha = getHourAngle ();
	// time target was last observed
	lastObs = getLastObsTime ();
	ln_get_timet_from_julian (JD, &now);
	start_t = now - 86400;
	// do not observe if already observed between 1h and 3h from now
	if (lastObs > 3600 && lastObs < (3 * 3600))
		retBonus -= log (lastObs / 3600) * 50;
	// do not observe if observed during last 24 h 
	else if (lastObs < 86400)
		retBonus -= log (3) * 50;
	// do not observe if too close to moon
	if (lunarDist < 60.0)
		retBonus -= log (61 - lunarDist) * 10;
	// bonus for south
	if (ha < 165)
		retBonus += log ((180 - ha) / 15.0);
	if (ha > 195)
		retBonus += log ((ha - 180) / 15.0);
	// bonus for altitute
	retBonus += hrz.alt * 2;
	// try to follow some period, based on how well we do in the past
	retBonus -= sin (getNumObs (&start_t, &now) * M_PI_4) * 5;
	// getBonus returns target priority + target bonus (from database)
	return ConstTarget::getBonus (JD) + retBonus;
}

// will pickup the Moon
LunarTarget::LunarTarget (int in_tar_id, struct ln_lnlat_posn *in_obs):Target (in_tar_id, in_obs)
{
}

void LunarTarget::getPosition (struct ln_equ_posn *pos, double JD)
{
	ln_get_lunar_equ_coords (JD, pos);
}

int LunarTarget::getRST (struct ln_rst_time *rst, double JD, double horizon)
{
	return ln_get_body_rst_horizon (JD, observer, ln_get_lunar_equ_coords, horizon, rst);
}

TargetSwiftFOV::TargetSwiftFOV (int in_tar_id, struct ln_lnlat_posn *in_obs):Target (in_tar_id, in_obs)
{
	swiftOnBonus = 0;
	target_id = TARGET_SWIFT_FOV;
	swiftId = -1;
	oldSwiftId = -1;
	swiftName = NULL;
	target_name = new char[200];

	swiftLastTar = -1;
	swiftLastTarName = NULL;
}

TargetSwiftFOV::~TargetSwiftFOV (void)
{
	delete[] swiftName;
	delete[] swiftLastTarName;
}

void TargetSwiftFOV::load ()
{
	struct ln_hrz_posn testHrz;
	struct ln_equ_posn testEqu;
	double JD = ln_get_julian_from_sys ();

	EXEC SQL BEGIN DECLARE SECTION;
	int d_swift_id = -1;
	double d_swift_ra;
	double d_swift_dec;
	double d_swift_roll;
	int d_swift_roll_null;
	int d_swift_time;
	float d_swift_obstime;
	VARCHAR d_swift_name[70];
	EXEC SQL END DECLARE SECTION;

	swiftId = -1;

	Target::load();

	delete[] swiftLastTarName;
	swiftLastTarName = NULL;

	EXEC SQL DECLARE find_swift_poiniting CURSOR FOR
	SELECT
		swift_id,
		swift_ra,
		swift_dec,
		swift_roll,
		EXTRACT (EPOCH FROM swift_time),
		swift_obstime,
		swift_name
	FROM
		swift
	WHERE
		swift_time is not NULL
		AND swift_obstime is not NULL
	ORDER BY
		swift_id desc;
	EXEC SQL OPEN find_swift_poiniting;
	while (1)
	{
		EXEC SQL FETCH next FROM find_swift_poiniting INTO
			:d_swift_id,
			:d_swift_ra,
			:d_swift_dec,
			:d_swift_roll :d_swift_roll_null,
			:d_swift_time,
			:d_swift_obstime,
			:d_swift_name;
		if (sqlca.sqlcode)
			break;
		// fill last values
		if (swiftLastTarName == NULL)
		{
			swiftLastTar = d_swift_id;
			swiftLastTarName = new char[d_swift_name.len + 1];
			strcpy (swiftLastTarName, d_swift_name.arr);
			swiftLastTarTimeStart = d_swift_time;
			swiftLastTarTimeEnd = d_swift_time + (int) d_swift_obstime;
			swiftLastTarPos.ra = d_swift_ra;
			swiftLastTarPos.dec = d_swift_dec;
		}
		// check for our altitude..
		testEqu.ra = d_swift_ra;
		testEqu.dec = d_swift_dec;
		ln_get_hrz_from_equ (&testEqu, observer, JD, &testHrz);
		if (testHrz.alt > rts2core::Configuration::instance ()->getSwiftMinHorizon ())
		{
			if (testHrz.alt < rts2core::Configuration::instance ()->getSwiftSoftHorizon ())
			{
				testHrz.alt = rts2core::Configuration::instance ()->getSwiftSoftHorizon ();
				// get equ coordinates we will observe..
				ln_get_equ_from_hrz (&testHrz, observer, JD, &testEqu);
			}
			swiftFovCenter.ra = testEqu.ra;
			swiftFovCenter.dec = testEqu.dec;
			if (oldSwiftId == -1)
				oldSwiftId = d_swift_id;
			swiftId = d_swift_id;
			swiftTimeStart = d_swift_time;
			swiftTimeEnd = d_swift_time + (int) d_swift_obstime;
			break;
		}
	}
	EXEC SQL CLOSE find_swift_poiniting;
	if (swiftId < 0)
	{
		setTargetName ("Cannot find any Swift FOV");
		swiftFovCenter.ra = NAN;
		swiftFovCenter.dec = NAN;
		swiftTimeStart = 0;
		swiftTimeEnd = 0;
		swiftRoll = NAN;
		return;
	}
	if (d_swift_roll_null)
		swiftRoll = NAN;
	else
		swiftRoll = d_swift_roll;
	delete[] swiftName;

	swiftName = new char[d_swift_name.len + 1];
	strncpy (swiftName, d_swift_name.arr, d_swift_name.len);
	swiftName[d_swift_name.len] = '\0';

	std::ostringstream name;
	name << "SwiftFOV #" << swiftId
		<< " ( " << Timestamp(swiftTimeStart)
		<< " - " << Timestamp(swiftTimeEnd) << " )";
	setTargetName (name.str().c_str());
}

void TargetSwiftFOV::getPosition (struct ln_equ_posn *pos, double JD)
{
	*pos = swiftFovCenter;
}

int TargetSwiftFOV::getRST (struct ln_rst_time *rst, double JD, double horizon)
{
	struct ln_equ_posn pos;

	getPosition (&pos, JD);
	return ln_get_object_next_rst_horizon (JD, observer, &pos, horizon, rst);
}

moveType TargetSwiftFOV::afterSlewProcessed ()
{
	EXEC SQL BEGIN DECLARE SECTION;
		int d_obs_id;
		int d_swift_id = swiftId;
	EXEC SQL END DECLARE SECTION;

	d_obs_id = getObsId ();
	EXEC SQL
		INSERT INTO
			swift_observation
			(
			swift_id,
			obs_id
			) VALUES (
			:d_swift_id,
			:d_obs_id
			);
	if (sqlca.sqlcode)
	{
		logMsgDb ("TargetSwiftFOV::startSlew SQL error", MESSAGE_ERROR);
		EXEC SQL ROLLBACK;
		return OBS_MOVE_FAILED;
	}
	EXEC SQL COMMIT;
	return OBS_MOVE;
}

int TargetSwiftFOV::considerForObserving (double JD)
{
	// find pointing
	int ret;

	load ();

	if (swiftId < 0)
	{
		// no pointing..expect that after 3 minutes, it will be better,
		// as we can get new pointing from GCN
		setNextObservable (JD + 1/1440.0/20.0);
		return -1;
	}

	ret = isAboveHorizon (JD);

	if (!ret)
	{
		time_t nextObs;
		time (&nextObs);
								 // we found pointing that already
		if (nextObs > swiftTimeEnd)
			// happens..hope that after 2
			// minutes, we will get better
			// results
		{
			nextObs += 2 * 60;
		}
		else
		{
			nextObs = swiftTimeEnd;
		}
		setNextObservable (&nextObs);
		return -1;
	}

	return selectedAsGood ();
}

int TargetSwiftFOV::beforeMove ()
{
	// are we still the best swiftId on planet?
	load ();
	if (oldSwiftId != swiftId)
		endObservation (-1);	 // startSlew will be called after move suceeded and will write new observation..
	return 0;
}

float TargetSwiftFOV::getBonus (double JD)
{
	EXEC SQL BEGIN DECLARE SECTION;
		int d_tar_id = target_id;
		double d_bonus;
	EXEC SQL END DECLARE SECTION;

	EXEC SQL
		SELECT
			tar_priority
		INTO
			:d_bonus
		FROM
			targets
		WHERE
			tar_id = :d_tar_id;

	swiftOnBonus = d_bonus;

	time_t now;
	ln_get_timet_from_julian (JD, &now);
	if (now > swiftTimeStart - 120 && now < swiftTimeEnd + 120)
		return swiftOnBonus;
	if (now > swiftTimeStart - 300 && now < swiftTimeEnd + 300)
		return swiftOnBonus / 2.0;
	return getTargetPriority ();
}

int TargetSwiftFOV::isContinues ()
{
	double ret;
	load ();
	if (oldSwiftId != swiftId)
		return 0;
	ret = Target::getBonus ();
	// FOV become uinteresting..
	if (ret == 1)
		return 0;
	return 1;
}

void TargetSwiftFOV::printExtra (Rts2InfoValStream &_os, double JD)
{
	Target::printExtra (_os, JD);
	double now = timetFromJD (JD);
	_os
		<< InfoVal<const char *> ("NAME", swiftName)
		<< InfoVal<int> ("SwiftFOW ID", swiftId)
		<< InfoVal<Timestamp> ("FROM", Timestamp (swiftTimeStart))
		<< InfoVal<TimeDiff> ("FROM DIFF", TimeDiff (now, swiftTimeStart))
		<< InfoVal<Timestamp> ("TO", Timestamp (swiftTimeEnd))
		<< InfoVal<TimeDiff> ("TO DIFF", TimeDiff (now, swiftTimeEnd))
		<< InfoVal<double> ("ROLL", swiftRoll)
		<< std::endl;

	if (swiftLastTarName != NULL)
	{
		struct ln_hrz_posn lastHrz;
		ln_get_hrz_from_equ (&swiftLastTarPos, observer, JD, &lastHrz);
		_os
			<< InfoVal<const char *> ("LAST NAME", swiftLastTarName)
			<< InfoVal<int> ("LAST ID", swiftLastTar)
			<< InfoVal<Timestamp> ("LAST START", Timestamp (swiftLastTarTimeStart))
			<< InfoVal<TimeDiff> ("LAST START DIFF", TimeDiff (now, swiftLastTarTimeStart))
			<< InfoVal<Timestamp> ("LAST END", Timestamp (swiftLastTarTimeEnd))
			<< InfoVal<TimeDiff> ("LAST END DIFF", TimeDiff (now, swiftLastTarTimeEnd))
			<< InfoVal<LibnovaRaJ2000> ("LAST RA", LibnovaRaJ2000 (swiftLastTarPos.ra))
			<< InfoVal<LibnovaDecJ2000> ("LAST DEC", LibnovaDecJ2000 (swiftLastTarPos.dec))
			<< InfoVal<LibnovaDeg90> ("LAST ALT", LibnovaDeg90 (lastHrz.alt))
			<< InfoVal<LibnovaDeg360> ("LAST AZ", LibnovaDeg360 (lastHrz.az))
			<< std::endl;
	}
}

TargetIntegralFOV::TargetIntegralFOV (int in_tar_id, struct ln_lnlat_posn *in_obs):Target (in_tar_id, in_obs)
{
	target_id = TARGET_INTEGRAL_FOV;
	integralId = -1;
	oldIntegralId = -1;
	integralOnBonus = 0;
}

TargetIntegralFOV::~TargetIntegralFOV (void)
{
}

void TargetIntegralFOV::load ()
{
	struct ln_hrz_posn testHrz;
	struct ln_equ_posn testEqu;
	double JD = ln_get_julian_from_sys ();

	EXEC SQL BEGIN DECLARE SECTION;
	int d_integral_id = -1;
	double d_integral_ra;
	double d_integral_dec;
	long d_integral_time;
	EXEC SQL END DECLARE SECTION;

	integralId = -1;

	Target::load();

	EXEC SQL DECLARE find_integral_poiniting CURSOR FOR
	SELECT
		integral_id,
		integral_ra,
		integral_dec,
		EXTRACT (EPOCH FROM integral_time)
	FROM
		integral
	WHERE
		integral_time is not NULL
	ORDER BY
		integral_id desc;
	EXEC SQL OPEN find_integral_poiniting;
	while (1)
	{
		EXEC SQL FETCH next FROM find_integral_poiniting INTO
			:d_integral_id,
			:d_integral_ra,
			:d_integral_dec,
			:d_integral_time;
		if (sqlca.sqlcode)
			break;
		// check for our altitude..
		testEqu.ra = d_integral_ra;
		testEqu.dec = d_integral_dec;
		ln_get_hrz_from_equ (&testEqu, observer, JD, &testHrz);
		if (testHrz.alt > 0)
		{
			if (testHrz.alt < 30)
			{
				testHrz.alt = 30;
				// get equ coordinates we will observe..
				ln_get_equ_from_hrz (&testHrz, observer, JD, &testEqu);
			}
			integralFovCenter.ra = testEqu.ra;
			integralFovCenter.dec = testEqu.dec;
			if (oldIntegralId == -1)
				oldIntegralId = d_integral_id;
			integralId = d_integral_id;
			integralTimeStart = d_integral_time;
			break;
		}
	}
	EXEC SQL CLOSE find_integral_poiniting;
	if (integralId < 0)
	{
		setTargetName ("Cannot find any INTEGRAL FOV");
		integralFovCenter.ra = NAN;
		integralFovCenter.dec = NAN;
		integralTimeStart = 0;
		return;
	}

	std::ostringstream name;
	name << "IntegralFOV #" << integralId
		<< " ( " << Timestamp(integralTimeStart) << " )";
	setTargetName (name.str().c_str());
}

void TargetIntegralFOV::getPosition (struct ln_equ_posn *pos, double JD)
{
	*pos = integralFovCenter;
}

int TargetIntegralFOV::getRST (struct ln_rst_time *rst, double JD, double horizon)
{
	struct ln_equ_posn pos;

	getPosition (&pos, JD);
	return ln_get_object_next_rst_horizon (JD, observer, &pos, horizon, rst);
}

moveType TargetIntegralFOV::afterSlewProcessed ()
{
	EXEC SQL BEGIN DECLARE SECTION;
		int d_obs_id;
		int d_integral_id = integralId;
	EXEC SQL END DECLARE SECTION;

	d_obs_id = getObsId ();
	EXEC SQL
		INSERT INTO
			integral_observation
			(
			integral_id,
			obs_id
			) VALUES (
			:d_integral_id,
			:d_obs_id
			);
	if (sqlca.sqlcode)
	{
		logMsgDb ("TargetIntegralFOV::startSlew SQL error", MESSAGE_ERROR);
		EXEC SQL ROLLBACK;
		return OBS_MOVE_FAILED;
	}
	EXEC SQL COMMIT;
	return OBS_MOVE;
}

int TargetIntegralFOV::considerForObserving (double JD)
{
	// find pointing
	int ret;

	load ();

	if (integralId < 0)
	{
		// no pointing..expect that after 3 minutes, it will be better,
		// as we can get new pointing from GCN
		setNextObservable (JD + 1/1440.0/20.0);
		return -1;
	}

	ret = isAboveHorizon (JD);

	if (!ret)
	{
		time_t nextObs;
		time (&nextObs);
		nextObs += 120;
		setNextObservable (&nextObs);
		return -1;
	}

	return selectedAsGood ();
}

int TargetIntegralFOV::beforeMove ()
{
	// are we still the best swiftId on planet?
	load ();
	if (oldIntegralId != integralId)
		endObservation (-1);	 // startSlew will be called after move suceeded and will write new observation..
	return 0;
}

float TargetIntegralFOV::getBonus (double JD)
{
	EXEC SQL BEGIN DECLARE SECTION;
		int d_tar_id = target_id;
		double d_bonus;
	EXEC SQL END DECLARE SECTION;

	EXEC SQL
		SELECT
			tar_priority
		INTO
			:d_bonus
		FROM
			targets
		WHERE
			tar_id = :d_tar_id;

	integralOnBonus = d_bonus;

	time_t now;
	ln_get_timet_from_julian (JD, &now);
	if (now > integralTimeStart - 120)
		return integralOnBonus;
	if (now > integralTimeStart - 300)
		return integralOnBonus / 2.0;
	return 1;
}

int TargetIntegralFOV::isContinues ()
{
	double ret;
	load ();
	if (oldIntegralId != integralId)
		return 0;
	ret = Target::getBonus ();
	// FOV become uinteresting..
	if (ret == 1)
		return 0;
	return 1;
}

void TargetIntegralFOV::printExtra (Rts2InfoValStream &_os, double JD)
{
	Target::printExtra (_os, JD);
	_os
		<< InfoVal<Timestamp> ("FROM", Timestamp (integralTimeStart))
		<< std::endl;
}

TargetGps::TargetGps (int in_tar_id, struct ln_lnlat_posn *in_obs): ConstTarget (in_tar_id, in_obs)
{
}

float TargetGps::getBonus (double JD)
{
	// get our altitude..
	struct ln_hrz_posn hrz;
	struct ln_equ_posn curr;
	int numobs;
	int numobs2;
	time_t now;
	time_t start_t;
	time_t start_t2;
	double gal_ctr;
	getAltAz (&hrz, JD);
	getPosition (&curr, JD);
	// get number of observations in last 24 hours..
	ln_get_timet_from_julian (JD, &now);
	start_t = now - 86400;
	start_t2 = now - 86400 * 2;
	numobs = getNumObs (&start_t, &now);
	numobs2 = getNumObs (&start_t2, &start_t);
	gal_ctr = getGalCenterDist (JD);
	if ((90 - observer->lat + curr.dec) == 0)
		return ConstTarget::getBonus (JD) - 200;
	return ConstTarget::getBonus (JD) + 20 * (hrz.alt / (90 - observer->lat + curr.dec)) + gal_ctr / 9 - numobs * 10 - numobs2 * 5;
}

TargetSkySurvey::TargetSkySurvey (int in_tar_id, struct ln_lnlat_posn *in_obs): ConstTarget (in_tar_id, in_obs)
{
}

float TargetSkySurvey::getBonus (double JD)
{
	time_t now;
	time_t start_t;
	int numobs;
	ln_get_timet_from_julian (JD, &now);
	now -= 300;
	start_t = now - 86400;
	numobs = getNumObs (&start_t, &now);
	return ConstTarget::getBonus (JD) - numobs * 10;
}

TargetTerestial::TargetTerestial (int in_tar_id, struct ln_lnlat_posn *in_obs):ConstTarget (in_tar_id, in_obs)
{
}

int TargetTerestial::considerForObserving (double JD)
{
	// we can obsere it any time..
	return selectedAsGood ();
}

float TargetTerestial::getBonus (double JD)
{
	time_t now;
	time_t start_t;
	struct tm *now_t;
	int numobs;
	int minofday;
	ln_get_timet_from_julian (JD, &now);
	now_t = gmtime (&now);

	minofday = now_t->tm_hour * 60 + now_t->tm_min;
	// HAM times (all UTC) (min of day)
	// 1:30 - 3:30          90 - 210
	// 4:00 - 6:00         240 - 360
	// 6:30 - 8:30         390 - 510
	// not correct times..
	if (minofday < 100
		|| (minofday > 200 && minofday < 250)
		|| (minofday > 350 && minofday < 400)
		|| minofday > 500)
		return 1;

	// we can observe..

	start_t = now - 3600;
	numobs = getNumObs (&start_t, &now);

	if (numobs == 0)
		return 600;
	if (numobs == 1)
		return 1;
	return 1;
}

moveType TargetTerestial::afterSlewProcessed ()
{
	return OBS_MOVE_FIXED;
}

TargetPlan::TargetPlan (int in_tar_id, struct ln_lnlat_posn *in_obs) : Target (in_tar_id, in_obs)
{
	selectedPlan = NULL;
	nextPlan = NULL;
	rts2core::Configuration::instance ()->getFloat ("selector", "last_search", hourLastSearch, 16.0);
	rts2core::Configuration::instance ()->getFloat ("selector", "consider_plan", hourConsiderPlans, 1.0);
	nextTargetRefresh = 0;
}

TargetPlan::~TargetPlan (void)
{
	delete selectedPlan;
	delete nextPlan;
}


// refresh next time..
void TargetPlan::refreshNext ()
{
	EXEC SQL BEGIN DECLARE SECTION;
		int db_next_plan_id;
		long db_next_plan_start;
		long db_next;
	EXEC SQL END DECLARE SECTION;

	time_t now;
	int ret;

	time (&now);
	if (now < nextTargetRefresh)
		return;

	// next refresh after 60 seconds
	nextTargetRefresh = now + 60;

	if (nextPlan)
	{
		// we need to select new plan..don't bother with db select
		if (nextPlan->getPlanStart () < now)
			return;
	}
	// consider targets 1/2 hours old..
	db_next = now - 1800;

	EXEC SQL
		SELECT
			plan_id,
			EXTRACT (EPOCH FROM plan_start)
		INTO
			:db_next_plan_id,
			:db_next_plan_start
		FROM
			plan
		WHERE
			plan_start = (SELECT min(plan_start) FROM plan WHERE plan_start >= to_timestamp (:db_next));
	if (sqlca.sqlcode)
	{
		logMsgDb ("TargetPlan::refreshNext cannot load next target",
			(sqlca.sqlcode == ECPG_NOT_FOUND) ? MESSAGE_DEBUG : MESSAGE_ERROR);
		EXEC SQL ROLLBACK;
		// we can be last plan entry - so change us, so selectedTarget will be changed
		return;
	}
	EXEC SQL ROLLBACK;
	// we loaded same plan as already prepared
	if (nextPlan && nextPlan->getPlanId() == db_next_plan_id && nextPlan->getPlanStart() == db_next_plan_start)
		return;
	delete nextPlan;
	nextPlan = new Plan (db_next_plan_id);
	ret = nextPlan->load ();
	if (ret)
	{
		delete nextPlan;
		nextPlan = NULL;
		return;
	}
	// we will need change
	return;
}

void TargetPlan::load ()
{
	load (ln_get_julian_from_sys ());
}

void TargetPlan::load (double JD)
{
	EXEC SQL BEGIN DECLARE SECTION;
	int db_cur_plan_id;
	double db_cur_plan_start;

	long last;
	EXEC SQL END DECLARE SECTION;

	int db_plan_id = -1;
	int db_next_plan_id = -1;

	time_t now;
	time_t considerPlans;

	ln_get_timet_from_julian (JD, &now);

	// always load plan target!
	Target::loadTarget (getTargetID ());

	// get plan entries from last 12 hours..
	last = (time_t) (now - (hourLastSearch * 3600));

	considerPlans = (time_t) (now - (hourConsiderPlans * 3600.0));

	// check for next plan entry after 1 minute..
	nextTargetRefresh = now + 60;

	EXEC SQL DECLARE cur_plan CURSOR FOR
	SELECT
		plan_id,
		EXTRACT (EPOCH FROM plan_start)
	FROM
		plan
	WHERE
		EXTRACT (EPOCH FROM plan_start) >= :last
	AND tar_id <> 7
	ORDER BY
		plan_start ASC;

	EXEC SQL OPEN cur_plan;
	while (1)
	{
		EXEC SQL FETCH next FROM cur_plan INTO
			:db_cur_plan_id,
			:db_cur_plan_start;
		if (sqlca.sqlcode)
			break;
		if (db_cur_plan_start > now)
		{
			if (db_plan_id == -1)
			{
				// that's the last plan ID
				db_plan_id = db_next_plan_id;
			}
			db_next_plan_id = db_cur_plan_id;
			break;
		}
		if (db_cur_plan_start > considerPlans)
		{
			// we found current plan - one that wasn't observed and is close enought to current time
			if (db_plan_id == -1)
			{
				db_plan_id = db_cur_plan_id;
			}
			else
			{
				db_next_plan_id = db_cur_plan_id;
				break;
			}
		}
		// keep for futher reference
		db_next_plan_id = db_cur_plan_id;
	}
	if (sqlca.sqlcode)
	{
		if (sqlca.sqlcode != ECPG_NOT_FOUND || db_next_plan_id == -1)
		{
			if (sqlca.sqlcode != ECPG_NOT_FOUND)
				logMsgDb ("TargetPlan::load cannot find any plan", MESSAGE_ERROR);
			EXEC SQL CLOSE cur_plan;
			return;
		}
		// we don't find next, but we have current
		if (db_plan_id == -1)
		{
			db_plan_id = db_next_plan_id;
		}
		db_next_plan_id = -1;
	}
	EXEC SQL CLOSE cur_plan;

	if (db_plan_id != -1)
	{
		delete selectedPlan;
		selectedPlan = new Plan (db_plan_id);
		selectedPlan->load ();
		if (selectedPlan->getTarget ()->getTargetType () == TYPE_PLAN)
		{
			delete selectedPlan;
			selectedPlan = NULL;
			throw SqlError ("target scheduled from plan is also planning target, that is not allowed");
		}
		setTargetName (selectedPlan->getTarget()->getTargetName ());
	}
	else
	{
		setTargetName ("No scheduled target");
	}
	if (db_next_plan_id != -1)
	{
		delete nextPlan;
		nextPlan = new Plan (db_next_plan_id);
		nextPlan->load ();
	}
}

bool TargetPlan::getScript (const char *camera_name, std::string &script)
{
	if (selectedPlan)
		return selectedPlan->getTarget()->getScript (camera_name, script);
	return Target::getScript (camera_name, script);
}

void TargetPlan::getPosition (struct ln_equ_posn *pos, double JD)
{
	if (selectedPlan)
	{
		selectedPlan->getTarget()->getPosition (pos, JD);
	}
	else
	{
		// pretend to observe in zenith..
		pos->ra = ln_get_mean_sidereal_time (JD) * 15.0 + observer->lng;
		pos->dec = observer->lat;
	}
}

int TargetPlan::getRST (struct ln_rst_time *rst, double JD, double horizon)
{
	if (selectedPlan)
		return selectedPlan->getTarget()->getRST (rst, JD, horizon);
	// otherwise we have circumpolar target
	return 1;
}

int TargetPlan::getObsTargetID ()
{
	if (selectedPlan)
		return selectedPlan->getTarget()->getObsTargetID ();
	return Target::getObsTargetID ();
}

int TargetPlan::considerForObserving (double JD)
{
	load (JD);
	return Target::considerForObserving (JD);
}

float TargetPlan::getBonus (double JD)
{
	// we have something to observe
	if (selectedPlan)
		return Target::getBonus (JD);
	return 0;
}

int TargetPlan::isContinues ()
{
	time_t now;
	refreshNext ();
	time (&now);
	if ((nextPlan && nextPlan->getPlanStart () < now)
		|| !(nextPlan))
		return 2;
	if (selectedPlan)
		return selectedPlan->getTarget()->isContinues ();
	return 0;
}

int TargetPlan::beforeMove ()
{
	if (selectedPlan)
		selectedPlan->getTarget ()->beforeMove ();
	return Target::beforeMove ();
}

moveType TargetPlan::startSlew (struct ln_equ_posn *pos, bool update_position, int plan_id)
{
	moveType ret;
	if (selectedPlan)
	{
		ret = selectedPlan->startSlew (pos, plan_id);
		setObsId (selectedPlan->getTarget()->getObsId ());
		return ret;
	}
	return Target::startSlew (pos, update_position, plan_id);
}

void TargetPlan::printExtra (Rts2InfoValStream & _os, double JD)
{
	Target::printExtra (_os, JD);
	if (selectedPlan)
	{
		_os
			<< "SELECTED PLAN" << std::endl
			<< selectedPlan << std::endl
			<< *(selectedPlan->getTarget ()) << std::endl
			<< "*************************************************" << std::endl << std::endl;
	}
	else
	{
		_os << "NO PLAN SELECTED" << std::endl;
	}
	if (nextPlan && nextPlan->getTarget()->getTargetType() != TYPE_PLAN)
	{
		_os << "NEXT PLAN" << std::endl
			<< nextPlan << std::endl
			<< *(nextPlan->getTarget ()) << std::endl
			<< "*************************************************" << std::endl << std::endl;
	}
	else
	{
		_os << "NO NEXT PLAN SELECTED" << std::endl << std::endl;
	}
}
