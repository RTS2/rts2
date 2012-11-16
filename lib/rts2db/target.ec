/* 
 * Target class.
 * Copyright (C) 2003-2010 Petr Kubanek <petr@kubanek.net>
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

#include "rts2db/target_auger.h"
#include "rts2db/targetell.h"
#include "rts2targetplanet.h"
#include "rts2db/targetgrb.h"

#include "rts2db/constraints.h"
#include "rts2db/target.h"
#include "rts2db/observation.h"
#include "rts2db/observationset.h"
#include "rts2db/targetset.h"
#include "rts2db/sqlerror.h"
#include "rts2db/simbadtarget.h"

#include "infoval.h"
#include "app.h"
#include "configuration.h"
#include "libnova_cpp.h"
#include "timestamp.h"

#include <sstream>
#include <iomanip>

#include <sys/types.h>
#include <sys/stat.h>

EXEC SQL include sqlca;

using namespace rts2db;

void Target::logMsgDb (const char *message, messageType_t msgType)
{
	logStream (msgType) << "SQL error: " << sqlca.sqlcode << " " <<
		sqlca.sqlerrm.sqlerrmc << " (at " << message << ")" << sendLog;
}

void Target::getTargetSubject (std::string &subj)
{
	std::ostringstream _os;
	_os
		<< getTargetName ()
		<< " #" << getObsTargetID () << " (" << getTargetID () << ")"
		<< " " << getTargetType ();
	subj = _os.str();
}

void Target::addWatch (const char *filename)
{
	if (watchConn == NULL)
		return;
	int ret = watchConn->addWatch (filename);
	if (ret < 0)
		logStream (MESSAGE_ERROR) << "cannot add " << filename << " to watched files: " << strerror (errno) << sendLog;
	watchIDs.push_back (ret);
}

void Target::writeAirmass (std::ostream & _os, double jd)
{
	double am = getAirmass (jd);
	if (am > 9)
		_os << "nan ";
	else
		_os << std::setw (3) << getAirmass (jd);
}

void Target::printAltTable (std::ostream & _os, double jd_start, double h_start, double h_end, double h_step, bool header)
{
	double i;
	struct ln_hrz_posn hrz;
	struct ln_equ_posn pos;
	double jd;

	int old_precison = 0;
	std::ios_base::fmtflags old_settings = std::ios_base::skipws;

	jd_start += h_start / 24.0;

	if (header)
	{
		old_precison = _os.precision (0);
		old_settings = _os.flags ();
		_os.setf (std::ios_base::fixed, std::ios_base::floatfield);
		
		_os << " H ALT  AZ  HA  AIR  LD  SD SAL SAZ LAL LAZ" << std::endl;
	}

	jd = jd_start;
	for (i = h_start; i <= h_end; i+=h_step, jd += h_step/24.0)
	{
		getAltAz (&hrz, jd);
		char old_fill = _os.fill ('0');
		_os << std::setw (2) << i << " ";
		_os.fill (old_fill);
		_os << std::setw (3) << hrz.alt << " " << std::setw (3) << hrz.az << " " << std::setw (3) << (getHourAngle (jd)  / 15.0) << " ";

		_os.precision (2);
		if (header)
		{
			writeAirmass (_os, jd);
		}
		else
		{
			_os << getBonus (jd);
		}
		_os.precision (0);

		ln_get_solar_equ_coords (jd, &pos);
		ln_get_hrz_from_equ (&pos, getObserver (), jd, &hrz);

		_os << " " << std::setw(3) << getLunarDistance (jd)
			<< " " << std::setw(3) << getSolarDistance (jd)
			<< " " << std::setw(3) << hrz.alt
			<< " " << std::setw(3) << hrz.az;

		ln_get_lunar_equ_coords (jd, &pos);
		ln_get_hrz_from_equ (&pos, getObserver (), jd, &hrz);
		_os << " " << std::setw (3) << hrz.alt
			<< " " << std::setw (3) << hrz.az;

		_os << std::endl;
	}

	if (header)
	{
		_os.setf (old_settings);
		_os.precision (old_precison);
	}
}

void Target::printAltTable (std::ostream & _os, double JD)
{
	double jd_start = ((int) JD) - 0.5;
	_os
		<< std::endl
		<< "** HOUR, ALTITUDE, AZIMUTH, MOON DISTANCE, SUN POSITION table from "
		<< TimeJD (jd_start)
		<< " **"
		<< std::endl
		<< std::endl;

	printAltTable (_os, jd_start, -1, 24);
}

void Target::printAltTableSingleCol (std::ostream & _os, double jd_start, double i, double step)
{
	printAltTable (_os, jd_start, i, i+step, step * 2.0, false);
}

Target::Target (int in_tar_id, struct ln_lnlat_posn *in_obs):Rts2Target ()
{
	rts2core::Configuration *config = rts2core::Configuration::instance ();

	watchConn = NULL;

	observer = in_obs;

	config->getDouble ("observatory", "min_alt", minObsAlt, 0);

	observation = NULL;

	target_id = in_tar_id;
	obs_target_id = -1;
	target_type = TYPE_UNKNOW;
	target_name = NULL;
	target_comment = NULL;

	startCalledNum = 0;

	airmassScale = 750.0;

	constraintsLoaded = CONSTRAINTS_NONE;
	
	constraintFile = NULL;

	groupConstraintFile = NULL;

	observationStart = -1;

	satisfiedFrom = NAN;
	satisfiedTo = NAN;
	satisfiedProbedUntil = NAN;
}

Target::Target ()
{
	rts2core::Configuration *config;
	config = rts2core::Configuration::instance ();

	watchConn = NULL;

	observer = config->getObserver ();

	config->getDouble ("observatory", "min_alt", minObsAlt, 0);

	observation = NULL;

	target_id = -1;
	obs_target_id = -1;
	target_type = TYPE_UNKNOW;
	target_name = NULL;
	target_comment = NULL;

	startCalledNum = 0;

	airmassScale = 750.0;

	constraintsLoaded = CONSTRAINTS_NONE;

	constraintFile = NULL;

	groupConstraintFile = NULL;

	observationStart = -1;

	satisfiedFrom = NAN;
	satisfiedTo = NAN;
	satisfiedProbedUntil = NAN;

	tar_priority = 0;
	tar_bonus = NAN;
	tar_bonus_time = 0;
	tar_next_observable = 0;

	config->getFloat ("newtarget", "priority", tar_priority, 0);
	setTargetEnabled (config->getBoolean ("newtarget", "enabled", false), false);
}

Target::~Target (void)
{
	endObservation (-1);
	delete[] target_name;
	delete[] target_comment;
	delete observation;
	delete[] constraintFile;
}

void Target::load ()
{
	loadTarget (getObsTargetID ());
}

void Target::loadTarget (int in_tar_id)
{
	EXEC SQL BEGIN DECLARE SECTION;
	// cannot use TARGET_NAME_LEN, as some versions of ecpg complains about it
	VARCHAR d_tar_name[150];
	VARCHAR d_tar_info[2000];
	int d_tar_info_ind;
	float d_tar_priority;
	int d_tar_priority_ind;
	float d_tar_bonus;
	int d_tar_bonus_ind;
	long d_tar_bonus_time;
	int d_tar_bonus_time_ind;
	long d_tar_next_observable;
	int d_tar_next_observable_ind;
	bool d_tar_enabled;
	int db_tar_id = in_tar_id;
	EXEC SQL END DECLARE SECTION;

	EXEC SQL
	SELECT
		tar_name,
		tar_info,
		tar_priority,
		tar_bonus,
		EXTRACT (EPOCH FROM tar_bonus_time),
		EXTRACT (EPOCH FROM tar_next_observable),
		tar_enabled
	INTO
		:d_tar_name,
		:d_tar_info :d_tar_info_ind,
		:d_tar_priority :d_tar_priority_ind,
		:d_tar_bonus :d_tar_bonus_ind,
		:d_tar_bonus_time :d_tar_bonus_time_ind,
		:d_tar_next_observable :d_tar_next_observable_ind,
		:d_tar_enabled
	FROM
		targets
	WHERE
		tar_id = :db_tar_id;
	if (sqlca.sqlcode)
	{
	  	std::ostringstream err;
		err << "cannot load target with ID " << in_tar_id;
	  	throw SqlError (err.str ().c_str ());
	}

	delete[] target_name;

	target_name = new char[d_tar_name.len + 1];
	strncpy (target_name, d_tar_name.arr, d_tar_name.len);
	target_name[d_tar_name.len] = '\0';

	if (d_tar_info_ind >= 0)
	{
		char t_tar_info[d_tar_info.len + 1];
		strncpy (t_tar_info, d_tar_info.arr, d_tar_info.len);
		t_tar_info[d_tar_info.len] = '\0';
		tar_info = std::string (t_tar_info);
	}
	else
	{
		tar_info = std::string ("");
	}

	if (d_tar_priority_ind >= 0)
		tar_priority = d_tar_priority;
	else
		tar_priority = 0;

	if (d_tar_bonus_ind >= 0)
		tar_bonus = d_tar_bonus;
	else
		tar_bonus = -1;

	if (d_tar_bonus_time_ind >= 0)
		tar_bonus_time = d_tar_bonus_time;
	else
		tar_bonus_time = 0;

	if (d_tar_next_observable_ind >= 0)
		tar_next_observable = d_tar_next_observable;
	else
		tar_next_observable = 0;

	setTargetEnabled (d_tar_enabled, false);
}

int Target::save (bool overwrite)
{
	EXEC SQL BEGIN DECLARE SECTION;
	int db_new_id = getTargetID ();
	EXEC SQL END DECLARE SECTION;

	// generate new id, if we don't have any
	if (db_new_id <= 0)
	{
		EXEC SQL
			SELECT
			nextval ('tar_id')
			INTO
				:db_new_id;
		if (sqlca.sqlcode)
		{
			logMsgDb ("Target::save cannot get new tar_id", MESSAGE_ERROR);
			return -1;
		}
	}

	return saveWithID (overwrite, db_new_id);
}

void Target::deleteTarget ()
{
	EXEC SQL BEGIN DECLARE SECTION;
	int d_tar_id = getTargetID ();
	int d_ret;
	EXEC SQL END DECLARE SECTION;

	EXEC SQL SELECT delete_target(:d_tar_id) INTO :d_ret;
	if (sqlca.sqlcode)
	{
		throw SqlError ();
	}
	EXEC SQL COMMIT;
}

int Target::saveWithID (bool overwrite, int tar_id)
{
	// first, try an update..
	EXEC SQL BEGIN DECLARE SECTION;
		int db_tar_id = tar_id;
		char db_type_id = getTargetType ();
		VARCHAR db_tar_name[150];
		int db_tar_name_ind = 0;
		VARCHAR db_tar_comment[2000];
		int db_tar_comment_ind = 0;
		float db_tar_priority = getTargetPriority ();
		float db_tar_bonus = getTargetBonus ();
		int db_tar_bonus_ind;
		long db_tar_bonus_time = *(getTargetBonusTime ());
		int db_tar_bonus_time_ind;
		bool db_tar_enabled = getTargetEnabled ();
		VARCHAR db_tar_info[2000];
		int db_tar_info_ind;
	EXEC SQL END DECLARE SECTION;
	// fill in name and comment..
	if (getTargetName ())
	{
		db_tar_name.len = strlen (getTargetName ());
		strcpy (db_tar_name.arr, getTargetName ());
	}
	else
	{
		db_tar_name.len = 0;
		db_tar_name_ind = -1;
	}

	if (getTargetComment ())
	{
		db_tar_comment.len = strlen (getTargetComment ());
		strcpy (db_tar_comment.arr, getTargetComment ());
	}
	else
	{
		db_tar_comment.len = 0;
		db_tar_comment_ind = -1;
	}

	if (isnan (db_tar_bonus))
	{
		db_tar_bonus = 0;
		db_tar_bonus_ind =-1;
		db_tar_bonus_time = 0;
		db_tar_bonus_time_ind = -1;
	}
	else
	{
		db_tar_bonus_ind = 0;
		if (db_tar_bonus_time > 0)
		{
			db_tar_bonus_time_ind = 0;
		}
		else
		{
			db_tar_bonus_time_ind = -1;
		}
	}
	if (getTargetInfo ())
	{
		db_tar_info.len = strlen (getTargetInfo ());
		if (db_tar_info.len > 2000)
			db_tar_info.len = 2000;
		strncpy (db_tar_info.arr, getTargetInfo (), db_tar_info.len);
		db_tar_info_ind = 0;
	}
	else
	{
		db_tar_info.len = 0;
		db_tar_info_ind = -1;
	}

	// try inserting it..
	EXEC SQL
		INSERT INTO targets
			(
			tar_id,
			type_id,
			tar_name,
			tar_comment,
			tar_priority,
			tar_bonus,
			tar_bonus_time,
			tar_next_observable,
			tar_enabled,
			tar_info
			)
		VALUES
			(
			:db_tar_id,
			:db_type_id,
			:db_tar_name :db_tar_name_ind,
			:db_tar_comment :db_tar_comment_ind,
			:db_tar_priority,
			:db_tar_bonus :db_tar_bonus_ind,
			to_timestamp (:db_tar_bonus_time :db_tar_bonus_time_ind),
		null,
			:db_tar_enabled,
			:db_tar_info :db_tar_info_ind
			);
	// insert failed - try update
	if (sqlca.sqlcode)
	{
		EXEC SQL ROLLBACK;

		if (!overwrite)
		{
			return -1;
		}

		EXEC SQL
			UPDATE
				targets
			SET
				type_id = :db_type_id,
				tar_name = :db_tar_name,
				tar_comment = :db_tar_comment,
				tar_priority = :db_tar_priority,
				tar_bonus = :db_tar_bonus :db_tar_bonus_ind,
				tar_bonus_time = to_timestamp(:db_tar_bonus_time :db_tar_bonus_time_ind),
				tar_enabled = :db_tar_enabled,
				tar_info = :db_tar_info :db_tar_info_ind
			WHERE
				tar_id = :db_tar_id;

		if (sqlca.sqlcode)
		{
			logMsgDb ("Target::saveWithID", MESSAGE_ERROR);
			EXEC SQL ROLLBACK;
			return -1;
		}
	}
	target_id = db_tar_id;

	EXEC SQL COMMIT;
	return 0;
}

int Target::compareWithTarget (Target *in_target, double in_sep_limit)
{
	struct ln_equ_posn other_position;
	in_target->getPosition (&other_position);

	return (getDistance (&other_position) < in_sep_limit);
}

moveType Target::startSlew (struct ln_equ_posn *position, bool update_position)
{
	EXEC SQL BEGIN DECLARE SECTION;
	int d_tar_id = getObsTargetID ();
	int d_obs_id;
	double d_obs_ra;
	double d_obs_dec;
	float d_obs_alt;
	float d_obs_az;
	int d_pos_ind;
	EXEC SQL END DECLARE SECTION;

	struct ln_hrz_posn hrz;

	if (update_position)
		getPosition (position);

	if (getObsId () > 0)		 // we already observe that target
		return OBS_ALREADY_STARTED;

	if (isnan (position->ra) || isnan (position->dec))
	{
		d_obs_ra = d_obs_dec = d_obs_alt = d_obs_az = NAN;
		d_pos_ind = -1;
	}
	else
	{
		d_obs_ra = position->ra;
		d_obs_dec = position->dec;
		ln_get_hrz_from_equ (position, observer, ln_get_julian_from_sys (), &hrz);
		d_obs_alt = hrz.alt;
		d_obs_az = hrz.az;
		d_pos_ind = 0;
	}

	EXEC SQL
		SELECT
		nextval ('obs_id')
		INTO
			:d_obs_id;
	EXEC SQL
		INSERT INTO
			observations
			(
			tar_id,
			obs_id,
			obs_slew,
			obs_ra,
			obs_dec,
			obs_alt,
			obs_az
			)
		VALUES
			(
			:d_tar_id,
			:d_obs_id,
			now (),
			:d_obs_ra :d_pos_ind,
			:d_obs_dec :d_pos_ind,
			:d_obs_alt :d_pos_ind,
			:d_obs_az :d_pos_ind
			);
	if (sqlca.sqlcode != 0)
	{
		logMsgDb ("cannot insert observation slew start to db", MESSAGE_ERROR);
		EXEC SQL ROLLBACK;
		return OBS_MOVE_FAILED;
	}
	EXEC SQL COMMIT;
	setObsId (d_obs_id);
	observation = new Observation (d_obs_id);
	return afterSlewProcessed ();
}

int Target::newObsSlew (struct ln_equ_posn *position)
{
	endObservation (-1);
	delete observation;
	observation = NULL;
	nullImgId ();
	return updateSlew (position);
}

int Target::updateSlew (struct ln_equ_posn *position)
{
	if (observation == NULL)
		return startSlew (position, false) == OBS_MOVE_FAILED ? -1 : 0;

	struct ln_hrz_posn hrz;
	ln_get_hrz_from_equ (position, observer, ln_get_julian_from_sys (), &hrz);
	try
	{
		observation->startSlew (position, &hrz);
	}
	catch (SqlError &er)
	{
		logStream (MESSAGE_ERROR) << er << sendLog;
		return -1;
	}
	return 0;
}

moveType Target::afterSlewProcessed ()
{
	return OBS_MOVE;
}

int Target::startObservation ()
{
	if (observationStarted ())
		return 0;
	time (&observationStart);
	if (observation && getObsId () > 0)
	{
		observation->startObservation ();
		obsStarted ();
	}
	return 0;
}

int Target::endObservation (int in_next_id, rts2core::Block *master)
{
	int old_obs_id = getObsId ();

	int ret = endObservation (in_next_id);

	// check if that was the last observation..
	Observation out_observation = Observation (old_obs_id);
	out_observation.checkUnprocessedImages (master);

	return ret;
}

int Target::endObservation (int in_next_id)
{
	if (isContinues () == 1 && in_next_id == getTargetID ())
		return 1;
	if (getObsId () > 0)
	{
		observation->endObservation (getObsState ());
		setObsId (-1);
	}
	observationStart = -1;
	return 0;
}

int Target::isContinues ()
{
	return 0;
}

int Target::observationStarted ()
{
	return (observationStart > 0) ? 1 : 0;
}

int Target::secToObjectSet (double JD)
{
	struct ln_rst_time rst;
	int ret;
	ret = getRST (&rst, JD, getMinObsAlt ());
	if (ret)
		return -1;				 // don't rise, circumpolar etc..
	ret = int ((rst.set - JD) * 86400.0);
	if (ret < 0)
		// hope we get current set, we are interested in next set..
		return ret + ((int (ret/86400) + 1) * -86400);
	return ret;
}

int Target::secToObjectRise (double JD)
{
	struct ln_rst_time rst;
	int ret;
	ret = getRST (&rst, JD, getMinObsAlt ());
	if (ret)
		return -1;				 // don't rise, circumpolar etc..
	ret = int ((rst.rise - JD) * 86400.0);
	if (ret < 0)
		// hope we get current set, we are interested in next set..
		return ret + ((int (ret/86400) + 1) * -86400);
	return ret;
}

int Target::secToObjectMeridianPass (double JD)
{
	struct ln_rst_time rst;
	int ret;
	ret = getRST (&rst, JD, getMinObsAlt ());
	if (ret)
		return -1;				 // don't rise, circumpolar etc..
	ret = int ((rst.transit - JD) * 86400.0);
	if (ret < 0)
		// hope we get current set, we are interested in next set..
		return ret + ((int (ret/86400) + 1) * -86400);
	return ret;
}

bool Target::isVisibleDuringNight (double jd, double horizon)
{
	struct ln_rst_time rst;
	int ret;
	Rts2Night night (jd, getObserver ());

	ret = getRST (&rst, night.getJDFrom (), horizon);
	// not visible at all
	if (ret < 0)
		return false;
	// circumpolar
	else if (ret > 0)
		return true;
	if (night.getJDFrom () < rst.set && night.getJDTo () > rst.set)
		return true;
	if (night.getJDFrom () < rst.rise && night.getJDTo () > rst.rise)
		return true;
	if (night.getJDFrom () < rst.transit && night.getJDTo () > rst.transit)
		return true;
	return false;
}

int Target::beforeMove ()
{
	startCalledNum++;
	return 0;
}

int Target::postprocess ()
{
	return 0;
}

void Target::getDBScript (const char *camera_name, std::string &script)
{
	EXEC SQL BEGIN DECLARE SECTION;
		int tar_id = getTargetID ();
		VARCHAR d_camera_name[8];
		VARCHAR sc_script[2000];
		int sc_indicator;
	EXEC SQL END DECLARE SECTION;

	d_camera_name.len = strlen (camera_name);
	strncpy (d_camera_name.arr, camera_name, d_camera_name.len);

	EXEC SQL DECLARE find_script CURSOR FOR
		SELECT
			script
		FROM
			scripts
		WHERE
			tar_id = :tar_id AND camera_name = :d_camera_name;
	EXEC SQL OPEN find_script;
	EXEC SQL FETCH next FROM find_script INTO
		:sc_script :sc_indicator;

	if (sqlca.sqlcode == ECPG_NOT_FOUND)
	{
		EXEC SQL CLOSE find_script;
		EXEC SQL ROLLBACK;
		throw SqlError ();
	}

	if (sqlca.sqlcode || sc_indicator < 0)
	{
		logStream (MESSAGE_ERROR) << "while loading script for device " << camera_name << " and target " << tar_id << " : " << sqlca.sqlerrm.sqlerrmc << sendLog;
		EXEC SQL CLOSE find_script;
		EXEC SQL ROLLBACK;
		throw SqlError ();
	}

	sc_script.arr[sc_script.len] = '\0';
	script = std::string (sc_script.arr);

	EXEC SQL CLOSE find_script;
	EXEC SQL ROLLBACK;
}

bool Target::getScript (const char *device_name, std::string &buf)
{
	int ret;
	rts2core::Configuration *config;
	config = rts2core::Configuration::instance ();

	try
	{
		getDBScript (device_name, buf);
		return false;
	}
	catch (rts2core::Error &er)
	{
	}

	ret = config->getString (device_name, "script", buf);
	if (!ret)
		return false;
	throw DeviceMissingExcetion (device_name);
}

void Target::setScript (const char *device_name, const char *buf)
{
	EXEC SQL BEGIN DECLARE SECTION;
	VARCHAR d_camera_name[8];
	VARCHAR d_script[2000];
	int d_tar_id = getTargetID ();
	EXEC SQL END DECLARE SECTION;

	d_camera_name.len = strlen (device_name);
	if (d_camera_name.len > 8)
		d_camera_name.len = 8;
	strncpy (d_camera_name.arr, device_name, 8);

	d_script.len = strlen (buf);
	if (d_script.len > 2000)
		d_script.len = 2000;
	strncpy (d_script.arr, buf, d_script.len);

	EXEC SQL
		INSERT INTO scripts
			(
			camera_name,
			tar_id,
			script
			)
		VALUES
			(
			:d_camera_name,
			:d_tar_id,
			:d_script
			);
	// insert failed - try update
	if (sqlca.sqlcode)
	{
		EXEC SQL ROLLBACK;
		EXEC SQL UPDATE
				scripts
			SET
				script = :d_script
			WHERE
				camera_name = :d_camera_name
			AND tar_id = :d_tar_id;
		if (sqlca.sqlcode)
		{
			if (sqlca.sqlcode == ECPG_NOT_FOUND)
				throw CameraMissingExcetion (device_name);
			EXEC SQL ROLLBACK;
			throw SqlError ();
		}
	}
	EXEC SQL COMMIT;
}

std::string Target::getPIName ()
{
	return labels.getTargetLabels (getTargetID (), LABEL_PI).getString ("not set", ",");
}

void Target::setPIName (const char *name)
{
	deleteLabels (LABEL_PI);
 	addLabel (name, LABEL_PI, true);
}

std::string Target::getProgramName ()
{
	return labels.getTargetLabels (getTargetID (), LABEL_PROGRAM).getString ("not set", ",");
}

void Target::setProgramName (const char *program)
{
	deleteLabels (LABEL_PROGRAM);
 	addLabel (program, LABEL_PROGRAM, true);
}

void Target::setConstraints (Constraints &cons)
{
	int ret = mkpath (getConstraintFile (), 0777);
	if (ret)
		throw rts2core::Error ((std::string ("cannot create directory for ") + getConstraintFile () + " : " + strerror (errno)).c_str ());

	std::ofstream ofs;
	
	ofs.exceptions ( std::ofstream::eofbit | std::ofstream::failbit | std::ofstream::badbit );
	try
	{
		ofs.open (getConstraintFile ());

		ofs << "<?xml version=\"1.0\"?>" << std::endl << std::endl;
		cons.printXML (ofs);
		ofs.close ();
	}
	catch (std::ofstream::failure f)
	{
		throw rts2core::Error ((std::string ("cannot write constraint file ") + getConstraintFile () + " : " + strerror (errno)).c_str ());
	}
}

void Target::appendConstraints (Constraints &cons)
{
  	Constraints tarc (cons);
	try
	{
		tarc.load (getConstraintFile (), false);
	}
	catch (XmlError er)
	{
		logStream (MESSAGE_WARNING) << er << sendLog;
	}
	setConstraints (tarc);
}

void Target::getAltAz (struct ln_hrz_posn *hrz, double JD, struct ln_lnlat_posn *obs)
{
	struct ln_equ_posn object;

	getPosition (&object, JD);

	if (isnan (object.ra) || isnan (object.dec))
	{
		hrz->alt = hrz->az = NAN;
	}
	else
	{
		ln_get_hrz_from_equ (&object, obs, JD, hrz);
	}
}

void Target::getMinMaxAlt (double _start, double _end, double &_min, double &_max)
{
	struct ln_equ_posn mid;
	double midJD = (_start + _end) / 2.0;

  	getPosition (&mid, midJD);

	double absLat = fabs (observer->lat);
	// change sign if we are on south hemisphere
	if (observer->lat < 0)
		mid.dec *= -1;

	// difference of JD is larger then 1, for sure we will get a max and min alt..
	if ((_end - _start) >= 1.0)
	{
		// use midJD to calculate DEC at maximum / minimum
		// this is not a correct use
		_min = absLat - 90 + mid.dec;
		_max = 90 - absLat + mid.dec;
		return;
	}

	struct ln_hrz_posn startHrz, endHrz;

	getAltAz (&startHrz, _start);
	getAltAz (&endHrz, _end);

	_min = (startHrz.alt < endHrz.alt) ? startHrz.alt : endHrz.alt;
	_max = (startHrz.alt > endHrz.alt) ? startHrz.alt : endHrz.alt;

	// now compute if object is in low or high transit during given interval
	double midRa = mid.ra;
	double startLst = ln_range_degrees (ln_get_mean_sidereal_time (_start) * 15.0 + observer->lng);
	double endLst = ln_range_degrees (ln_get_mean_sidereal_time (_end) * 15.0 + observer->lng);

	if (startLst > endLst)
	{
		// arange times so they are ordered
		endLst += 360.0;
		if (startLst > midRa)
			midRa += 360.0;
	}

	// it culmitaes during given period
	if (midRa > startLst && midRa < endLst)
		_max = 90 - absLat + mid.dec;
	midRa = ln_range_degrees (midRa + 180);
	if (midRa > endLst + 360)
		midRa = endLst - 360;
	// it pass low point during given period
	if (midRa > startLst && midRa < endLst)
		_min = absLat - 90 + mid.dec;
}

void Target::getGalLng (struct ln_gal_posn *gal, double JD)
{
	struct ln_equ_posn curr;
	getPosition (&curr, JD);
	ln_get_gal_from_equ (&curr, gal);
}

double Target::getGalCenterDist (double JD)
{
	static struct ln_equ_posn cntr = { 265.610844, -28.916790 };
	struct ln_equ_posn curr;
	getPosition (&curr, JD);
	return ln_get_angular_separation (&curr, &cntr);
}

double Target::getAirmass (double JD)
{
	struct ln_hrz_posn hrz;
	getAltAz (&hrz, JD);
	return ln_get_airmass (hrz.alt, airmassScale);
}

double Target::getZenitDistance (double JD)
{
	struct ln_hrz_posn hrz;
	getAltAz (&hrz, JD);
	return 90.0 - hrz.alt;
}

double Target::getHourAngle (double JD, struct ln_lnlat_posn *obs)
{
	double lst;
	double ha;
	struct ln_equ_posn pos;
	lst = ln_get_mean_sidereal_time (JD) * 15.0 + obs->lng;
	getPosition (&pos);
	ha = ln_range_degrees (lst - pos.ra);
	if (ha > 180)
		ha -= 360;
	return ha;
}

double Target::getDistance (struct ln_equ_posn *in_pos, double JD)
{
	struct ln_equ_posn object;
	getPosition (&object, JD);
	return ln_get_angular_separation (&object, in_pos);
}

double Target::getRaDistance (struct ln_equ_posn *in_pos, double JD)
{
	struct ln_equ_posn object;
	getPosition (&object, JD);
	return ln_range_degrees (object.ra - in_pos->ra);
}

double Target::getSolarDistance (double JD)
{
	struct ln_equ_posn eq_sun;
	ln_get_solar_equ_coords (JD, &eq_sun);
	return getDistance (&eq_sun, JD);
}

double Target::getSolarRaDistance (double JD)
{
	struct ln_equ_posn eq_sun;
	ln_get_solar_equ_coords (JD, &eq_sun);
	return getRaDistance (&eq_sun, JD);
}

double Target::getLunarDistance (double JD)
{
	struct ln_equ_posn moon;
	ln_get_lunar_equ_coords (JD, &moon);
	return getDistance (&moon, JD);
}

double Target::getLunarRaDistance (double JD)
{
	struct ln_equ_posn moon;
	ln_get_lunar_equ_coords (JD, &moon);
	return getRaDistance (&moon, JD);
}

/**
 * This method is called to check that target which was selected as good is
 * still the best.
 *
 * It should reload from DB values, which are important for selection process,
 * and if they indicate that target should not be observed, it should return -1.
 */
