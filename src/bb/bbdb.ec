/**
 * RTS2 BB Database API
 * Copyright (C) 2012 Petr Kubanek <petr@kubanek.net>
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

#include "bbdb.h"
#include "rts2db/sqlerror.h"
#include "rts2json/jsonvalue.h"

#include <math.h>

using namespace rts2bb;

Observatory::Observatory (int id)
{
	observatory_id = id;
	position.lng = NAN;
	position.lat = NAN;
	altitude = NAN;
}

void Observatory::load ()
{
	EXEC SQL BEGIN DECLARE SECTION;
	int db_observatory_id = observatory_id;
	double db_longitude;
	double db_latitude;
	double db_altitude;
	VARCHAR db_apiurl[100];
	EXEC SQL END DECLARE SECTION;

	EXEC SQL SELECT
		longitude,
		latitude,
		altitude,
		apiurl
	INTO
		:db_longitude,
		:db_latitude,
		:db_altitude,
		:db_apiurl
	FROM
		observatories
	WHERE
		observatory_id = :db_observatory_id;
	
	if (sqlca.sqlcode)
		throw rts2db::SqlError ();

	position.lng = db_longitude;
	position.lat = db_latitude;
	altitude = db_altitude;

	db_apiurl.arr[db_apiurl.len] = '\0';

	url = std::string (db_apiurl.arr);

	EXEC SQL COMMIT;
}

void Observatory::auth (SoupAuth *_auth)
{
	soup_auth_authenticate (_auth, "petr", "test");
}

void Observatories::load ()
{
	EXEC SQL BEGIN DECLARE SECTION;
	int db_observatory_id;
	EXEC SQL END DECLARE SECTION;

	EXEC SQL DECLARE cur_observatories CURSOR FOR
		SELECT
			observatory_id
		FROM
			observatories
		ORDER BY
			observatory_id;
	EXEC SQL OPEN cur_observatories;
	while (true)
	{
		EXEC SQL FETCH next FROM cur_observatories INTO
			:db_observatory_id;
		if (sqlca.sqlcode)
		{
			if (sqlca.sqlcode == ECPG_NOT_FOUND)
				break;
			throw rts2db::SqlError ();
		}
		push_back (Observatory (db_observatory_id));
	}

	for (Observatories::iterator iter = begin (); iter != end (); iter++)
		iter->load ();
	EXEC SQL ROLLBACK;
}

void ObservatorySchedule::load ()
{
	EXEC SQL BEGIN DECLARE SECTION;
	int db_schedule_id = schedule_id;
	int db_observatory_id = observatory_id;
	int db_state;

	double db_created;
	double db_last_update;
	double db_sched_from;
	double db_sched_to;

	int db_created_ind;
	int db_last_update_ind;
	int db_sched_from_ind;
	int db_sched_to_ind;
	EXEC SQL END DECLARE SECTION;

	EXEC SQL SELECT 
		state,
		EXTRACT (EPOCH FROM created),
		EXTRACT (EPOCH FROM last_update),
		EXTRACT (EPOCH FROM sched_from),
		EXTRACT (EPOCH FROM sched_to)
	INTO
		:db_state,
		:db_created :db_created_ind,
		:db_last_update :db_last_update_ind,
		:db_sched_from :db_sched_from_ind,
		:db_sched_to :db_sched_to_ind
	FROM
		observatory_schedules
	WHERE
		schedule_id = :db_schedule_id
		AND observatory_id = :db_observatory_id;
	
	if (sqlca.sqlcode)
		throw rts2db::SqlError ();

	state = db_state;
	from = db_nan_double (db_sched_from, db_sched_from_ind);
	to = db_nan_double (db_sched_to, db_sched_to_ind);
}

void ObservatorySchedule::updateState (int _state, double _from, double _to)
{
	EXEC SQL BEGIN DECLARE SECTION;
	int db_schedule_id = schedule_id;
	int db_observatory_id = observatory_id;
	int db_state = _state;
	int db_old_state;

	double db_sched_from = _from;
	int db_sched_from_ind = db_nan_indicator (_from);
	double db_sched_to = _to;
	int db_sched_to_ind = db_nan_indicator (_to);
	EXEC SQL END DECLARE SECTION;

	EXEC SQL SELECT
		state
	INTO
		:db_old_state
	FROM
		observatory_schedules
	WHERE
		schedule_id = :db_schedule_id
		AND observatory_id = :db_observatory_id;

	if (sqlca.sqlcode)
	{
		EXEC SQL INSERT INTO
			observatory_schedules
		VALUES (:db_schedule_id, :db_observatory_id, :db_state, now (), now (), to_timestamp (:db_sched_from :db_sched_from_ind), to_timestamp (:db_sched_to :db_sched_to_ind));
	}
	else
	{
		EXEC SQL UPDATE
			observatory_schedules
		SET
			state = :db_state,
			last_update = now (),
			sched_from = to_timestamp (:db_sched_from :db_sched_from_ind),
			sched_to = to_timestamp (:db_sched_to :db_sched_to_ind)
		WHERE
			schedule_id = :db_schedule_id
			AND observatory_id = :db_observatory_id;
	}
	if (sqlca.sqlcode)
		throw rts2db::SqlError ();

	EXEC SQL COMMIT;
	state = _state;
	from = db_nan_double (db_sched_from, db_sched_from_ind);
	to = db_nan_double (db_sched_to, db_sched_to_ind);
}

void ObservatorySchedule::toJSON (std::ostream &os)
{
	os << "\"state\":" << state
		<< ",\"created\":" << rts2json::JsonDouble (created)
		<< ",\"last_update\":" << rts2json::JsonDouble (last_update)
		<< ",\"from\":" << rts2json::JsonDouble (from)
		<< ",\"to\":" << rts2json::JsonDouble (to);
}

void BBSchedules::load ()
{
	EXEC SQL BEGIN DECLARE SECTION;
	int db_schedule_id = schedule_id;
	int db_tar_id;

	int db_observatory_id;
	int db_state;
	double db_created;
	double db_last_update;
	double db_sched_from;
	double db_sched_to;

	int db_created_ind;
	int db_last_update_ind;
	int db_sched_from_ind;
	int db_sched_to_ind;
	EXEC SQL END DECLARE SECTION;

	EXEC SQL SELECT tar_id INTO :db_tar_id FROM bb_schedules WHERE schedule_id = :db_schedule_id;

	if (sqlca.sqlcode)
		throw rts2db::SqlError ();
	
	tar_id = db_tar_id;

	// load all schedules
	EXEC SQL DECLARE cur_schedules CURSOR FOR
		SELECT
			observatory_id,
			state,
			EXTRACT (EPOCH FROM created),
			EXTRACT (EPOCH FROM last_update),
			EXTRACT (EPOCH FROM sched_from),
			EXTRACT (EPOCH FROM sched_to)
		FROM
			observatory_schedules
		WHERE
			schedule_id = :db_schedule_id;

	EXEC SQL OPEN cur_schedules;

	while (true)
	{
		EXEC SQL FETCH next FROM
			cur_schedules
		INTO
			:db_observatory_id,
			:db_state,
			:db_created :db_created_ind,
			:db_last_update :db_last_update_ind,
			:db_sched_from :db_sched_from_ind,
			:db_sched_to :db_sched_to_ind
		;
		if (sqlca.sqlcode)
		{
			if (sqlca.sqlcode == ECPG_NOT_FOUND)
				break;
			throw rts2db::SqlError ();
		}

		push_back (ObservatorySchedule (db_schedule_id, db_observatory_id, db_state, db_nan_double (db_created, db_created_ind), db_nan_double (db_last_update, db_last_update_ind), db_nan_double (db_sched_from, db_sched_from_ind), db_nan_double (db_sched_to, db_sched_to_ind)));
	}

	EXEC SQL ROLLBACK;
}

void BBSchedules::toJSON (std::ostream &os)
{
	os << "[";
	
	for (BBSchedules::iterator iter = begin (); iter != end (); iter++)
	{
		if (iter != begin ())
			os << ",";
		os << "[" << iter->getObservatoryId () << "," << iter->getState () << "," << iter->getFrom () << "," << iter->getTo () << "," << iter->getCreated () << "]";
	}

	os << "]";
}

/***
 * Register new target mapping into BB database.
 *
 * @param observatory_id  ID of observatory requesting the change
 * @param tar_id          ID of target in central database
 * @param obs_tar_id      ID of target in observatory database
 */
