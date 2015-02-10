/* 
 * State changes databse infrastructure.
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

#include "httpd.h"

#include "rts2db/sqlerror.h"

EXEC SQL include sqlca;

using namespace rts2xmlrpc;

void StateChangeRecord::run (HttpD *_master, rts2core::Connection *_conn, double validTime)
{
	EXEC SQL BEGIN DECLARE SECTION;
	int db_recval_id = dbValueId;
	VARCHAR db_device_name[25];
	VARCHAR db_value_name[25];
	int db_value;
	double db_rectime = validTime;
	EXEC SQL END DECLARE SECTION;

	if (db_recval_id < 0)
	{
		db_device_name.len = strlen (_conn->getName ());
		if (db_device_name.len > 25)
			db_device_name.len = 25;
		strncpy (db_device_name.arr, _conn->getName (), db_device_name.len);

		db_value_name.len = 5;
		strncpy (db_value_name.arr, "state", db_value_name.len);
		EXEC SQL SELECT recval_id INTO :db_recval_id
			FROM recvals WHERE device_name = :db_device_name AND value_name = :db_value_name;
		if (sqlca.sqlcode)
		{
			if (sqlca.sqlcode == ECPG_NOT_FOUND)
			{
				// insert new record
				EXEC SQL SELECT nextval ('recval_ids') INTO :db_recval_id;
				EXEC SQL INSERT INTO recvals VALUES (:db_recval_id, :db_device_name, :db_value_name, 0);
				if (sqlca.sqlcode)
					throw rts2db::SqlError ();
			}
			else
			{
				throw rts2db::SqlError ();
			}
		}
	}
	
	db_value = _conn->getState () & getChangeMask ();
	EXEC SQL INSERT INTO records_state
	VALUES
		(
			:db_recval_id,
			to_timestamp (:db_rectime),
			:db_value
		);
	EXEC SQL COMMIT;
	if (sqlca.sqlcode)
	{
		throw rts2db::SqlError ();
	}
}
