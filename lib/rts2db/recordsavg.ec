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

#include "rts2db/recordsavg.h"
#include "rts2db/sqlerror.h"
#include "rts2db/devicedb.h"

using namespace rts2db;

void RecordAvgSet::load (double t_from, double t_to)
{
	if (checkDbConnection ())
		throw SqlError ();

	EXEC SQL BEGIN DECLARE SECTION;
	int d_recval_id = recval_id;
	double d_t_from = t_from;
	double d_t_to = t_to;

	double d_rectime;
	double d_avg;
	double d_min;
	double d_max;
	int d_nrec;
	EXEC SQL END DECLARE SECTION;

	EXEC SQL DECLARE records_double_avg_cur CURSOR FOR
	SELECT
		EXTRACT (EPOCH FROM hour),
		avg_value,
		min_value,
		max_value,
		nrec
	FROM
		mv_records_double_hour
	WHERE
		  recval_id = :d_recval_id
		AND hour BETWEEN to_timestamp (:d_t_from) AND to_timestamp (:d_t_to)
	ORDER BY
		hour;

	EXEC SQL OPEN records_double_avg_cur;

	while (true)
	{
		EXEC SQL FETCH next FROM records_double_avg_cur INTO
			:d_rectime,
			:d_avg,
			:d_min,
			:d_max,
			:d_nrec;
		if (sqlca.sqlcode)
			break;
		push_back (RecordAvg (d_rectime + 1800, d_avg, d_min, d_max, d_nrec));
	}

	if (sqlca.sqlcode != ECPG_NOT_FOUND)
	{
		throw SqlError();
	}
	EXEC SQL CLOSE records_double_avg_cur;
	EXEC SQL ROLLBACK;
}