void rts2bb::createMapping (int observatory_id, int tar_id, int obs_tar_id)
{
	EXEC SQL BEGIN DECLARE SECTION;
	int db_observatory_id = observatory_id;
	int db_tar_id = tar_id;
	int db_obs_tar_id = obs_tar_id;
	EXEC SQL END DECLARE SECTION;
	
	EXEC SQL INSERT INTO targets_observatories
		(observatory_id, tar_id, obs_tar_id) 
	VALUES
		(:db_observatory_id, :db_tar_id, :db_obs_tar_id);

	if (sqlca.sqlcode)
		throw rts2db::SqlError ();
	
	EXEC SQL COMMIT;
}

void rts2bb::reportObservation (int observatory_id, int obs_id, int obs_tar_id, double obs_ra, double obs_dec, double obs_slew, double obs_start, double obs_end, double onsky, int good_images, int bad_images)
{
	EXEC SQL BEGIN DECLARE SECTION;
	int db_observatory_id = observatory_id;
	int db_obs_id = obs_id;
	int db_tar_id;
	double db_obs_ra = obs_ra;
	double db_obs_dec = obs_dec;
	double db_obs_slew = obs_slew;
	double db_obs_start = obs_start;
	double db_obs_end = obs_end;
	double db_onsky = onsky;
	int db_good_images = good_images;
	int db_bad_images = bad_images;

	int db_obs_ra_ind = db_nan_indicator (obs_ra);
	int db_obs_dec_ind = db_nan_indicator (obs_dec);
	int db_obs_slew_ind = db_nan_indicator (obs_slew);
	int db_obs_start_ind = db_nan_indicator (obs_start);
	int db_obs_end_ind = db_nan_indicator (obs_end);
	int db_onsky_ind = db_nan_indicator (onsky);

	int db_obs_count;
	EXEC SQL END DECLARE SECTION;

	db_tar_id = findMapping (observatory_id, obs_tar_id);

	EXEC SQL SELECT count(*) INTO :db_obs_count FROM observatory_observations WHERE observatory_id = :db_observatory_id AND obs_id = :db_obs_id;

	if (db_obs_count == 1)
	{
		EXEC SQL UPDATE observatory_observations SET
			tar_id = :db_tar_id,
			obs_ra = :db_obs_ra :db_obs_ra_ind,
			obs_dec = :db_obs_dec :db_obs_dec_ind,
			obs_slew = to_timestamp (:db_obs_slew :db_obs_slew_ind),
			obs_start = to_timestamp (:db_obs_start :db_obs_start_ind),
			obs_end = to_timestamp (:db_obs_end :db_obs_end_ind),
			onsky = :db_onsky :db_onsky_ind,
			good_images = :db_good_images,
			bad_images = :db_bad_images
		WHERE
			observatory_id = :db_observatory_id AND obs_id = :db_obs_id;

		if (sqlca.sqlcode)
			throw rts2db::SqlError ();
	}
	else
	{
		EXEC SQL INSERT INTO observatory_observations
			(observatory_id, obs_id, tar_id, obs_ra, obs_dec, obs_slew, obs_start, obs_end, onsky, good_images, bad_images)
		VALUES
			(:db_observatory_id, :db_obs_id, :db_tar_id, :db_obs_ra :db_obs_ra_ind, :db_obs_dec :db_obs_dec_ind,
			to_timestamp (:db_obs_slew :db_obs_slew_ind),
			to_timestamp (:db_obs_start :db_obs_start_ind),
			to_timestamp (:db_obs_end :db_obs_end_ind),
			:db_onsky :db_onsky_ind, :db_good_images, :db_bad_images);

		if (sqlca.sqlcode)
			throw rts2db::SqlError ();
	}
	EXEC SQL COMMIT;
}

