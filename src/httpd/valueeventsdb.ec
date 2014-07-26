/* 
 * Value changes triggering infrastructure. 
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

#include "rts2db/recvals.h"
#include "rts2db/sqlerror.h"

EXEC SQL include sqlca;

using namespace rts2xmlrpc;

int ValueChangeRecord::getRecvalId (const char *suffix, int recval_type)
{
	EXEC SQL BEGIN DECLARE SECTION;
	int db_recval_id;
	VARCHAR db_device_name[25];
	VARCHAR db_value_name[26];
	int db_recval_type = recval_type;
	EXEC SQL END DECLARE SECTION;

	std::map <const char *, int>::iterator iter = dbValueIds.find (suffix);

	if (iter != dbValueIds.end ())
		return iter->second;

	db_device_name.len = deviceName.length ();
	if (db_device_name.len > 25)
		db_device_name.len = 25;
	strncpy (db_device_name.arr, deviceName.c_str (), db_device_name.len);

	db_value_name.len = valueName.length ();
	if (db_value_name.len > 25)
		db_value_name.len = 25;
	strncpy (db_value_name.arr, valueName.c_str (), db_value_name.len);
	db_value_name.arr[db_value_name.len] = '\0';
	if (suffix != NULL)
	{
		strncat (db_value_name.arr, suffix, 25 - db_value_name.len);
		db_value_name.len += strlen (suffix);
		if (db_value_name.len > 25)
			db_value_name.len = 25;
	}
	EXEC SQL SELECT recval_id INTO :db_recval_id
		FROM recvals WHERE device_name = :db_device_name AND value_name = :db_value_name;
	if (sqlca.sqlcode)
	{
		if (sqlca.sqlcode == ECPG_NOT_FOUND)
		{
			// insert new record
			EXEC SQL SELECT nextval ('recval_ids') INTO :db_recval_id;
			EXEC SQL INSERT INTO recvals VALUES (:db_recval_id, :db_device_name, :db_value_name, :db_recval_type);
			if (sqlca.sqlcode)
				throw rts2db::SqlError ();
		}
		else
		{
			throw rts2db::SqlError ();
		}
	}

	dbValueIds[suffix] = db_recval_id;

	return db_recval_id;
}

void ValueChangeRecord::recordValueInteger (int recval_id, int val, double validTime)
{
	EXEC SQL BEGIN DECLARE SECTION;
	int db_recval_id = recval_id;
	int  db_value = val;
	double db_rectime = validTime;
	EXEC SQL END DECLARE SECTION;

	EXEC SQL INSERT INTO records_integer
	VALUES
	(
		:db_recval_id,
		to_timestamp (:db_rectime),
		:db_value
	);

	if (sqlca.sqlcode)
		throw rts2db::SqlError ();
}

void ValueChangeRecord::recordValueDouble (int recval_id, double val, double validTime)
{
	EXEC SQL BEGIN DECLARE SECTION;
	int db_recval_id = recval_id;
	double  db_value = val;
	double db_rectime = validTime;
	EXEC SQL END DECLARE SECTION;

	EXEC SQL INSERT INTO records_double
	VALUES
	(
		:db_recval_id,
		to_timestamp (:db_rectime),
		:db_value
	);

	if (sqlca.sqlcode)
		throw rts2db::SqlError ();
}

void ValueChangeRecord::recordValueBoolean (int recval_id, bool val, double validTime)
{
	EXEC SQL BEGIN DECLARE SECTION;
	int db_recval_id = recval_id;
	bool db_value = val;
	double db_rectime = validTime;
	EXEC SQL END DECLARE SECTION;

	EXEC SQL INSERT INTO records_boolean
	VALUES
	(
		:db_recval_id,
		to_timestamp (:db_rectime),
		:db_value
	);

	if (sqlca.sqlcode)
		throw rts2db::SqlError ();
}

void ValueChangeRecord::run (rts2core::Value *val, double validTime)
{

	std::ostringstream _os;

	switch (val->getValueBaseType ())
	{
		case RTS2_VALUE_INTEGER:
			recordValueInteger (getRecvalId (NULL, RTS2_VALUE_INTEGER | val->getValueDisplayType ()), val->getValueInteger (), validTime);
			break;
		case RTS2_VALUE_DOUBLE:
		case RTS2_VALUE_FLOAT:
			recordValueDouble (getRecvalId (NULL, RTS2_VALUE_DOUBLE | val->getValueDisplayType ()), val->getValueDouble (), validTime);
			break;
		case RTS2_VALUE_RADEC:
			recordValueDouble (getRecvalId ("RA", RTS2_VALUE_DOUBLE | RTS2_DT_RA), ((rts2core::ValueRaDec *) val)->getRa (), validTime);
			recordValueDouble (getRecvalId ("DEC", RTS2_VALUE_DOUBLE | RTS2_DT_DEC), ((rts2core::ValueRaDec *) val)->getDec (), validTime);
			break;
		case RTS2_VALUE_ALTAZ:
			recordValueDouble (getRecvalId ("ALT", RTS2_VALUE_DOUBLE | RTS2_DT_DEGREES), ((rts2core::ValueAltAz *) val)->getAlt (), validTime);
			recordValueDouble (getRecvalId ("AZ", RTS2_VALUE_DOUBLE | RTS2_DT_DEGREES), ((rts2core::ValueAltAz *) val)->getAz (), validTime);
			break;
		case RTS2_VALUE_BOOL:
			recordValueBoolean (getRecvalId (NULL, RTS2_VALUE_BOOL), ((rts2core::ValueBool *) val)->getValueBool (), validTime);
			break;
		default:
			_os << "Cannot record value " << valueName.c_str ();
			throw rts2core::Error (_os.str ());
	}
	EXEC SQL COMMIT;
	if (sqlca.sqlcode)
		throw rts2db::SqlError ();
}