int Target::selectedAsGood ()
{
	EXEC SQL BEGIN DECLARE SECTION;
		bool d_tar_enabled;
		int d_tar_id = getTargetID ();
		float d_tar_priority;
		int d_tar_priority_ind;
		float d_tar_bonus;
		int d_tar_bonus_ind;
		long d_tar_next_observable;
		int d_tar_next_observable_ind;
	EXEC SQL END DECLARE SECTION;

	// check if we are still enabled..
	EXEC SQL
		SELECT
			tar_enabled,
			tar_priority,
			tar_bonus,
			EXTRACT (EPOCH FROM tar_next_observable)
		INTO
			:d_tar_enabled,
			:d_tar_priority :d_tar_priority_ind,
			:d_tar_bonus :d_tar_bonus_ind,
			:d_tar_next_observable :d_tar_next_observable_ind
		FROM
			targets
		WHERE
			tar_id = :d_tar_id;
	if (sqlca.sqlcode)
	{
		logMsgDb ("Target::selectedAsGood", MESSAGE_ERROR);
		return -1;
	}
	setTargetEnabled (d_tar_enabled, false);
	if (d_tar_priority_ind >= 0)
		tar_priority = d_tar_priority;
	else
		tar_priority = 0;
	if (d_tar_bonus_ind >= 0)
		tar_bonus = d_tar_bonus;
	else
		tar_bonus = 0;

	if (getTargetEnabled () && tar_priority + tar_bonus >= 0)
	{
		if (d_tar_next_observable_ind >= 0)
		{
			time_t now;
			time (&now);
			if (now < d_tar_next_observable)
				return -1;
		}
		return 0;
	}
	return -1;
}

