/*
 * Utility classes for record values.
 * Copyright (C) 2009 Petr Kubanek <petr@kubanek.net>
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

#include "rts2db/records.h"
#include "rts2db/recvals.h"
#include "rts2db/sqlerror.h"
#include "rts2db/devicedb.h"

using namespace rts2db;

int RecordsSet::getValueType ()
{
	if (checkDbConnection ())
		throw SqlError ();

	EXEC SQL BEGIN DECLARE SECTION;
	int d_recval_id = recval_id;
	int d_value_type;
	EXEC SQL END DECLARE SECTION;

	if (value_type != -1)
		return value_type;

	EXEC SQL SELECT value_type INTO :d_value_type FROM recvals WHERE recval_id = :d_recval_id;
	if (sqlca.sqlcode)
		throw SqlError ();
	value_type = d_value_type;
	return value_type;
}

void RecordsSet::loadState (double t_from, double t_to)
{
	if (checkDbConnection ())
		throw SqlError ();

	EXEC SQL BEGIN DECLARE SECTION;
	int d_recval_id = recval_id;
	double d_rectime;
	double d_value;
	double d_t_from = t_from;
	double d_t_to = t_to;
	EXEC SQL END DECLARE SECTION;

	EXEC SQL DECLARE records_state_cur CURSOR FOR
	SELECT
		EXTRACT (EPOCH FROM rectime),
		value
	FROM
		records_state
	WHERE
		  recval_id = :d_recval_id
		AND rectime BETWEEN to_timestamp (:d_t_from) AND to_timestamp (:d_t_to)
	ORDER BY
		rectime;

	EXEC SQL OPEN records_state_cur;

	while (true)
	{
		EXEC SQL FETCH next FROM records_state_cur INTO
			:d_rectime,
			:d_value;
		if (sqlca.sqlcode)
			break;
		push_back (Record (d_rectime, d_value));
	}

	if (sqlca.sqlcode != ECPG_NOT_FOUND)
	{
		throw SqlError();
	}
	EXEC SQL CLOSE records_state_cur;
	EXEC SQL ROLLBACK;
}

void RecordsSet::loadDouble (double t_from, double t_to)
{
	if (checkDbConnection ())
		throw SqlError ();

	EXEC SQL BEGIN DECLARE SECTION;
	int d_recval_id = recval_id;
	double d_rectime;
	double d_value;
	double d_t_from = t_from;
	double d_t_to = t_to;
	EXEC SQL END DECLARE SECTION;

	EXEC SQL DECLARE records_double_cur CURSOR FOR
	SELECT
		EXTRACT (EPOCH FROM rectime),
		value
	FROM
		records_double
	WHERE
		  recval_id = :d_recval_id
		AND rectime BETWEEN to_timestamp (:d_t_from) AND to_timestamp (:d_t_to)
	ORDER BY
		rectime;

	EXEC SQL OPEN records_double_cur;

	min = INFINITY;
	max = -INFINITY;

	while (true)
	{
		EXEC SQL FETCH next FROM records_double_cur INTO
			:d_rectime,
			:d_value;
		if (sqlca.sqlcode)
			break;
		if (d_value < min)
			min = d_value;
		if (d_value > max)
		  	max = d_value;
		push_back (Record (d_rectime, d_value));
	}

	if (sqlca.sqlcode != ECPG_NOT_FOUND)
	{
		throw SqlError();
	}
	EXEC SQL CLOSE records_double_cur;
	EXEC SQL ROLLBACK;
}

void RecordsSet::loadBoolean (double t_from, double t_to)
{
	if (checkDbConnection ())
		throw SqlError ();

	EXEC SQL BEGIN DECLARE SECTION;
	int d_recval_id = recval_id;
	double d_rectime;
	bool d_value;
	double d_t_from = t_from;
	double d_t_to = t_to;
	EXEC SQL END DECLARE SECTION;

	EXEC SQL DECLARE records_boolean_cur CURSOR FOR
	SELECT
		EXTRACT (EPOCH FROM rectime),
		value
	FROM
		records_boolean
	WHERE
		  recval_id = :d_recval_id
		AND rectime BETWEEN to_timestamp (:d_t_from) AND to_timestamp (:d_t_to)
	ORDER BY
		rectime;

	EXEC SQL OPEN records_boolean_cur;

	min = 1;
	max = 0;

	while (true)
	{
		EXEC SQL FETCH next FROM records_boolean_cur INTO
			:d_rectime,
			:d_value;
		if (sqlca.sqlcode)
			break;
		if (d_value < min)
			min = d_value;
		if (d_value > max)
		  	max = d_value;
		push_back (Record (d_rectime, d_value));
	}

	if (sqlca.sqlcode != ECPG_NOT_FOUND)
	{
		throw SqlError();
	}
	EXEC SQL CLOSE records_boolean_cur;
	EXEC SQL ROLLBACK;
}

void RecordsSet::load (double t_from, double t_to)
{
	switch (getValueBaseType ())
	{
		case RECVAL_STATE:
			loadState (t_from, t_to);
			break;
		case RTS2_VALUE_DOUBLE:
			loadDouble (t_from, t_to);
			break;
		case RTS2_VALUE_BOOL:
			loadBoolean (t_from, t_to);
			break;
		default:
			throw rts2core::Error ("unknown value type");
	}
}