int rts2bb::findMapping (int observatory_id, int obs_tar_id)
{
	EXEC SQL BEGIN DECLARE SECTION;
	int db_observatory_id = observatory_id;
	int db_obs_tar_id = obs_tar_id;
	int db_tar_id;
	EXEC SQL END DECLARE SECTION;

	EXEC SQL SELECT tar_id INTO :db_tar_id FROM targets_observatories WHERE observatory_id = :db_observatory_id AND obs_tar_id = :db_obs_tar_id;

	if (sqlca.sqlcode)
		throw rts2db::SqlError ();

	EXEC SQL ROLLBACK;
	return db_tar_id;
}

int rts2bb::findObservatoryMapping (int observatory_id, int tar_id)
{
	EXEC SQL BEGIN DECLARE SECTION;
	int db_observatory_id = observatory_id;
	int db_obs_tar_id;
	int db_tar_id = tar_id;
	EXEC SQL END DECLARE SECTION;

	EXEC SQL SELECT obs_tar_id INTO :db_obs_tar_id FROM targets_observatories WHERE observatory_id = :db_observatory_id AND tar_id = :db_tar_id;

	if (sqlca.sqlcode)
		throw rts2db::SqlError ();

	EXEC SQL ROLLBACK;
	return db_obs_tar_id;
}

int rts2bb::createSchedule (int target_id)
{
	EXEC SQL BEGIN DECLARE SECTION;
	int db_schedule_id;
	int db_target_id = target_id;
	EXEC SQL END DECLARE SECTION;

	EXEC SQL SELECT nextval('bb_schedule_id') INTO :db_schedule_id;

	EXEC SQL INSERT INTO
		bb_schedules
	VALUES (
		:db_schedule_id,
		:db_target_id
	);

	if (sqlca.sqlcode)
		throw rts2db::SqlError ();

	EXEC SQL COMMIT;
	return db_schedule_id;
}