/**
 * return 0 if we cannot observe that target, 1 if it's above horizon.
 */
bool Target::isGood (double lst, double JD, struct ln_equ_posn * pos)
{
	struct ln_hrz_posn hrz;
	getAltAz (&hrz, JD);
	return isAboveHorizon (&hrz);
}

bool Target::isGood (double JD)
{
	struct ln_equ_posn pos;
	getPosition (&pos, JD);
	return isGood (ln_get_mean_sidereal_time (JD) + observer->lng / 15.0, JD, &pos);
}

bool Target::isAboveHorizon (struct ln_hrz_posn *hrz)
{
	// assume that undefined objects are always above horizon
	if (isnan (hrz->alt))
		return true;
	if (hrz->alt < getMinObsAlt ())
		return false;
	return rts2core::Configuration::instance ()->getObjectChecker ()->is_good (hrz);
}

int Target::considerForObserving (double JD)
{
	// horizon constrain..
	struct ln_equ_posn curr_position;
	double lst = ln_get_mean_sidereal_time (JD) + observer->lng / 15.0;
	int ret;
	getPosition (&curr_position, JD);

	ret = isGood (lst, JD, &curr_position);
	if (!ret)
	{
		struct ln_rst_time rst;
		ret = getRST (&rst, JD, getMinObsAlt ());
		if (ret == -1)
		{
			// object doesn't rise, let's hope tomorrow it will rise
			logStream (MESSAGE_DEBUG)
				<< "Target::considerForObserving tar " << getTargetID ()
				<< " obstarid " << getObsTargetID ()
				<< " don't rise"
				<< sendLog;
			setNextObservable (JD + 1);
			return -1;
		}
		// handle circumpolar objects..
		if (ret == 1)
		{
			logStream (MESSAGE_DEBUG) << "Target::considerForObserving tar "
				<< getTargetID ()
				<< " obstarid " << getObsTargetID ()
				<< " is circumpolar, but is not good, scheduling after 10 minutes"
				<< sendLog;
			setNextObservable (JD + 10.0 / (24.0 * 60.0));
			return -1;
		}
		// object is above horizon, but checker reject it..let's see what
		// will hapens in 12 minutes
		if (rst.transit < rst.set && rst.set < rst.rise)
		{
			// object rose, but is not above horizon, let's hope in 12 minutes it will get above horizon
			logStream (MESSAGE_DEBUG) << "Target::considerForObserving "
				<< getTargetID ()
				<< " obstarid " << getObsTargetID ()
				<< " will rise tommorow: " << LibnovaDate (rst.rise)
				<< " JD:" << JD
				<< sendLog;
			setNextObservable (JD + 12*(1.0/1440.0));
			return -1;
		}
		// object is setting, let's target it for next rise..
		logStream (MESSAGE_DEBUG)
			<< "Target::considerForObserving " << getTargetID ()
			<< " obstarid " << getObsTargetID ()
			<< " will rise at: " << LibnovaDate (rst.rise)
			<< sendLog;
		setNextObservable (rst.rise);
		return -1;
	}
	// target was selected for observation
	return selectedAsGood ();
}

