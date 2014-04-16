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

	rep_n = -1;
	rep_separation = NAN;

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
	int db_rep_n;
	float db_rep_separation;
	int db_rep_separation_ind;
	EXEC SQL END DECLARE SECTION;

	if (queue_id < 0)
		return;

	EXEC SQL SELECT
		queue_id,
		tar_id,
		plan_id,
		EXTRACT (EPOCH FROM time_start),
		EXTRACT (EPOCH FROM time_end),
		queue_order,
		repeat_n,
		repeat_separation
	INTO
		:db_queue_id,
		:db_tar_id,
		:db_plan_id :db_plan_id_ind,
		:db_time_start :db_time_start_ind,
		:db_time_end :db_time_end_ind,
		:db_queue_order,
		:db_rep_n,
		:db_rep_separation :db_rep_separation_ind
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

	rep_n = db_rep_n;
	rep_separation = db_nan_float (db_rep_separation, db_rep_separation_ind);

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
	int db_rep_n = rep_n;
	float db_rep_separation = rep_separation;
	int db_rep_separation_ind = db_nan_indicator (rep_separation);
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
		queue_order,
		repeat_n,
		repeat_separation)
	VALUES (
		:db_qid,
		:db_queue_id,
		:db_tar_id,
		:db_plan_id :db_plan_id_ind,
		to_timestamp (:db_time_start :db_time_start_ind),
		to_timestamp (:db_time_end :db_time_end_ind),
		:db_queue_order,
		:db_rep_n,
		:db_rep_separation :db_rep_separation_ind
	);
	if (sqlca.sqlcode)
		throw SqlError ();

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
	int db_rep_n = rep_n;
	float db_rep_separation = rep_separation;
	int db_rep_separation_ind = db_nan_indicator (rep_separation);
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
		queue_order = :db_queue_order,
		repeat_n = :db_rep_n,
		repeat_separation = :db_rep_separation :db_rep_separation_ind
	WHERE
		qid = :db_qid;
	if (sqlca.sqlcode)
		throw SqlError ();
	
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
		throw SqlError ();

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
		throw SqlError ();
	}
	EXEC SQL CLOSE cur_queues;
	EXEC SQL ROLLBACK;
	return ret;
}

Queue::Queue (int _queue_id)
{
	queue_id = _queue_id;
}

void Queue::load ()
{
	EXEC SQL BEGIN DECLARE SECTION;
	int db_queue_id = queue_id;

	int db_queue_type;
	bool db_skip_below_horizon;
	bool db_test_constraints;
	bool db_remove_after_execution;
	bool db_block_until_visible;
	bool db_check_target_length;
	bool db_queue_enabled;
	float db_queue_window;
	EXEC SQL END DECLARE SECTION;

	EXEC SQL SELECT
		queue_type,
		skip_below_horizon,
		test_constraints,
		remove_after_execution,
		block_until_visible,
		check_target_length,
		queue_enabled,
		queue_window
	INTO
		:db_queue_type,
		:db_skip_below_horizon,
		:db_test_constraints,
		:db_remove_after_execution,
		:db_block_until_visible,
		:db_check_target_length,
		:db_queue_enabled,
		:db_queue_window
	FROM
		queues
	WHERE
		queue_id = :db_queue_id;
	
	if (sqlca.sqlcode)
		throw SqlError ();

	queue_type = db_queue_type;
	skip_below_horizon = db_skip_below_horizon;
	test_constraints = db_test_constraints;
	remove_after_execution = db_remove_after_execution;
	block_until_visible = db_block_until_visible;
	check_target_length = db_check_target_length;
	queue_enabled = db_queue_enabled;
	queue_window = db_queue_window;
	
	EXEC SQL ROLLBACK;
}

void Queue::save ()
{
	EXEC SQL BEGIN DECLARE SECTION;
	int db_queue_id = queue_id;
	EXEC SQL END DECLARE SECTION;

	EXEC SQL SELECT queue_id FROM queues WHERE queue_id = :db_queue_id;

	if (sqlca.sqlcode)
	{
		if (sqlca.sqlcode != ECPG_NOT_FOUND)
			throw SqlError ();
		create ();
	}
	else
	{
		update ();
	}
}

void Queue::create ()
{
	EXEC SQL BEGIN DECLARE SECTION;
	int db_queue_id = queue_id;

	int db_queue_type = queue_type;
	bool db_skip_below_horizon = skip_below_horizon;
	bool db_test_constraints = test_constraints;
	bool db_remove_after_execution = remove_after_execution;
	bool db_block_until_visible = block_until_visible;
	bool db_check_target_length = check_target_length;
	bool db_queue_enabled = queue_enabled;
	float db_queue_window = queue_window;
	EXEC SQL END DECLARE SECTION;

	EXEC SQL INSERT INTO queues(
		queue_id,
		queue_type,
		skip_below_horizon,
		test_constraints,
		remove_after_execution,
		block_until_visible,
		check_target_length,
		queue_enabled,
		queue_window)
	VALUES (
		:db_queue_id,
		:db_queue_type,
		:db_skip_below_horizon,
		:db_test_constraints,
		:db_remove_after_execution,
		:db_block_until_visible,
		:db_check_target_length,
		:db_queue_enabled,
		:db_queue_window
	);
	
	if (sqlca.sqlcode)
		throw SqlError ();
	
	EXEC SQL COMMIT;
}

void Queue::update ()
{
	EXEC SQL BEGIN DECLARE SECTION;
	int db_queue_id = queue_id;

	int db_queue_type = queue_type;
	bool db_skip_below_horizon = skip_below_horizon;
	bool db_test_constraints = test_constraints;
	bool db_remove_after_execution = remove_after_execution;
	bool db_block_until_visible = block_until_visible;
	bool db_check_target_length = check_target_length;
	bool db_queue_enabled = queue_enabled;
	float db_queue_window = queue_window;
	EXEC SQL END DECLARE SECTION;

	EXEC SQL UPDATE
		queues
	SET
		queue_type = :db_queue_type,
		skip_below_horizon = :db_skip_below_horizon,
		test_constraints = :db_test_constraints,
		remove_after_execution = :db_remove_after_execution,
		block_until_visible = :db_block_until_visible,
		check_target_length = :db_check_target_length,
		queue_enabled = :db_queue_enabled,
		queue_window = :db_queue_window
	WHERE
		queue_id = :db_queue_id;
	
	if (sqlca.sqlcode)
		throw SqlError ();
	
	EXEC SQL COMMIT;
}
