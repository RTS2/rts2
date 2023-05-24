/*
 * User set.
 * Copyright (C) 2008 Petr Kubanek <petr@kubanek.net>
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

#include "app.h"
#include "rts2db/sqlerror.h"
#include "rts2db/userset.h"
#include "rts2db/devicedb.h"

#ifdef RTS2_HAVE_CRYPT
#include <crypt.h>
#endif

using namespace rts2db;

int UserSet::load ()
{
	if (checkDbConnection ())
		return -1;

	EXEC SQL BEGIN DECLARE SECTION;
	int db_id;
	VARCHAR db_login[25];
	VARCHAR db_email[200];
	VARCHAR db_allowed_devices[2000];
	int db_allowed_devices_ind;
	EXEC SQL END DECLARE SECTION;

	EXEC SQL BEGIN TRANSACTION;

	EXEC SQL DECLARE user_cur CURSOR FOR
		SELECT
			usr_id,
			usr_login,
			usr_email,
			allowed_devices
		FROM
			users
			ORDER BY
			usr_login asc;

	EXEC SQL OPEN user_cur;

	while (1)
	{
		EXEC SQL FETCH next FROM user_cur INTO
				:db_id,
				:db_login,
				:db_email,
				:db_allowed_devices :db_allowed_devices_ind;
		if (sqlca.sqlcode)
			break;
		if (db_allowed_devices_ind == 0)
		{
			db_allowed_devices.arr[db_allowed_devices.len] = '\0';
			push_back (User (db_id, std::string (db_login.arr), std::string (db_email.arr), db_allowed_devices.arr));
		}
		else
		{
			push_back (User (db_id, std::string (db_login.arr), std::string (db_email.arr), NULL));
		}
	}
	if (sqlca.sqlcode != ECPG_NOT_FOUND)
	{
		logStream (MESSAGE_ERROR) << "UserSet::load cannot load user set " << sqlca.sqlerrm.sqlerrmc << sendLog;
		EXEC SQL ROLLBACK;
		return -1;
	}

	EXEC SQL COMMIT;

	// load types
	for (UserSet::iterator iter = begin (); iter != end (); iter++)
	{
		int ret = (*iter).loadTypes ();
		if (ret)
			return ret;
	}

	return 0;
}

UserSet::UserSet ()
{
	load ();
}

UserSet::~UserSet (void)
{
}

int createUser (std::string login, std::string password, std::string email, std::string allowed_devices)
{
	if (checkDbConnection ())
		return -1;

	EXEC SQL BEGIN DECLARE SECTION;
	VARCHAR db_login[25];
	VARCHAR db_password[101];
	VARCHAR db_email[200];
	VARCHAR db_allowed_devices[2000];
	EXEC SQL END DECLARE SECTION;

	if (login.length () > 25)
	{
		logStream (MESSAGE_ERROR) << "login is too long" << sendLog;
		return -1;
	}

#ifdef RTS2_HAVE_CRYPT
	char salt[101];
	strcpy (salt, "$6$");
	random_salt (salt + 3, 8);
	strcpy (salt + 11, "$");
	strncpy (db_password.arr, crypt (password.c_str (), salt), 100);
	db_password.len = strlen (db_password.arr);
#else
	if (password.length () > 100)
	{
		logStream (MESSAGE_ERROR) << "password is too long" << sendLog;
		return -1;
	}

	strncpy (db_password.arr, password.c_str (), 100);
	db_password.len = password.length ();
#endif

	if (email.length () > 25)
	{
		logStream (MESSAGE_ERROR) << "email is too long" << sendLog;
		return -1;
	}
	if (allowed_devices.length () > 1999)
	{
		logStream (MESSAGE_ERROR) << "allowed devices is too long" << sendLog;
		return -1;
	}

	strncpy (db_login.arr, login.c_str (), 25);
	db_login.len = login.length ();

	strncpy (db_email.arr, email.c_str (), 200);
	db_email.len = email.length ();

	strncpy (db_allowed_devices.arr, allowed_devices.c_str(), 2000);
	db_allowed_devices.len = allowed_devices.length ();

	EXEC SQL INSERT INTO users
	(
		usr_id,
		usr_login,
		usr_passwd,
		usr_email,
		allowed_devices
	)
	VALUES (
		nextval ('user_id'),
		:db_login,
		:db_password,
		:db_email,
		:db_allowed_devices
	);

	if (sqlca.sqlcode)
	{
		logStream (MESSAGE_ERROR) << "cannot insert new user " << sqlca.sqlerrm.sqlerrmc << sendLog;
		EXEC SQL ROLLBACK;
		return -1;
	}

	EXEC SQL COMMIT;
	return 0;
}

int removeUser (std::string login)
{
	if (checkDbConnection ())
		return -1;

	EXEC SQL BEGIN DECLARE SECTION;
	VARCHAR db_login[25];
	EXEC SQL END DECLARE SECTION;

	if (login.length () > 25)
	{
		logStream (MESSAGE_ERROR) << "login is too long" << sendLog;
		return -1;
	}

	strncpy (db_login.arr, login.c_str (), 25);
	db_login.len = login.length ();

	EXEC SQL DELETE FROM users WHERE usr_login = :db_login;

	if (sqlca.sqlcode)
	{
		throw rts2db::SqlError ("cannot delete user");
	}
	EXEC SQL COMMIT;
	return 0;
}