int Target::dropBonus ()
{
	EXEC SQL BEGIN DECLARE SECTION;
		int db_tar_id;
	EXEC SQL END DECLARE SECTION;

	EXEC SQL UPDATE
			targets
		SET
			tar_bonus = NULL,
			tar_bonus_time = NULL
		WHERE
			tar_id = :db_tar_id;
	if (sqlca.sqlcode)
	{
		logMsgDb ("Target::dropBonus", MESSAGE_ERROR);
		EXEC SQL ROLLBACK;
		return -1;
	}
	EXEC SQL COMMIT;
	return 0;
}

float Target::getBonus (double JD)
{
	return tar_priority + tar_bonus;
}

int Target::changePriority (int pri_change, time_t *time_ch)
{
	EXEC SQL BEGIN DECLARE SECTION;
		int db_tar_id = getObsTargetID ();
		int db_priority_change = pri_change;
		int db_next_t = (int) *time_ch;
	EXEC SQL END DECLARE SECTION;

	EXEC SQL UPDATE
			targets
		SET
			tar_bonus = tar_bonus + :db_priority_change,
			tar_bonus_time = to_timestamp(:db_next_t)
		WHERE
			tar_id = :db_tar_id;
	if (sqlca.sqlcode)
	{
		logMsgDb ("Target::changePriority", MESSAGE_ERROR);
		EXEC SQL ROLLBACK;
		return -1;
	}
	EXEC SQL COMMIT;
	return 0;
}

