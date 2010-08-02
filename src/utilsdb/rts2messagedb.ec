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


#include "rts2messagedb.h"
#include "../utils/rts2block.h"

#include <iostream>

EXEC SQL include sqlca;

Rts2MessageDB::Rts2MessageDB (const struct timeval &in_messageTime, std::string in_messageOName, messageType_t in_messageType, std::string in_messageString): Rts2Message (in_messageTime, in_messageOName, in_messageType, in_messageString)
{
}

Rts2MessageDB::~Rts2MessageDB (void)
{
}

void Rts2MessageDB::insertDB ()
{
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

	strncpy (d_message_string.arr, messageString.c_str(), 200);
	d_message_string.len = strlen (d_message_string.arr);
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
