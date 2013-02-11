/* 
 * Schedule class.
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

#include "rts2db/sqlerror.h"
#include "rts2db/queues.h"
#include "utilsfunc.h"

using namespace rts2db;

QueueEntry::QueueEntry (unsigned int _qid, int _queue_id)
{
	qid = _qid;
	queue_id = _queue_id;
	tar_id = 0;
	plan_id = 0;
	t_start = NAN;
	t_end = NAN;

	queue_order = -1;
}

void QueueEntry::load ()
{
	EXEC SQL BEGIN DECLARE SECTION;
	unsigned int db_qid = qid;
	int db_queue_id;
	unsigned int db_tar_id;
	int db_plan_id;
	int db_plan_id_ind;
	double db_time_start;
	double db_time_end;
	int db_time_start_ind;
	int db_time_end_ind;
	int db_queue_order;
	EXEC SQL END DECLARE SECTION;

	if (queue_id < 0)
		return;

	EXEC SQL SELECT
		queue_id,
		tar_id,
		plan_id,
		EXTRACT (EPOCH FROM time_start),
		EXTRACT (EPOCH FROM time_end),
		queue_order
	INTO
		:db_queue_id,
		:db_tar_id,
		:db_plan_id :db_plan_id_ind,
		:db_time_start :db_time_start_ind,
		:db_time_end :db_time_end_ind,
		:db_queue_order
	FROM
		queues_targets
	WHERE
		qid = :db_qid;
	
	if (sqlca.sqlcode)
		throw SqlError();

	queue_id = db_queue_id;	
	tar_id = db_tar_id;
	if (db_plan_id_ind == 0)
		plan_id = db_plan_id;

	t_start = db_nan_double (db_time_start, db_time_start_ind);
	t_end = db_nan_double (db_time_end, db_time_end_ind);

	queue_order = db_queue_order;
}

int static_qid = 0;

int QueueEntry::nextQid ()
{
	EXEC SQL BEGIN DECLARE SECTION;
	unsigned int db_qid;
	EXEC SQL END DECLARE SECTION;

	if (queue_id < 0)
		return ++static_qid;

	EXEC SQL SELECT
		nextval ('qid')
	INTO
		:db_qid;
	if (sqlca.sqlcode)
		throw SqlError ();

	return db_qid;
}


void QueueEntry::create ()
{
	EXEC SQL BEGIN DECLARE SECTION;
	unsigned int db_qid = qid;
	int db_queue_id = queue_id;
	unsigned int db_tar_id = tar_id;
	int db_plan_id = plan_id;
	int db_plan_id_ind = plan_id > 0 ? 0 : -1;
	double db_time_start = t_start;
	double db_time_end = t_end;
	int db_time_start_ind = db_nan_indicator (t_start);
	int db_time_end_ind = db_nan_indicator (t_end);
	int db_queue_order = queue_order;
	EXEC SQL END DECLARE SECTION;

	if (queue_id < 0)
		return;

	EXEC SQL INSERT INTO queues_targets (
		qid,
		queue_id,
		tar_id,
		plan_id,
		time_start,
		time_end,
		queue_order)
	VALUES (
		:db_qid,
		:db_queue_id,
		:db_tar_id,
		:db_plan_id :db_plan_id_ind,
		to_timestamp (:db_time_start :db_time_start_ind),
		to_timestamp (:db_time_end :db_time_end_ind),
		:db_queue_order
	);
	if (sqlca.sqlcode)
	{
		EXEC SQL ROLLBACK;
		throw SqlError ();
	}
	EXEC SQL COMMIT;
}

void QueueEntry::update ()
{
	EXEC SQL BEGIN DECLARE SECTION;
	unsigned int db_qid = qid;
	int db_queue_id = queue_id;
	unsigned int db_tar_id = tar_id;
	int db_plan_id = plan_id;
	int db_plan_id_ind = plan_id > 0 ? 0 : -1;
	double db_time_start = t_start;
	double db_time_end = t_end;
	int db_time_start_ind = db_nan_indicator (t_start);
	int db_time_end_ind = db_nan_indicator (t_end);
	int db_queue_order = queue_order;
	EXEC SQL END DECLARE SECTION;

	if (queue_id < 0)
		return;

	EXEC SQL UPDATE
		queues_targets
	SET
		queue_id = :db_queue_id,
		tar_id = :db_tar_id,
		plan_id = :db_plan_id :db_plan_id_ind,
		time_start = to_timestamp (:db_time_start :db_time_start_ind),
		time_end = to_timestamp (:db_time_end :db_time_end_ind),
		queue_order = :db_queue_order
	WHERE
		qid = :db_qid;
	if (sqlca.sqlcode)
	{
		EXEC SQL ROLLBACK;
		throw SqlError ();
	}
	EXEC SQL COMMIT;
}

void QueueEntry::remove ()
{
	EXEC SQL BEGIN DECLARE SECTION;
	unsigned int db_qid = qid;
	EXEC SQL END DECLARE SECTION;

	if (queue_id < 0)
		return;

	EXEC SQL DELETE FROM queues_targets WHERE qid = :db_qid;

	if (sqlca.sqlcode)
	{
		EXEC SQL ROLLBACK;
		throw SqlError ();
	}
	EXEC SQL COMMIT;
}

std::list <unsigned int> rts2db::queueQids (int queue_id)
{
	EXEC SQL BEGIN DECLARE SECTION;
	int db_queue_id = queue_id;
	unsigned int db_qid;
	EXEC SQL END DECLARE SECTION;

	std::list <unsigned int> ret;

	EXEC SQL DECLARE cur_queues CURSOR FOR
	SELECT
		qid
	FROM
		queues_targets
	WHERE
		queue_id = :db_queue_id
	ORDER BY
		qid ASC;
	EXEC SQL OPEN cur_queues;

	while (1)
	{
		EXEC SQL FETCH next FROM
			cur_queues
		INTO
			:db_qid;
		if (sqlca.sqlcode)
			break;
		ret.push_back (db_qid);
	}
	if (sqlca.sqlcode != ECPG_NOT_FOUND)
	{
		EXEC SQL CLOSE cur_queues;
		EXEC SQL ROLLBACK;
		throw SqlError ();
	}
	EXEC SQL CLOSE cur_queues;
	EXEC SQL ROLLBACK;
	return ret;
}