int Target::changePriority (int pri_change, double validJD)
{
	time_t next;
	ln_get_timet_from_julian (validJD, &next);
	return changePriority (pri_change, &next);
}

int Target::setNextObservable (time_t *time_ch)
{
	EXEC SQL BEGIN DECLARE SECTION;
	int db_tar_id = getObsTargetID ();
	int db_next_observable;
	int db_next_observable_ind;
	EXEC SQL END DECLARE SECTION;

	if (time_ch)
	{
		db_next_observable = (int) *time_ch;
		db_next_observable_ind = 0;
		tar_next_observable = *time_ch;
	}
	else
	{
		db_next_observable = 0;
		db_next_observable_ind = -1;
		tar_next_observable = 0;
	}

	EXEC SQL UPDATE
		targets
	SET
		tar_next_observable = to_timestamp(:db_next_observable :db_next_observable_ind)
	WHERE
		tar_id = :db_tar_id;
	if (sqlca.sqlcode)
	{
		logMsgDb ("Target::setNextObservable", MESSAGE_ERROR);
		EXEC SQL ROLLBACK;
		return -1;
	}
	EXEC SQL COMMIT;
	return 0;
}

int Target::setNextObservable (double validJD)
{
	time_t next;
	ln_get_timet_from_julian (validJD, &next);
	return setNextObservable (&next);
}

int Target::getNumObs (time_t *start_time, time_t *end_time)
{
	EXEC SQL BEGIN DECLARE SECTION;
		int d_start_time = (int) *start_time;
		int d_end_time = (int) *end_time;
		int d_count;
		int d_tar_id = getTargetID ();
	EXEC SQL END DECLARE SECTION;

	EXEC SQL
		SELECT
		count (*)
		INTO
			:d_count
		FROM
			observations
		WHERE
			tar_id = :d_tar_id
		AND obs_slew >= to_timestamp(:d_start_time)
		AND (obs_end is null
		OR obs_end <= to_timestamp(:d_end_time)
			);
	// runnign observations counts as well - hence obs_end is null

	EXEC SQL COMMIT;

	return d_count;
}

int Target::getTotalNumberOfObservations ()
{
	EXEC SQL BEGIN DECLARE SECTION;
	int d_count;
	int d_tar_id = getTargetID ();
	EXEC SQL END DECLARE SECTION;

	EXEC SQL
	SELECT
		count (*)
	INTO
		:d_count
	FROM
		observations
	WHERE
		tar_id = :d_tar_id;
	// runnign observations counts as well - hence obs_end is null

	EXEC SQL COMMIT;

	return d_count;
}

double Target::getLastObsTime ()
{
	EXEC SQL BEGIN DECLARE SECTION;
		int d_tar_id = getTargetID ();
		double d_time_diff;
	EXEC SQL END DECLARE SECTION;

	EXEC SQL
		SELECT
		min (EXTRACT (EPOCH FROM (now () - obs_slew)))
		INTO
			:d_time_diff
		FROM
			observations
		WHERE
			tar_id = :d_tar_id
		GROUP BY
			tar_id;

	if (sqlca.sqlcode)
	{
		if (sqlca.sqlcode == ECPG_NOT_FOUND)
		{
			// 1 year was the last observation..
			return NAN;
		}
		else
			logMsgDb ("Target::getLastObsTime", MESSAGE_ERROR);
	}
	
	EXEC SQL COMMIT;

	return d_time_diff;
}

int Target::getTotalNumberOfImages ()
{
	EXEC SQL BEGIN DECLARE SECTION;
	int d_count = 0;
	int d_tar_id = getTargetID ();
	EXEC SQL END DECLARE SECTION;

	EXEC SQL
	SELECT
		count (*)
	INTO
		:d_count
	FROM
		observations,images
	WHERE
		observations.tar_id = :d_tar_id AND images.obs_id = observations.obs_id;
	// runnign observations counts as well - hence obs_end is null

	EXEC SQL COMMIT;

	return d_count;
}

double Target::getTotalOpenTime ()
{
	EXEC SQL BEGIN DECLARE SECTION;
	double total;
	int d_tar_id = getTargetID ();
	EXEC SQL END DECLARE SECTION;

	EXEC SQL
	SELECT
		sum (images.img_exposure)
	INTO
		:total
	FROM
		observations,images
	WHERE
		observations.tar_id = :d_tar_id AND images.obs_id = observations.obs_id;
	// runnign observations counts as well - hence obs_end is null

	EXEC SQL COMMIT;

	return total;
}

double Target::getFirstObs ()
{
	EXEC SQL BEGIN DECLARE SECTION;
		int db_tar_id = getTargetID ();
		double ret;
	EXEC SQL END DECLARE SECTION;
	EXEC SQL
		SELECT
		MIN (EXTRACT (EPOCH FROM obs_start))
		INTO
			:ret
		FROM
			observations
		WHERE
			tar_id = :db_tar_id;
	if (sqlca.sqlcode)
	{
		EXEC SQL ROLLBACK;
		return NAN;
	}
	EXEC SQL ROLLBACK;
	return ret;
}

