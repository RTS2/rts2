/* 
 * Filter look-up table and creator.
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

#include "rts2fits/dbfilters.h"
#include "rts2db/sqlerror.h"
#include "app.h"

using namespace rts2image;

DBFilters *DBFilters::pInstance = NULL;

DBFilters::DBFilters ()
{
}

DBFilters * DBFilters::instance ()
{
	if (!pInstance)
		pInstance = new DBFilters ();
	return pInstance;
}

void DBFilters::load ()
{
	EXEC SQL BEGIN DECLARE SECTION;
	int db_filter_id;
	VARCHAR db_standard_name[50];
	EXEC SQL END DECLARE SECTION;

	EXEC SQL DECLARE all_filters CURSOR FOR
	SELECT
		filter_id,
		standard_name
	FROM
		filters;
	
	EXEC SQL OPEN all_filters;

	while (1)
	{
		EXEC SQL FETCH next FROM all_filters INTO
			:db_filter_id,
			:db_standard_name;
		if (sqlca.sqlcode)
		  	break;
		(*this)[db_filter_id] = std::string (db_standard_name.arr);
	}

	if (sqlca.sqlcode != ECPG_NOT_FOUND)
		throw rts2db::SqlError ();
	EXEC SQL CLOSE all_filters;
	EXEC SQL ROLLBACK;
}

int DBFilters::getIndex (const char *filter)
{
	EXEC SQL BEGIN DECLARE SECTION;
	int db_filter_id;
	VARCHAR db_standard_name[50];
	EXEC SQL END DECLARE SECTION;

	DBFilters::iterator iter;

	for (iter = begin (); iter != end (); iter++)
	{
		if (iter->second == filter)
			return iter->first;
	}

	EXEC SQL SELECT
		nextval('filter_id')
	INTO
		:db_filter_id;
	
	if (sqlca.sqlcode)
	{
		logStream (MESSAGE_ERROR) << "cannot get id for new filter" << sendLog;
		return -1;
	}

	db_standard_name.len = strlen (filter) > 50 ? 50 : strlen (filter);
	memcpy (db_standard_name.arr, filter, db_standard_name.len);
	
	EXEC SQL INSERT INTO
		filters (filter_id, standard_name)
	VALUES
		(:db_filter_id, :db_standard_name) ;
	if (sqlca.sqlcode)
	{
		// try to re-load filter table..
		load ();
		for (iter = begin (); iter != end (); iter++)
		{
			if (iter->second == filter)
				return iter->first;
		}
		logStream (MESSAGE_ERROR) << "cannot find index for filter " << filter << ", and cannot create entry for it in filters database" << sendLog;
		return -1;
	}
	return db_filter_id;
}
