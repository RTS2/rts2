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

#include "configuration.h"
#include "libnova_cpp.h"

#include "rts2db/plan.h"
#include "rts2db/sqlerror.h"

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
	plan_status = 0;
	plan_start = plan_end = NAN;
	bb_observatory_id = -1;
	bb_schedule_id = -1;
}

Plan::Plan (int in_plan_id)
{
	plan_id = in_plan_id;
	tar_id = -1;
	prop_id = -1;
	target = NULL;
	plan_status = 0;
	plan_start = plan_end = NAN;
	bb_observatory_id = -1;
	bb_schedule_id = -1;
}

Plan::Plan (const Plan &cp)
{
	plan_id = cp.plan_id;
	prop_id = cp.prop_id;
	tar_id = cp.tar_id;
	plan_start = cp.plan_start;
	plan_end = cp.plan_end;
	plan_status = cp.plan_status;
	bb_observatory_id = cp.bb_observatory_id;
	bb_schedule_id = cp.bb_schedule_id;
	target = cp.target;

	target = NULL;
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
	double db_plan_start;
	double db_plan_end;
	int db_plan_end_ind;
	int db_plan_status;
	int db_bb_observatory_id;
	int db_bb_observatory_id_ind;
	int db_bb_schedule_id;
	int db_bb_schedule_id_ind;
	EXEC SQL END DECLARE SECTION;

	EXEC SQL SELECT
		prop_id,
		tar_id,
		EXTRACT (EPOCH FROM plan_start),
		EXTRACT (EPOCH FROM plan_end),
		plan_status,
		bb_observatory_id,
		bb_schedule_id
	INTO
		:db_prop_id :db_prop_id_ind,
		:db_tar_id,
		:db_plan_start,
		:db_plan_end :db_plan_end_ind,
		:db_plan_status,
		:db_bb_observatory_id :db_bb_observatory_id_ind,
		:db_bb_schedule_id :db_bb_schedule_id_ind
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
	plan_start = (long) db_plan_start;
	if (db_plan_end_ind)
		plan_end = NAN;
	else
		plan_end = db_plan_end;
	plan_status = db_plan_status;

	if (db_bb_observatory_id_ind)
	{
		bb_observatory_id = -1;
	}
	else
	{
		bb_observatory_id = db_bb_observatory_id;
	}

	if (db_bb_schedule_id_ind)
	{
		bb_schedule_id = -1;
	}
	else
	{
		bb_schedule_id = db_bb_schedule_id;
	}

	EXEC SQL COMMIT;
	return 0;
}

int Plan::loadBBSchedule (int _bb_observatory_id, int _bb_schedule_id)
{
	EXEC SQL BEGIN DECLARE SECTION;
	int db_plan_id;
	int db_prop_id;
	int db_prop_id_ind;
	int db_tar_id;
	double db_plan_start;
	double db_plan_end;
	int db_plan_end_ind;
	int db_plan_status;
	int db_bb_observatory_id = _bb_observatory_id;
	int db_bb_schedule_id = _bb_schedule_id;
	EXEC SQL END DECLARE SECTION;

	EXEC SQL SELECT
		plan_id,
		prop_id,
		tar_id,
		EXTRACT (EPOCH FROM plan_start),
		EXTRACT (EPOCH FROM plan_end),
		plan_status
	INTO
		:db_plan_id,
		:db_prop_id :db_prop_id_ind,
		:db_tar_id,
		:db_plan_start,
		:db_plan_end :db_plan_end_ind,
		:db_plan_status
	FROM
		plan
	WHERE
		bb_observatory_id = :db_bb_observatory_id AND
		bb_schedule_id = :db_bb_schedule_id;

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
	plan_start = (long) db_plan_start;
	if (db_plan_end_ind)
		plan_end = NAN;
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
	double db_plan_start = plan_start;
	double db_plan_end = plan_end;
	int db_plan_end_ind = (isnan (plan_end) ? -1 : 0);
	int db_plan_status = plan_status;
	int db_bb_observatory_id = bb_observatory_id;
	int db_bb_observatory_id_ind;
	int db_bb_schedule_id = bb_schedule_id;
	int db_bb_schedule_id_ind;
	EXEC SQL END DECLARE SECTION;

	// don't save entries with same target id as master plan
	if (db_tar_id == TARGET_PLAN)
		return -1;

	if (db_plan_id == -1)
	{
		EXEC SQL SELECT
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

	if (bb_observatory_id < 0)
		db_bb_observatory_id_ind = -1;
	else
		db_bb_observatory_id_ind = 0;

	if (bb_schedule_id < 0)
		db_bb_schedule_id_ind = -1;
	else
		db_bb_schedule_id_ind = 0;

	EXEC SQL INSERT INTO plan (
		plan_id,
		tar_id,
		prop_id,
		plan_start,
		plan_end,
		plan_status,
		bb_observatory_id,
		bb_schedule_id
	)
	VALUES (
		:db_plan_id,
		:db_tar_id,
		:db_prop_id :db_prop_id_ind,
		to_timestamp (:db_plan_start),
		to_timestamp (:db_plan_end :db_plan_end_ind),
		:db_plan_status,
		:db_bb_observatory_id :db_bb_observatory_id_ind,
		:db_bb_schedule_id :db_bb_schedule_id_ind
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
			plan_start = to_timestamp (:db_plan_start),
			plan_end = to_timestamp (:db_plan_end :db_plan_end_ind),
			plan_status = :db_plan_status,
			bb_observatory_id = :db_bb_observatory_id :db_bb_observatory_id_ind,
			bb_schedule_id = :db_bb_schedule_id :db_bb_schedule_id_ind
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
	return getTarget ()->startSlew (position, update_position);
}

Target * Plan::getTarget ()
{
	if (target)
		return target;
	target = createTarget (tar_id, rts2core::Configuration::instance ()->getObserver ());
	return target;
}

void Plan::print (std::ostream & _os)
{
	struct ln_hrz_posn hrz;
	const char *tar_name;
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
		good = getTarget()->isAboveHorizon (JD);
	}
	getTarget ()->getAltAz (&hrz, JD);
	LibnovaHrz lHrz (&hrz);
	_os << "  " << std::setw (8) << plan_id << SEP
		<< std::setw (8) << prop_id << SEP
		<< std::left << std::setw (20) << tar_name << SEP
		<< std::right << std::setw (8) << tar_id << SEP
		<< std::setw (9) << Timestamp (plan_start) << SEP
		<< std::setw (9) << Timestamp (plan_end) << SEP
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
	_is >> tar_id;
	if (_is.fail ())
		throw rts2core::Error ("cannot parse target ID");
	LibnovaDate date;
	_is >> date;
	if (_is.fail ())
		throw rts2core::Error ("cannot parse start date");
	plan_start = date.getDateDouble ();
	try
	{
		_is >> date;
		plan_end = date.getDateDouble ();
	}
	catch (rts2core::Error &er)
	{
		plan_end = NAN;
	}
	Target *tar = getTarget ();
	if (tar == NULL)
	{
		std::ostringstream os;
		os << "cannot load target with ID " << getTargetId ();
		throw rts2core::Error (os.str ());
	}
}