double Target::getLastObs ()
{
	EXEC SQL BEGIN DECLARE SECTION;
	int db_tar_id = getTargetID ();
	double ret;
	EXEC SQL END DECLARE SECTION;
	EXEC SQL
	SELECT
		MAX (EXTRACT (EPOCH FROM obs_start))
	INTO
		:ret
	FROM
		observations
	WHERE
		tar_id = :db_tar_id;
	if (sqlca.sqlcode)
	{
		EXEC SQL ROLLBACK;
		return NAN;
	}
	EXEC SQL ROLLBACK;
	return ret;
}

void Target::printExtra (Rts2InfoValStream &_os, double JD)
{
	if (getTargetEnabled ())
	{
		_os << "Target is enabled" << std::endl;
	}
	else
	{
		_os << "Target is disabled" << std::endl;
	}
	_os
		<< std::endl
		<< InfoVal<const char*> ("IS VISIBLE TONIGHT", isVisibleDuringNight (JD, getMinObsAlt ()) ? "yes" : "no")
		<< std::endl
		<< InfoVal<double> ("TARGET PRIORITY", tar_priority)
		<< InfoVal<double> ("TARGET BONUS", tar_bonus)
		<< InfoVal<Timestamp> ("TARGET BONUS TIME", Timestamp(tar_bonus_time))
		<< InfoVal<Timestamp> ("TARGET NEXT OBS.", Timestamp(tar_next_observable))
		<< std::endl;
}

void Target::printShortInfo (std::ostream & _os, double JD)
{
	struct ln_equ_posn pos;
	struct ln_hrz_posn hrz;
	const char * name = getTargetName ();
	int old_prec = _os.precision (2);
	getPosition (&pos, JD);
	getAltAz (&hrz, JD);
	LibnovaRaDec raDec (&pos);
	LibnovaHrz hrzP (&hrz);
	double h = getHourAngle (JD);
	LibnovaHA ha (h);
	_os
		<< std::setw (5) << getTargetID () << SEP
		<< getTargetType () << SEP
		<< raDec << SEP << ha << SEP;
	writeAirmass (_os, JD);
	_os << SEP << hrzP << SEP;
	if (h < -30)
		_os << "rising    ";
	else if (h > 30)
		_os << "setting   ";
	else
		_os << "transiting";

	_os << SEP;
	_os << std::left << std::setw (25) << (name ? name :  "null") << std::right << SEP;
	_os.precision (old_prec);
}

void Target::printDS9Reg (std::ostream & _os, double JD)
{
	struct ln_equ_posn pos;
	getPosition (&pos, JD);
	_os << "circle(" << pos.ra << "," << pos.dec << "," << getSDiam (JD) << ")" << std::endl;
}

void Target::printShortBonusInfo (std::ostream & _os, double JD)
{
	struct ln_equ_posn pos;
	struct ln_hrz_posn hrz;
	const char * name = getTargetName ();
	int old_prec = _os.precision (2);
	getPosition (&pos, JD);
	getAltAz (&hrz, JD);
	LibnovaRaDec raDec (&pos);
	LibnovaHrz hrzP (&hrz);
	_os
		<< std::setw (5) << getTargetID () << SEP
		<< std::setw (7) << getBonus (JD) << SEP
		<< getTargetType () << SEP
		<< std::left << std::setw (40) << (name ? name :  "null") << std::right << SEP
		<< raDec << SEP;
	writeAirmass (_os, JD);
	_os
		<< SEP << hrzP;
	_os.precision (old_prec);
}

int Target::printObservations (double radius, double JD, std::ostream &_os)
{
	struct ln_equ_posn tar_pos;
	getPosition (&tar_pos, JD);

	ObservationSet obsset = ObservationSet ();
	obsset.loadRadius (&tar_pos, radius);
	_os << obsset;

	return obsset.size ();
}

TargetSet Target::getTargets (double radius)
{
	return getTargets (radius, ln_get_julian_from_sys ());
}

TargetSet Target::getTargets (double radius, double JD)
{
	struct ln_equ_posn tar_pos;
	getPosition (&tar_pos, JD);

	TargetSet tarset = TargetSet (&tar_pos, radius);
	tarset.load ();
	return tarset;
}

int Target::printTargets (double radius, double JD, std::ostream &_os)
{
	TargetSet tarset = getTargets (radius, JD);
	_os << tarset;

	return tarset.size ();
}

int Target::printImages (double JD, std::ostream &_os, int flags)
{
	struct ln_equ_posn tar_pos;
	int ret;

	getPosition (&tar_pos, JD);

	ImageSetPosition img_set = ImageSetPosition (&tar_pos);
	ret = img_set.load ();
	if (ret)
		return ret;

	img_set.print (std::cout, flags);

	return img_set.size ();
}

Target *createTarget (int _tar_id, struct ln_lnlat_posn *_obs, rts2core::ConnNotify *watchConn)
{
	EXEC SQL BEGIN DECLARE SECTION;
	int db_tar_id = _tar_id;
	char db_type_id;
	EXEC SQL END DECLARE SECTION;

	Target *retTarget;

	EXEC SQL
	SELECT
		type_id
	INTO
		:db_type_id
	FROM
		targets
	WHERE
		tar_id = :db_tar_id;

	if (sqlca.sqlcode)
	{
	  	std::ostringstream err;
		err << "target with ID " << db_tar_id << " does not exists";
	  	throw SqlError (err.str ().c_str ());
	}

	// get more informations about target..
	switch (db_type_id)
	{
		// calibration targets..
		case TYPE_DARK:
			retTarget = new DarkTarget (_tar_id, _obs);
			break;
		case TYPE_FLAT:
			retTarget = new FlatTarget (_tar_id, _obs);
			break;
		case TYPE_CALIBRATION:
			retTarget = new CalibrationTarget (_tar_id, _obs);
			break;
		case TYPE_MODEL:
			retTarget = new ModelTarget (_tar_id, _obs);
			break;
		case TYPE_OPORTUNITY:
			retTarget = new OportunityTarget (_tar_id, _obs);
			break;
		case TYPE_ELLIPTICAL:
			retTarget = new EllTarget (_tar_id, _obs);
			break;
		case TYPE_GRB:
			retTarget = new TargetGRB (_tar_id, _obs, 3600, 86400, 5 * 86400);
			break;
		case TYPE_SWIFT_FOV:
			retTarget = new TargetSwiftFOV (_tar_id, _obs);
			break;
		case TYPE_INTEGRAL_FOV:
			retTarget = new TargetIntegralFOV (_tar_id, _obs);
			break;
		case TYPE_GPS:
			retTarget = new TargetGps (_tar_id, _obs);
			break;
		case TYPE_SKY_SURVEY:
			retTarget = new TargetSkySurvey (_tar_id, _obs);
			break;
		case TYPE_TERESTIAL:
			retTarget = new TargetTerestial (_tar_id, _obs);
			break;
		case TYPE_PLAN:
			retTarget = new TargetPlan (_tar_id, _obs);
			break;
		case TYPE_AUGER:
			retTarget = new TargetAuger (_tar_id, _obs, 1800);
			break;
		case TYPE_PLANET:
			retTarget = new TargetPlanet (_tar_id, _obs);
			break;
		default:
			retTarget = new ConstTarget (_tar_id, _obs);
			break;
	}

	if (watchConn)
		retTarget->setWatchConnection (watchConn);

	retTarget->setTargetType (db_type_id);
	retTarget->load ();
	EXEC SQL COMMIT;
	return retTarget;
}

Target *createTargetByName (const char *tar_name, struct ln_lnlat_posn * obs)
{
	TargetSet ts (obs);
	ts.loadByName (tar_name);
	if (ts.size () == 1)
	{
		Target *ret = ts.begin ()->second;
		ts.clear ();
		return ret;
	}
	return NULL;
}

