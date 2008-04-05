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

#include "rts2user.h"

#include <iomanip>

Rts2User::Rts2User (int in_id, std::string in_login, std::string in_email)
{
	id = in_id;
	login = in_login;
	email = in_email;
}


Rts2User::~Rts2User (void)
{

}


std::ostream & operator << (std::ostream & _os, Rts2User &usr)
{
	_os << std::left << std::setw (5) << usr.id
		<< " " << std::setw (25) << usr.login << std::right
		<< " " << usr.email
		<< std::endl;
	return _os;
}


bool
verifyUser (std::string username, std::string pass)
{
	EXEC SQL BEGIN DECLARE SECTION;
	VARCHAR db_username[25];
	VARCHAR db_pass[25];
	int d_ret;
	EXEC SQL END DECLARE SECTION;
	db_username.len = username.length ();
	db_username.len = db_username.len > 25 ? 25 : db_username.len;
	strncpy (db_username.arr, username.c_str (), db_username.len);

	db_pass.len = pass.length ();
	db_pass.len = db_pass.len > 25 ? 25 : db_pass.len;
	strncpy (db_pass.arr, pass.c_str (), db_pass.len);

	EXEC SQL SELECT
		count (*)
	INTO
		:d_ret
	FROM
		users
	WHERE
		usr_login = :db_username
		AND usr_passwd = :db_pass;

	return d_ret == 1;
}
