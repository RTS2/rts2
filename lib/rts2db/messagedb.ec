/*
 * Class for database messages manipulation.
 * Copyright (C) 2007-2010 Petr Kubanek <petr@kubanek.net>
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


#include "rts2db/messagedb.h"
#include "rts2db/sqlerror.h"
#include "rts2db/devicedb.h"
#include "block.h"

#include <iostream>

EXEC SQL include sqlca;

using namespace rts2db;

MessageDB::MessageDB (const struct timeval &in_messageTime, std::string in_messageOName, messageType_t in_messageType, std::string in_messageString): rts2core::Message (in_messageTime, in_messageOName, in_messageType, in_messageString)
{
}

MessageDB::MessageDB (double in_messageTime, const char *in_messageOName, messageType_t in_messageType, const char *in_messageString): rts2core::Message (in_messageTime, in_messageOName, in_messageType, in_messageString)
{
}

MessageDB::~MessageDB (void)
{
}

void MessageDB::insertDB ()
{
	if (checkDbConnection ())
		return;

	EXEC SQL BEGIN DECLARE SECTION;
	double d_message_time = messageTime.tv_sec + (double) messageTime.tv_usec / USEC_SEC;
	varchar d_message_oname[8];
	int d_message_type = messageType;
	varchar d_message_string[200];
	EXEC SQL END DECLARE SECTION;

	strncpy (d_message_oname.arr, messageOName.c_str(), 8);
	d_message_oname.len = strlen (d_message_oname.arr);
	if (d_message_oname.len > 8)
		d_message_oname.len = 8;

	std::string msgStr = getMessageString ();

	strncpy (d_message_string.arr, msgStr.c_str(), 200);
	d_message_string.len = msgStr.length ();
	if (d_message_string.len > 200)
		d_message_string.len = 200;

	EXEC SQL INSERT INTO message
	(
		message_time,
		message_oname,
		message_type,
		message_string
	)
	VALUES
	(
		to_timestamp (:d_message_time),
		:d_message_oname,
		:d_message_type,
		:d_message_string
	);
	if (sqlca.sqlcode)
	{
		std::cerr << "Error writing to DB: " << sqlca.sqlerrm.sqlerrmc << " " << sqlca.sqlcode << std::endl;
		EXEC SQL ROLLBACK;
		return;
	}
	EXEC SQL COMMIT;
}

void MessageSet::load (double from, double to, int type_mask)
{
	if (checkDbConnection ())
		throw SqlError ();

	EXEC SQL BEGIN DECLARE SECTION;
	double d_from = from;
	double d_to = to;
	int d_type_mask = type_mask;

	double d_message_time;
	varchar d_message_oname[9];
	int d_message_type;
	varchar d_message_string[200];
	EXEC SQL END DECLARE SECTION;

	EXEC SQL DECLARE cur_message CURSOR FOR
	SELECT
		EXTRACT (EPOCH FROM message_time),
		message_oname,
		message_type,
		message_string
	FROM
		message
	WHERE
		message_time BETWEEN to_timestamp (:d_from) AND to_timestamp (:d_to)
		AND (message_type & :d_type_mask) <> 0
	ORDER BY
		message_time ASC;

	EXEC SQL OPEN cur_message;
	while (true)
	{
		EXEC SQL FETCH next FROM cur_message INTO
			:d_message_time,
			:d_message_oname,
			:d_message_type,
			:d_message_string;
		if (sqlca.sqlcode)
			break;
		d_message_oname.arr[d_message_oname.len] = '\0';
		push_back (MessageDB (d_message_time, d_message_oname.arr, d_message_type, d_message_string.arr));
	}
	if (sqlca.sqlcode != ECPG_NOT_FOUND)
	{
		EXEC SQL CLOSE cur_message;
		EXEC SQL ROLLBACK;
		throw SqlError ();
	}
	EXEC SQL CLOSE cur_message;
	EXEC SQL COMMIT;
}

double MessageSet::getNextCtime ()
{
	if (timeLogIter != end ())
		return timeLogIter->getMessageTime ();
	return NAN;
}

void MessageSet::printUntil (double time, std::ostream &os)
{
	while (timeLogIter != end () && timeLogIter->getMessageTime () <= time)
	{
		os << (*timeLogIter) << std::endl;
		timeLogIter++;
	}
}