void Target::sendPositionInfo (Rts2InfoValStream &_os, double JD, int extended)
{
	struct ln_hrz_posn hrz;
	struct ln_gal_posn gal;
	struct ln_rst_time rst;
	double hourangle;
	time_t now;
	int ret;

	ln_get_timet_from_julian (JD, &now);

	getAltAz (&hrz, JD);
	hourangle = getHourAngle (JD);
	_os
		<< InfoVal<LibnovaDeg90> ("ALTITUDE", LibnovaDeg90 (hrz.alt))
		<< InfoVal<LibnovaDeg90> ("ZENITH DISTANCE", LibnovaDeg90 (getZenitDistance (JD)))
		<< InfoVal<LibnovaDeg360> ("AZIMUTH", LibnovaDeg360 (hrz.az))
		<< InfoVal<LibnovaRa> ("HOUR ANGLE", LibnovaRa (hourangle))
		<< InfoVal<double> ("AIRMASS", getAirmass (JD))
		<< std::endl;

	ret = getRST (&rst, JD, LN_STAR_STANDART_HORIZON);
	switch (ret)
	{
		case 1:
			_os << " - CIRCUMPOLAR - " << std::endl;
			rst.transit = JD + ((360 - hourangle) / 15.0 / 24.0);
			_os << InfoVal<TimeJDDiff> ("TRANSIT", TimeJDDiff (rst.transit, now));
			break;
		case -1:
			_os << " - DON'T RISE - " << std::endl;
			break;
		default:
			if (rst.set < rst.rise && rst.rise < rst.transit)
			{
				_os
					<< InfoVal<TimeJDDiff> ("SET", TimeJDDiff (rst.set, now))
					<< InfoVal<TimeJDDiff> ("RISE", TimeJDDiff (rst.rise, now))
					<< InfoVal<TimeJDDiff> ("TRANSIT", TimeJDDiff (rst.transit, now));
			}
			else if (rst.transit < rst.set && rst.set < rst.rise)
			{
				_os
					<< InfoVal<TimeJDDiff> ("TRANSIT", TimeJDDiff (rst.transit, now))
					<< InfoVal<TimeJDDiff> ("SET", TimeJDDiff (rst.set, now))
					<< InfoVal<TimeJDDiff> ("RISE", TimeJDDiff (rst.rise, now));
			}
			else
			{
				_os
					<< InfoVal<TimeJDDiff> ("RISE", TimeJDDiff (rst.rise, now))
					<< InfoVal<TimeJDDiff> ("TRANSIT", TimeJDDiff (rst.transit, now))
					<< InfoVal<TimeJDDiff> ("SET", TimeJDDiff (rst.set, now));
			}
	}

	// test min-max routines
	double maxAlt, minAlt;
	getMinMaxAlt (JD - 1, JD + 1, minAlt, maxAlt);
	_os << InfoVal<LibnovaDeg90> ("MIN ALT", LibnovaDeg90 (minAlt))
		<< InfoVal<LibnovaDeg90> ("MAX ALT", LibnovaDeg90 (maxAlt));

	getMinMaxAlt (JD - 0.2, JD + 0.2, minAlt, maxAlt);
	_os <<  std::endl
		<< InfoVal<LibnovaDeg90> ("MIN ALT", LibnovaDeg90 (minAlt))
		<< InfoVal<LibnovaDeg90> ("MAX ALT", LibnovaDeg90 (maxAlt));


	_os << std::endl
		<< InfoVal<double> ("MIN_OBS_ALT", getMinObsAlt ())
		<< std::endl
		<< "RISE, SET AND TRANSIT ABOVE MIN_ALT"
		<< std::endl
		<< std::endl;

	ret = getRST (&rst, JD, getMinObsAlt ());
	switch (ret)
	{
		case 1:
			_os << " - CIRCUMPOLAR - " << std::endl;
			rst.transit = JD + ((360 - hourangle) / 15.0 / 24.0);
			_os << InfoVal<TimeJDDiff> ("TRANSIT", TimeJDDiff (rst.transit, now));
			break;
		case -1:
			_os << " - DON'T RISE - " << std::endl;
			break;
		default:
			if (rst.set < rst.rise && rst.rise < rst.transit)
			{
				_os
					<< InfoVal<TimeJDDiff> ("SET", TimeJDDiff (rst.set, now))
					<< InfoVal<TimeJDDiff> ("RISE", TimeJDDiff (rst.rise, now))
					<< InfoVal<TimeJDDiff> ("TRANSIT", TimeJDDiff (rst.transit, now));
			}
			else if (rst.transit < rst.set && rst.set < rst.rise)
			{
				_os
					<< InfoVal<TimeJDDiff> ("TRANSIT", TimeJDDiff (rst.transit, now))
					<< InfoVal<TimeJDDiff> ("SET", TimeJDDiff (rst.set, now))
					<< InfoVal<TimeJDDiff> ("RISE", TimeJDDiff (rst.rise, now));
			}
			else
			{
				_os
					<< InfoVal<TimeJDDiff> ("RISE", TimeJDDiff (rst.rise, now))
					<< InfoVal<TimeJDDiff> ("TRANSIT", TimeJDDiff (rst.transit, now))
					<< InfoVal<TimeJDDiff> ("SET", TimeJDDiff (rst.set, now));
			}
	}

	if (_os.getStream () && extended > 1)
		printAltTable (*(_os.getStream ()), JD);

	ConstraintsList violated;
	getViolatedConstraints (JD, violated);

	ConstraintsList satisfied;
	getSatisfiedConstraints (JD, satisfied);

	getGalLng (&gal, JD);
	_os
		<< std::endl
		<< InfoVal<LibnovaDeg360> ("GAL. LONGITUDE", LibnovaDeg360 (gal.l))
		<< InfoVal<LibnovaDeg90> ("GAL. LATITUDE", LibnovaDeg90 (gal.b))
		<< InfoVal<LibnovaDeg180> ("GAL. CENTER DIST.", LibnovaDeg180 (getGalCenterDist (JD)))
		<< InfoVal<LibnovaDeg360> ("SOLAR DIST.", LibnovaDeg360 (getSolarDistance (JD)))
		<< InfoVal<LibnovaDeg180> ("SOLAR RA DIST.", LibnovaDeg180 (getSolarRaDistance (JD)))
		<< InfoVal<LibnovaDeg360> ("LUNAR DIST.", LibnovaDeg360 (getLunarDistance (JD)))
		<< InfoVal<LibnovaDeg180> ("LUNAR RA DIST.", LibnovaDeg180 (getLunarRaDistance (JD)))
		<< InfoVal<LibnovaDeg360> ("LUNAR PHASE", LibnovaDeg360 (ln_get_lunar_phase (JD)))
		<< std::endl
		<< InfoVal<const char*> ("SYSTEM CONSTRAINTS", rts2core::Configuration::instance ()->getMasterConstraintFile ())
		<< InfoVal<const char*> ("SYSTEM CONSTRAINTS", (constraintsLoaded & CONSTRAINTS_SYSTEM) ? "used" : "empy/not used")
		<< InfoVal<const char*> ("GROUP CONSTRAINTS", getGroupConstraintFile ())
		<< InfoVal<const char*> ("GROUP CONSTRAINTS", (constraintsLoaded & CONSTRAINTS_GROUP) ? "used" : "empty/not used")
		<< InfoVal<const char*> ("TARGET CONSTRAINTS", getConstraintFile ())
		<< InfoVal<const char*> ("TARGET CONSTRAINTS", (constraintsLoaded & CONSTRAINTS_TARGET) ? "used" : "empty/not used")
		<< InfoVal<const char*> ("CONSTRAINTS", checkConstraints (JD) ? "satisfied" : "not met")
		<< InfoVal<std::string> ("VIOLATED", violated.toString ())
		<< InfoVal<std::string> ("SATISFIED", satisfied.toString ())
		<< std::endl;
}

void Target::sendInfo (Rts2InfoValStream & _os, double JD, int extended)
{
	struct ln_equ_posn pos;
	double gst;
	double lst;
	time_t now, last;

	const char *name = getTargetName ();
	char tar_type[2];
	tar_type[0] = getTargetType ();
	tar_type[1] = '\0';

	ln_get_timet_from_julian (JD, &now);

	getPosition (&pos, JD);

	_os
		<< InfoVal<int> ("ID", getTargetID ())
		<< InfoVal<int> ("SEL_ID", getObsTargetID ())
		<< InfoVal<const char *> ("NAME", (name ? name : "null name"))
		<< InfoVal<const char *> ("TYPE", tar_type)
		<< InfoVal<std::string> ("INFO", tar_info)
		<< InfoVal<std::string> ("PI", getPIName ())
		<< InfoVal<std::string> ("PROGRAM", getProgramName ())
		<< InfoVal<LibnovaRaJ2000> ("RA", LibnovaRaJ2000 (pos.ra))
		<< InfoVal<LibnovaDecJ2000> ("DEC", LibnovaDecJ2000 (pos.dec))
		<< std::endl;

	sendPositionInfo (_os, JD, extended);

	last = now - 86400;
	_os
		<< InfoVal<int> ("24 HOURS OBS", getNumObs (&last, &now));
	last = now - 7 * 86400;
	_os
		<< InfoVal<int> ("7 DAYS OBS", getNumObs (&last, &now))
		<< InfoVal<double> ("BONUS", getBonus (JD));

	// is above horizon?
	gst = ln_get_mean_sidereal_time (JD);
	lst = gst + getObserver()->lng / 15.0;
	_os << (isGood (lst, JD, & pos)
		? "Target is above local horizon."
		: "Target is below local horizon, it's not possible to observe it.")
		<< std::endl;
	printExtra (_os, JD);
}

