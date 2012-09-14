/* 
 * User management.
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

#include "rts2db/user.h"
#include "app.h"

#include <unistd.h>
#include <crypt.h>

using namespace rts2db;

TypeUser::TypeUser (char in_type, int in_eventMask)
{
	type = in_type;
	eventMask = in_eventMask;
}

TypeUser::~TypeUser (void)
{

}

int TypeUser::updateFlags (int id, int newFlags)
{
	EXEC SQL BEGIN DECLARE SECTION;
	int db_id = id;
	char db_type = type;
	int db_eventMask = newFlags;
	EXEC SQL END DECLARE SECTION;

	EXEC SQL UPDATE
		type_users
	SET
		event_mask = :db_eventMask
	WHERE
		usr_id = :db_id AND type_id = :db_type;
	
	if (sqlca.sqlcode)
	{
		logStream (MESSAGE_ERROR) << "error while updating flags for user " << id
			<< " and type " << type 
			<< " :" << sqlca.sqlerrm.sqlerrmc << sendLog;
		EXEC SQL ROLLBACK;
		return -1;
	}

	eventMask = newFlags;

	EXEC SQL COMMIT;
	return 0;
}

int TypeUserSet::load (int usr_id)
{
	EXEC SQL BEGIN DECLARE SECTION;
	int db_usr_id = usr_id;
	char db_type;
	int db_eventMask;
	EXEC SQL END DECLARE SECTION;

	EXEC SQL BEGIN TRANSACTION;

	EXEC SQL DECLARE typeuser_cur CURSOR FOR
		SELECT
			type_id,
			event_mask
		FROM
			type_users
		WHERE
			usr_id = :db_usr_id;

	EXEC SQL OPEN typeuser_cur;

	while (1)
	{
		EXEC SQL FETCH next FROM typeuser_cur INTO
				:db_type,
				:db_eventMask;
		if (sqlca.sqlcode)
			break;
		push_back (TypeUser (db_type, db_eventMask));
	}
	if (sqlca.sqlcode != ECPG_NOT_FOUND)
	{
		logStream (MESSAGE_ERROR) << "TypeUserSet::load cannot load user set " << sqlca.sqlerrm.sqlerrmc << sendLog;
		EXEC SQL ROLLBACK;
		return -1;
	}
	EXEC SQL ROLLBACK;
	return 0;
}

TypeUserSet::TypeUserSet (int usr_id)
{
	load (usr_id);
}

TypeUserSet::~TypeUserSet (void)
{

}

TypeUser * TypeUserSet::getFlags (char type)
{
	for (TypeUserSet::iterator iter = begin (); iter != end (); iter++)
	{
		if ((*iter).isType (type))
			return &(*iter);
	}
	return NULL;
}

int TypeUserSet::addNewTypeFlags (int id, char type, int flags)
{
	EXEC SQL BEGIN DECLARE SECTION;
        char db_type_id = type;
	int db_usr_id = id;
	int db_event_mask = flags;
	EXEC SQL END DECLARE SECTION;
	
	EXEC SQL INSERT INTO type_users
	(	
		type_id,
		usr_id,
		event_mask
	)
	VALUES (
		:db_type_id,
		:db_usr_id,
		:db_event_mask
	);

	if (sqlca.sqlcode)
	{
		logStream (MESSAGE_ERROR) << "cannot insert event trigger record " << sqlca.sqlerrm.sqlerrmc << sendLog;
		EXEC SQL ROLLBACK;
		return -1;
	}

	push_back (TypeUser (type, flags));

	EXEC SQL COMMIT;
	return 0;
}

int TypeUserSet::removeType (int id, char type)
{
	EXEC SQL BEGIN DECLARE SECTION;
	int db_id = id;
	char db_type = type;
	EXEC SQL END DECLARE SECTION;

	EXEC SQL DELETE FROM
		type_users
	WHERE
		type_id = :db_type AND usr_id = :db_id;
	
	if (sqlca.sqlcode)
	{
		logStream (MESSAGE_ERROR) << "error while removing user " << id 
			<< " and type " << type << "." << sendLog;
		EXEC SQL ROLLBACK;
		return -1;
	}

	EXEC SQL COMMIT;
	return 0;
}

User::User ()
{
 	id = -1;
	types = NULL;
}

User::User (int in_id, std::string in_login, std::string in_email)
{
	id = in_id;
	login = in_login;
	email = in_email;

	types = NULL;
}

User::~User (void)
{
	delete types;
}

int User::load (const char * in_login)
{
	EXEC SQL BEGIN DECLARE SECTION;
	int db_id;
	VARCHAR db_login[25];
	VARCHAR db_email[200];
	EXEC SQL END DECLARE SECTION;

	if (strlen (in_login) > 25)
	{
		logStream (MESSAGE_ERROR) << "login string '" << in_login << "' too long." << sendLog;
		return -1;
	}

	strcpy (db_login.arr, in_login);
	db_login.len = strlen (in_login);

	EXEC SQL SELECT
		usr_id,
		usr_email
	INTO
		:db_id,
		:db_email
	FROM
		users
	WHERE
		usr_login = :db_login;

	if (sqlca.sqlcode)
	{
		logStream (MESSAGE_ERROR) << "cannot find user with login " << in_login << "." << sendLog;
		id = -1;
		return -1;
	}
	id = db_id;
	login = std::string (in_login);
	email = std::string (db_email.arr);

	return loadTypes ();
}

int User::loadTypes ()
{
	delete types;
	types = new TypeUserSet (id);
	return 0;
}

int User::setPassword (std::string newPass)
{
	EXEC SQL BEGIN DECLARE SECTION;
	VARCHAR db_login[25];
	VARCHAR db_passwd[100];
	EXEC SQL END DECLARE SECTION;

	strncpy (db_login.arr, login.c_str (), 25);
	db_login.len = login.length ();

	if (newPass.length () > 100)
	{
		logStream (MESSAGE_ERROR) << "too long password" << sendLog;
		return -1;
	}

#ifdef RTS2_HAVE_CRYPT
	char salt[100];
	strcpy (salt, "$6$");
	random_salt (salt + 3, 8);
	strcpy (salt + 11, "$");
	strncpy (db_passwd.arr, crypt (newPass.c_str (), salt), 100);
	db_passwd.len = strlen (db_passwd.arr);
#else
	strncpy (db_passwd.arr, newPass.c_str (), 100);
	db_passwd.len = newPass.length ();
#endif

	EXEC SQL UPDATE
		users
	SET
		usr_passwd = :db_passwd
	WHERE
		usr_login = :db_login;

	if (sqlca.sqlcode)
	{
		logStream (MESSAGE_ERROR) << "error updating password " << sqlca.sqlerrm.sqlerrmc << sendLog;
		EXEC SQL ROLLBACK;
		return -1;
	}

	EXEC SQL COMMIT;

	return 0;
}

int User::setEmail (std::string newEmail)
{
	EXEC SQL BEGIN DECLARE SECTION;
	VARCHAR db_login[25];
	VARCHAR db_email[200];
	EXEC SQL END DECLARE SECTION;

	strncpy (db_login.arr, login.c_str (), 25);
	db_login.len = login.length ();

	if (newEmail.length () > 200)
	{
		logStream (MESSAGE_ERROR) << "too long email" << sendLog;
		return -1;
	}

	strncpy (db_email.arr, newEmail.c_str (), 200);
	db_email.len = newEmail.length ();

	EXEC SQL UPDATE
		users
	SET
		usr_email = :db_email
	WHERE
		usr_login = :db_login;

	if (sqlca.sqlcode)
	{
		logStream (MESSAGE_ERROR) << "error updating email " << sqlca.sqlerrm.sqlerrmc << sendLog;
		EXEC SQL ROLLBACK;
		return -1;
	}

	EXEC SQL COMMIT;

	return 0;
}

bool verifyUser (std::string username, std::string pass, bool &executePermission)
{
	EXEC SQL BEGIN DECLARE SECTION;
	VARCHAR db_username[25];
	VARCHAR d_passwd[100];
	int d_pass_ind;
	bool d_executePermission;
	EXEC SQL END DECLARE SECTION;

	db_username.len = username.length ();
	db_username.len = db_username.len > 25 ? 25 : db_username.len;
	strncpy (db_username.arr, username.c_str (), db_username.len);

	EXEC SQL BEGIN TRANSACTION;

	EXEC SQL DECLARE verify_cur CURSOR FOR
		SELECT
			usr_passwd,
			usr_execute_permission
		FROM
			users
		WHERE
			usr_login = :db_username;

	EXEC SQL OPEN verify_cur;

	EXEC SQL FETCH next FROM verify_cur INTO :d_passwd :d_pass_ind, :d_executePermission;

	if (sqlca.sqlcode == 0)
	{
		executePermission = d_executePermission;
		// null password - user blocked
		if (d_pass_ind)
			return false;
		d_passwd.arr[d_passwd.len] = '\0';
		EXEC SQL ROLLBACK;
#ifdef RTS2_HAVE_CRYPT
		return strcmp (crypt (pass.c_str (), d_passwd.arr), d_passwd.arr) == 0;
#else
		return pass == std::string (d_passwd.arr);
#endif
	}
	executePermission = false;
	EXEC SQL ROLLBACK;
	return false;
}
