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

#include "recvals.h"
#include "sqlerror.h"

using namespace rts2db;

void RecvalsSet::load ()
{
	EXEC SQL BEGIN DECLARE SECTION;
	int d_recval_id;
	VARCHAR d_device_name[26];
	VARCHAR d_value_name[26];
	double d_from;
	double d_to;
	long d_num_rec;
	EXEC SQL END DECLARE SECTION;

	EXEC SQL DECLARE recval_cur CURSOR FOR
	SELECT
		recval_id,
		device_name,
		value_name,
		EXTRACT (EPOCH FROM time_from),
		EXTRACT (EPOCH FROM time_to),
		nrec
	FROM
		recvals_statistics;

	EXEC SQL OPEN recval_cur;

	while (true)
	{
		EXEC SQL FETCH next FROM recval_cur INTO
			:d_recval_id,
			:d_device_name,
			:d_value_name,
			:d_from,
			:d_to
			:d_num_rec;
		if (sqlca.sqlcode)
			break;
		d_device_name.arr[d_device_name.len] = '\0';
		d_value_name.arr[d_value_name.len] = '\0';
		push_back (Recval (d_recval_id, d_device_name.arr, d_value_name.arr, d_from, d_to, d_num_rec));
	}

	if (sqlca.sqlcode != ECPG_NOT_FOUND)
	{
		throw SqlError();
	}
	EXEC SQL CLOSE recval_cur;
	EXEC SQL ROLLBACK;
}
