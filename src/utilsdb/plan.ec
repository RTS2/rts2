/* 
 * Plan target class.
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

#include "../utils/rts2config.h"
#include "../utils/libnova_cpp.h"
#include "plan.h"
#include "sqlerror.h"

#include <ostream>
#include <sstream>
#include <iomanip>

using namespace rts2db;

Plan::Plan ()
{
	plan_id = -1;
	tar_id = -1;
	prop_id = -1;
	target = NULL;
	obs_id = -1;
	observation = NULL;
	plan_status = 0;
	plan_start = plan_end = rts2_nan ("f");
}

Plan::Plan (int in_plan_id)
{
	plan_id = in_plan_id;
	tar_id = -1;
	prop_id = -1;
	target = NULL;
	obs_id = -1;
	observation = NULL;
	plan_status = 0;
	plan_start = plan_end = rts2_nan ("f");
}

Plan::~Plan (void)
{
	delete target;
}

int Plan::load ()
{
	EXEC SQL BEGIN DECLARE SECTION;
		int db_plan_id = plan_id;
		int db_prop_id;
		int db_prop_id_ind;
		int db_tar_id;
		int db_obs_id;
		int db_obs_id_ind;
		double db_plan_start;
		double db_plan_end;
		int db_plan_end_ind;
		int db_plan_status;
	EXEC SQL END DECLARE SECTION;

	EXEC SQL SELECT
			prop_id,
			tar_id,
			obs_id,
			EXTRACT (EPOCH FROM plan_start),
			EXTRACT (EPOCH FROM plan_end),
			plan_status
		INTO
			:db_prop_id :db_prop_id_ind,
			:db_tar_id,
			:db_obs_id :db_obs_id_ind,
			:db_plan_start,
			:db_plan_end :db_plan_end_ind,
			:db_plan_status
		FROM
			plan
		WHERE
			plan_id = :db_plan_id;
	if (sqlca.sqlcode)
	{
		EXEC SQL ROLLBACK;
		return -1;
	}
	if (db_prop_id_ind)
		prop_id = -1;
	else
		prop_id = db_prop_id;
	tar_id = db_tar_id;
	if (db_obs_id_ind)
		db_obs_id = -1;
	else
		obs_id = db_obs_id;
	plan_start = (long) db_plan_start;
	if (db_plan_end_ind)
		plan_end = rts2_nan ("f");
	else
		plan_end = db_plan_end;
	plan_status = db_plan_status;
	EXEC SQL COMMIT;
	return 0;
}

int Plan::save ()
{
	EXEC SQL BEGIN DECLARE SECTION;
		int db_plan_id = plan_id;
		int db_tar_id = tar_id;
		int db_prop_id = prop_id;
		int db_prop_id_ind;
		int db_obs_id = obs_id;
		int db_obs_id_ind;
		long db_plan_start = plan_start;
		long db_plan_end = plan_end;
		int db_plan_end_ind = (isnan (plan_end) ? -1 : 0);
		int db_plan_status = plan_status;
	EXEC SQL END DECLARE SECTION;

	// don't save entries with same target id as master plan
	if (db_tar_id == TARGET_PLAN)
		return -1;

	if (db_plan_id == -1)
	{
		EXEC SQL
			SELECT
			nextval ('plan_id')
			INTO
				:db_plan_id;
		if (sqlca.sqlcode)
		{
			logStream(MESSAGE_ERROR) << "Error getting nextval" << sqlca.sqlcode << sqlca.sqlerrm.sqlerrmc << sendLog;
			return -1;
		}
		plan_id = db_plan_id;
	}

	if (db_prop_id == -1)
	{
		db_prop_id = 0;
		db_prop_id_ind = -1;
	}
	else
	{
		db_prop_id_ind = 0;
	}

	if (db_obs_id == -1)
	{
		db_obs_id = 0;
		db_obs_id_ind = -1;
	}
	else
	{
		db_obs_id_ind = 0;
	}

	EXEC SQL INSERT INTO plan (
			plan_id,
			tar_id,
			prop_id,
			obs_id,
			plan_start,
			plan_end,
			plan_status
			)
		VALUES (
			:db_plan_id,
			:db_tar_id,
			:db_prop_id :db_prop_id_ind,
			:db_obs_id :db_obs_id_ind,
			to_timestamp (:db_plan_start),
			to_timestamp (:db_plan_end :db_plan_end_ind),
			:db_plan_status
			);
	if (sqlca.sqlcode)
	{
		logStream(MESSAGE_ERROR) << "Error inserting plan " << sqlca.sqlcode << sqlca.sqlerrm.sqlerrmc
			<< " prop_id:" << db_prop_id << " ind:" << db_prop_id_ind << sendLog;
		// try update
		EXEC SQL UPDATE
				plan
			SET
				tar_id = :db_tar_id,
				prop_id = :db_prop_id :db_prop_id_ind,
				obs_id = :db_obs_id :db_obs_id_ind,
				plan_start = to_timestamp (:db_plan_start),
				plan_end = to_timestamp (:db_plan_end :db_plan_end_ind),
				plan_status = :db_plan_status
			WHERE
				plan_id = :db_plan_id;
		if (sqlca.sqlcode)
		{
			logStream(MESSAGE_ERROR) << "Error updating plan " << sqlca.sqlcode << sqlca.sqlerrm.sqlerrmc << sendLog;
			EXEC SQL ROLLBACK;
			return -1;
		}
	}
	EXEC SQL COMMIT;
	return 0;
}

int Plan::del ()
{
	EXEC SQL BEGIN DECLARE SECTION;
		int db_plan_id = plan_id;
	EXEC SQL END DECLARE SECTION;

	if (db_plan_id == -1)
		return 0;

	EXEC SQL DELETE FROM
			plan
		WHERE
			plan_id = :db_plan_id;
	if (sqlca.sqlcode)
	{
		EXEC SQL ROLLBACK;
		return -1;
	}
	EXEC SQL COMMIT;
	return 0;
}

moveType Plan::startSlew (struct ln_equ_posn *position, bool update_position)
{
	EXEC SQL BEGIN DECLARE SECTION;
		int db_plan_id = plan_id;
		int db_obs_id;
	EXEC SQL END DECLARE SECTION;
	moveType ret;
	ret = getTarget ()->startSlew (position, update_position);
	if (obs_id > 0)
		return ret;

	obs_id = getTarget()->getObsId ();
	db_obs_id = obs_id;

	EXEC SQL
		UPDATE
			plan
		SET
			obs_id = :db_obs_id,
			plan_status = plan_status | 1
		WHERE
			plan_id = :db_plan_id;
	if (sqlca.sqlcode)
	{
		logStream (MESSAGE_ERROR) << "Plan::startSlew " << sqlca.sqlerrm.sqlerrmc << " (" << sqlca.sqlcode << ")" << sendLog;
		EXEC SQL ROLLBACK;
	}
	else
	{
		EXEC SQL COMMIT;
	}
	return ret;
}

Target * Plan::getTarget ()
{
	if (target)
		return target;
	try
	{
		target = createTarget (tar_id, Rts2Config::instance ()->getObserver ());
	}
	catch (SqlError err)
	{
	  	logStream (MESSAGE_ERROR) << "error while retrieving target for plan: " << err << sendLog;
		return NULL;
	}
	return target;
}

Observation * Plan::getObservation ()
{
	int ret;
	if (observation)
		return observation;
	if (obs_id <= 0)
		return NULL;
	observation = new Observation (obs_id);
	ret = observation->load ();
	if (ret)
	{
		delete observation;
		observation = NULL;
	}
	return observation;
}

void Plan::print (std::ostream & _os)
{
	struct ln_hrz_posn hrz;
	const char *tar_name;
	struct ln_lnlat_posn *obs;
	int good;
	double JD;
	int ret;
	time_t t = plan_start;
	JD = ln_get_julian_from_timet (&t);
	ret = load ();
	if (ret)
	{
		tar_name = "(not loaded!)";
		good = 0;
	}
	else
	{
		tar_name = getTarget()->getTargetName();
		good = getTarget()->isGood (JD);
	}
	obs = Rts2Config::instance()->getObserver ();
	getTarget ()->getAltAz (&hrz, JD);
	LibnovaHrz lHrz (&hrz);
	_os << "  " << std::setw (8) << plan_id << SEP
		<< std::setw (8) << prop_id << SEP
		<< std::left << std::setw (20) << tar_name << SEP
		<< std::right << std::setw (8) << tar_id << SEP
		<< std::setw (8) << obs_id << SEP
		<< std::setw (9) << LibnovaDateDouble (plan_start) << SEP
		<< std::setw (9) << LibnovaDateDouble (plan_end) << SEP
		<< std::setw (8) << plan_status << SEP
		<< lHrz << SEP
		<< std::setw(1) << (good ? 'G' : 'B')
		<< std::endl;
}

void Plan::printInfoVal (Rts2InfoValStream & _os)
{
	if (_os.getStream ())
		*(_os.getStream ()) << this;
}

void Plan::read (std::istream & _is)
{
	char buf[201];
	char *comt;
	_is.getline (buf, 200);
	// ignore comment..
	comt = strchr (buf, '#');
	if (comt)
		*comt = '\0';

	std::istringstream buf_s (buf);
	LibnovaDate start_date;
	buf_s >> tar_id >> start_date;
	time_t t;
	start_date.getTimeT (& t);
	plan_start = t;
}
