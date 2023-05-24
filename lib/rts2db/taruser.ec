/*
 * User access.
 * Copyright (C) 2005-2008 Petr Kubanek <petr@kubanek.net>
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

#include <algorithm>

#include "rts2db/taruser.h"
#include "rts2db/devicedb.h"
#include "block.h"

using namespace rts2db;

UserEvent::UserEvent (const UserEvent &in_user)
{
	usr_email = in_user.usr_email;
	event_mask = in_user.event_mask;
}

UserEvent::UserEvent (const char *in_usr_email, int in_event_mask)
{
	usr_email = std::string (in_usr_email);
	event_mask = in_event_mask;
}

UserEvent::~UserEvent (void)
{
}

/**********************************************************************
 *  TarUser class
 **********************************************************************/

TarUser::TarUser (int in_target, char in_type_id)
{
	tar_id = in_target;
	type_id = in_type_id;
}

TarUser::~TarUser (void)
{
}

int TarUser::load ()
{
	if (checkDbConnection ())
		return -1;

	EXEC SQL BEGIN DECLARE SECTION;
	int db_tar_id = tar_id;
	char db_type_id = type_id;

	int db_usr_id;
	// cannot use USER_EMAIL_LEN, as it does not work with some ecpg versions
	VARCHAR db_usr_email[200];
	int db_event_mask;
	EXEC SQL END DECLARE SECTION;

	// get TarUsers from targets_users

	EXEC SQL DECLARE cur_targets_users CURSOR FOR
	SELECT
		users.usr_id,
		usr_email,
		event_mask
	FROM
		users,
		targets_users
	WHERE
		tar_id = :db_tar_id
	AND targets_users.usr_id = users.usr_id;

	EXEC SQL OPEN cur_targets_users;
	while (1)
	{
		EXEC SQL FETCH next FROM cur_targets_users INTO
				:db_usr_id,
				:db_usr_email,
				:db_event_mask;
		if (sqlca.sqlcode)
			break;
		users.push_back (UserEvent (db_usr_email.arr, db_event_mask));
	}
	if (sqlca.sqlcode && sqlca.sqlcode != ECPG_NOT_FOUND)
	{
		logStream (MESSAGE_ERROR) << "TarUsers::load cannot get users " << sqlca.sqlerrm.sqlerrmc << sendLog;
		EXEC SQL CLOSE cur_targets_users;
		EXEC SQL ROLLBACK;
		return -1;
	}
	EXEC SQL CLOSE cur_targets_users;
	EXEC SQL COMMIT;

	// get TarUsers from type_users

	EXEC SQL DECLARE cur_type_users CURSOR FOR
	SELECT
		users.usr_id,
		usr_email,
		event_mask
	FROM
		users,
		type_users
	WHERE
		type_id = :db_type_id
	AND type_users.usr_id = users.usr_id;

	EXEC SQL OPEN cur_type_users;
	while (1)
	{
		EXEC SQL FETCH next FROM cur_type_users INTO
				:db_usr_id,
				:db_usr_email,
				:db_event_mask;
		if (sqlca.sqlcode)
			break;
		UserEvent newUser = UserEvent (db_usr_email.arr, db_event_mask);
		if (std::find (users.begin (), users.end (), newUser) != users.end ())
		{
			logStream (MESSAGE_ERROR) << "TarUsers::load user already exists (tar_id " << tar_id << ")" << sendLog;
		}
		else
		{
			users.push_back (UserEvent (newUser));
		}
	}
	if (sqlca.sqlcode && sqlca.sqlcode != ECPG_NOT_FOUND)
	{
		logStream (MESSAGE_ERROR) << "TarUsers::load cannot get users '" << sqlca.sqlerrm.sqlerrmc << "'" << sendLog;
		EXEC SQL CLOSE cur_type_users;
		EXEC SQL ROLLBACK;
		return -1;
	}
	EXEC SQL CLOSE cur_type_users;
	EXEC SQL COMMIT;
	return 0;
}

std::string TarUser::getUsers (int in_event_mask, int &count)
{
	std::vector <UserEvent>::iterator user_iter;
	int ret;
	std::string email_list = "";
	if (users.empty ())
	{
		ret = load ();
		if (ret)
			return std::string ("");
	}
	count = 0;
	for (user_iter = users.begin (); user_iter != users.end (); user_iter++)
	{
		if ((*user_iter).haveMask (in_event_mask))
		{
			if (count != 0)
				email_list += ", ";
			email_list += (*user_iter).getUserEmail ();
			count++;
		}
	}
	return email_list;
}
