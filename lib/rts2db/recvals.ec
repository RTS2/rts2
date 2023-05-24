/*
 * Utility classes for record values manipulations.
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

#include "utilsfunc.h"

#include "rts2db/recvals.h"
#include "rts2db/sqlerror.h"
#include "rts2db/devicedb.h"

using namespace rts2db;

void RecvalsSet::load ()
{
	if (checkDbConnection ())
		throw SqlError ();

	EXEC SQL BEGIN DECLARE SECTION;
	int d_recval_id;
	VARCHAR d_device_name[26];
	VARCHAR d_value_name[26];
	int d_value_type;
	double d_from;
	double d_to;
	int d_from_null;
	int d_to_null;
	EXEC SQL END DECLARE SECTION;

	EXEC SQL DECLARE recval_state_cur CURSOR FOR
	SELECT
		recval_id,
		device_name,
		value_name,
		EXTRACT (EPOCH FROM time_from),
		EXTRACT (EPOCH FROM time_to)
	FROM
		recvals_state_statistics;

	EXEC SQL OPEN recval_state_cur;

	while (true)
	{
		EXEC SQL FETCH next FROM recval_state_cur INTO
			:d_recval_id,
			:d_device_name,
			:d_value_name,
			:d_from :d_from_null,
			:d_to :d_to_null;
		if (sqlca.sqlcode)
			break;
		d_device_name.arr[d_device_name.len] = '\0';
		d_value_name.arr[d_value_name.len] = '\0';
		if (d_from_null)
			d_from = NAN;
		if (d_to_null)
			d_to = NAN;
		insert (std::pair <int, Recval> (d_recval_id, Recval (d_recval_id, d_device_name.arr, d_value_name.arr, RECVAL_STATE, d_from, d_to)));
	}

	if (sqlca.sqlcode != ECPG_NOT_FOUND)
	{
		throw SqlError();
	}
	EXEC SQL CLOSE recval_state_cur;


	EXEC SQL DECLARE recval_double_cur CURSOR FOR
	SELECT
		recval_id,
		device_name,
		value_name,
		value_type,
		EXTRACT (EPOCH FROM time_from),
		EXTRACT (EPOCH FROM time_to)
	FROM
		recvals_double_statistics;

	EXEC SQL OPEN recval_double_cur;

	while (true)
	{
		EXEC SQL FETCH next FROM recval_double_cur INTO
			:d_recval_id,
			:d_device_name,
			:d_value_name,
			:d_value_type,
			:d_from :d_from_null,
			:d_to :d_to_null;
		if (sqlca.sqlcode)
			break;
		d_device_name.arr[d_device_name.len] = '\0';
		d_value_name.arr[d_value_name.len] = '\0';
		if (d_from_null)
			d_from = NAN;
		if (d_to_null)
			d_to = NAN;
		insert (std::pair <int, Recval> (d_recval_id, Recval (d_recval_id, d_device_name.arr, d_value_name.arr, d_value_type, d_from, d_to)));
	}

	if (sqlca.sqlcode != ECPG_NOT_FOUND)
	{
		throw SqlError();
	}
	EXEC SQL CLOSE recval_double_cur;

	EXEC SQL DECLARE recval_boolean_cur CURSOR FOR
	SELECT
		recval_id,
		device_name,
		value_name,
		value_type,
		EXTRACT (EPOCH FROM time_from),
		EXTRACT (EPOCH FROM time_to)
	FROM
		recvals_boolean_statistics;

	EXEC SQL OPEN recval_boolean_cur;

	while (true)
	{
		EXEC SQL FETCH next FROM recval_boolean_cur INTO
			:d_recval_id,
			:d_device_name,
			:d_value_name,
			:d_value_type,
			:d_from,
			:d_to;
		if (sqlca.sqlcode)
			break;
		d_device_name.arr[d_device_name.len] = '\0';
		d_value_name.arr[d_value_name.len] = '\0';
		insert (std::pair <int, Recval> (d_recval_id, Recval (d_recval_id, d_device_name.arr, d_value_name.arr, d_value_type, d_from, d_to)));
	}

	if (sqlca.sqlcode != ECPG_NOT_FOUND)
	{
		throw SqlError();
	}
	EXEC SQL CLOSE recval_boolean_cur;
	EXEC SQL ROLLBACK;
}

Recval * RecvalsSet::searchByName (const char *_device_name, const char *_value_name)
{
	std::string _dn (_device_name);
	std::string _vn (_value_name);
	for (std::map <int, Recval>::iterator iter = begin (); iter != end (); iter++)
	{
		if (iter->second.getDevice () == _dn && iter->second.getValueName () == _vn)
			return &(iter->second);
	}
	return NULL;
}
