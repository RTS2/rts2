/* 
 * List all labels know to the system
 * Copyright (C) 2011 Petr Kubanek, Insititute of Physics <kubanek@fzu.cz>
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

#include "rts2db/labellist.h"
#include "rts2db/sqlerror.h"

using namespace rts2db;

void LabelList::load ()
{
	EXEC SQL BEGIN DECLARE SECTION;
	int db_label_id;
	int db_label_t;
	VARCHAR db_label_text[500];
	EXEC SQL END DECLARE SECTION;

	EXEC SQL DECLARE cur_labels CURSOR FOR
		SELECT
			label_id, label_type, label_text
		FROM
			labels;

	EXEC SQL OPEN cur_labels;

	while (1)
	{
		EXEC SQL FETCH next FROM cur_labels INTO
			:db_label_id, :db_label_t, :db_label_text;
		if (sqlca.sqlcode)
		  	break;
		db_label_text.arr[db_label_text.len] = '\0';	
		push_back ( LabelEntry (db_label_id, db_label_t, db_label_text.arr) );
	}
	if (sqlca.sqlcode != ECPG_NOT_FOUND)
		throw SqlError ();
}