void Target::sendConstraints (Rts2InfoValStream & _os, double JD)
{
	_os << "Constraints" << std::endl;
	getConstraints ()->printXML (*(_os.getStream ()));
	_os << std::endl;
}

size_t Target::getViolatedConstraints (double JD, ConstraintsList &violated)
{
	return getConstraints ()->getViolated (this, JD, violated);
}

ConstraintsList Target::getViolatedConstraints (double JD)
{
	ConstraintsList ret;
	getViolatedConstraints (JD, ret);
	return ret;
}

bool Target::isViolated (Constraint *cons, double JD)
{
	return !(cons->satisfy (this, JD));
}

size_t Target::getSatisfiedConstraints (double JD, ConstraintsList &satisfied)
{
	return getConstraints ()->getSatisfied (this, JD, satisfied);
}

ConstraintsList Target::getSatisfiedConstraints (double JD)
{
	ConstraintsList ret;
	getSatisfiedConstraints (JD, ret);
	return ret;
}

size_t Target::getAltitudeConstraints (std::map <std::string, std::vector <rts2db::ConstraintDoubleInterval> > &ac)
{
	return getConstraints ()->getAltitudeConstraints (ac);
}

size_t Target::getAltitudeViolatedConstraints (std::map <std::string, std::vector <rts2db::ConstraintDoubleInterval> > &ac)
{
	return getConstraints ()->getAltitudeViolatedConstraints (ac);
}

size_t Target::getTimeConstraints (std::map <std::string, ConstraintPtr> &cons)
{
	return getConstraints ()->getTimeConstraints (cons);
}

void Target::getSatisfiedIntervals (time_t from, time_t to, int length, int step, interval_arr_t &satisfiedIntervals)
{
	getConstraints ()->getSatisfiedIntervals (this, from, to, length, step, satisfiedIntervals);
}

double Target::getSatisfiedDuration (double from, double to, double length, double step)
{
	if (!isnan (satisfiedFrom) && !isnan (satisfiedProbedUntil))
	{
		if (from >= satisfiedFrom && from <= satisfiedProbedUntil)
		{
			// did not find end of interval..
			if (!isinf (satisfiedTo) || isnan (satisfiedTo) || to <= satisfiedProbedUntil)
				return satisfiedTo;
			// don't recalculate full interval
			from = satisfiedProbedUntil;
		}
	}
	satisfiedTo = getConstraints ()->getSatisfiedDuration (this, from, to, length, step);
	// update boundaries where we probed..
	if (isnan (satisfiedFrom) || from < satisfiedFrom)
		satisfiedFrom = from;
	// visible during full interval
	if (isinf (satisfiedTo))
	{
		satisfiedProbedUntil = to;
		// visible during full interval
		return INFINITY;
	}
	// not visible at all..
	else if (isnan (satisfiedTo))
	{
		satisfiedProbedUntil = from;
		return NAN;
	}
	satisfiedProbedUntil = satisfiedTo;
	return satisfiedTo;
}

void Target::getViolatedIntervals (time_t from, time_t to, int length, int step, interval_arr_t &satisfiedIntervals)
{
	getConstraints ()->getViolatedIntervals (this, from, to, length, step, satisfiedIntervals);
}

TargetSet * Target::getCalTargets (double JD, double minaird)
{
	TargetSetCalibration *ret = new TargetSetCalibration (this, JD, minaird);
	ret->load ();
	return ret;
}

void Target::writeToImage (rts2image::Image * image, double JD)
{
	// write lunar distance
	struct ln_equ_posn moon;
	struct ln_hrz_posn hmoon;
	ln_get_lunar_equ_coords (JD, &moon);
	ln_get_hrz_from_equ (&moon, observer, JD, &hmoon);
	image->setValue ("MOONDIST", getDistance (&moon, JD), "angular distance to between observation and the moon");
	image->setValue ("MOONRA", moon.ra, "lunar RA");
	image->setValue ("MOONDEC", moon.dec, "lunar DEC");
	image->setValue ("MOONPHA", ln_get_lunar_phase (JD) / 1.8, "moon phase");
	image->setValue ("MOONALT", hmoon.alt, "lunar altitude");
	image->setValue ("MOONAZ", hmoon.az, "lunar azimuth");
}

const char *Target::getConstraintFile ()
{
	if (constraintFile)
		return constraintFile;

	std::ostringstream os;
	os << rts2core::Configuration::instance ()->getTargetDir () << "/";
	if (rts2core::Configuration::instance ()->getTargetConstraintsWithName ())
		os << getTargetName ();
	else
		os << getTargetID ();
	os << "/constraints.xml";
	constraintFile = new char[os.str ().length () + 1];
	strcpy (constraintFile, os.str ().c_str ());
	return constraintFile;
}

const char *Target::getGroupConstraintFile ()
{
	if (groupConstraintFile)
		return groupConstraintFile;
	
	std::ostringstream os;
	os << rts2core::Configuration::instance ()->getTargetDir () << "/groups/" << getTargetType () << ".xml";
	groupConstraintFile = new char[os.str ().length () + 1];
	strcpy (groupConstraintFile, os.str ().c_str ());
	return groupConstraintFile;
}

Constraints * Target::getConstraints ()
{
	// find in cache..
	Constraints *ret = MasterConstraints::getTargetConstraints (getTargetID ());
	if (ret)
		return ret;

	constraintsLoaded = CONSTRAINTS_NONE;
	watchIDs.clear ();

	struct stat fs;
	// check for system level constraints
	try
	{
		ret = new Constraints (MasterConstraints::getConstraint ());
		constraintsLoaded |= CONSTRAINTS_SYSTEM;
		addWatch (rts2core::Configuration::instance ()->getMasterConstraintFile ());
	}
	catch (XmlError er)
	{
		logStream (MESSAGE_WARNING) << "cannot load master constraint file:" << er << sendLogNoEndl;
		ret = new Constraints ();
	}
	if (stat (getGroupConstraintFile (), &fs))
	{
		logStream (errno == ENOENT ? MESSAGE_DEBUG : MESSAGE_WARNING) << "cannot open " << getGroupConstraintFile () << ":" << strerror(errno) << sendLog;
	}
	else
	{
		// check for group constraint
		try
		{
			ret->load (getGroupConstraintFile ());
			constraintsLoaded |= CONSTRAINTS_GROUP;
			addWatch (getGroupConstraintFile ());
		}
		catch (XmlError er)
		{
			logStream (MESSAGE_WARNING) << "cannot load group constraint file " << getGroupConstraintFile () << ":" << er << sendLogNoEndl;
		}
	}
	// load target constrainst
	try
	{
		ret->load (getConstraintFile ());
		constraintsLoaded |= CONSTRAINTS_TARGET;
		addWatch (getConstraintFile ());
	}
	catch (XmlError er)
	{
		logStream (MESSAGE_WARNING) << "cannot load target constraint file " << getConstraintFile () << ":" << er << sendLogNoEndl;
	}

	MasterConstraints::setTargetConstraints (getTargetID (), ret);
	return ret; 
}

bool Target::checkConstraints (double JD)
{
	return getConstraints ()->satisfy (this, JD);
}

void Target::revalidateConstraints (int watchID)
{
	for (std::vector <int>::iterator iter = watchIDs.begin (); iter != watchIDs.end (); iter++)
	{
		if (*iter == watchID)
		{
			MasterConstraints::setTargetConstraints (getTargetID (), NULL);
			satisfiedFrom = satisfiedTo = NAN;
			return;
		}
	}
}

std::ostream & operator << (std::ostream &_os, Target &target)
{
	Rts2InfoValOStream _ivos = Rts2InfoValOStream (&_os);
	target.sendInfo (_ivos);
	return _os;
}

Rts2InfoValStream & operator << (Rts2InfoValStream &_os, Target &target)
{
	target.sendInfo (_os);
	return _os;
}
