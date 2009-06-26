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

#include "records.h"
#include "sqlerror.h"

using namespace rts2db;

void RecordsSet::load ()
{
	EXEC SQL BEGIN DECLARE SECTION;
	int d_recval_id = recval_id;
	double d_rectime;
	double d_value;
	EXEC SQL END DECLARE SECTION;

	EXEC SQL DECLARE records_cur CURSOR FOR
	SELECT
		rectime,
		value
	FROM
		records
	WHERE
		recval_id = :d_recval_id;

	EXEC SQL OPEN records_cur;

	while (true)
	{
		EXEC SQL FETCH next FROM records_cur INTO
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
	EXEC SQL CLOSE records_cur;
	EXEC SQL ROLLBACK;
}
