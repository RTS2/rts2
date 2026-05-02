/*
 * Account class.
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

#include "rts2db/account.h"
#include "rts2db/sqlerror.h"
#include "rts2db/devicedb.h"

using namespace rts2db;

Account::Account (int _id)
{
	id = _id;
}


Account::Account (int _id, std::string _name, float _share)
{
	id = _id;
	name = _name;
	share = _share;
}

int
Account::load ()
{
	if (checkDbConnection ())
		return -1;

	EXEC SQL BEGIN DECLARE SECTION;
	int d_account_id = id;
	VARCHAR d_account_name[150];
	float d_account_share;
	EXEC SQL END DECLARE SECTION;

	EXEC SQL SELECT
		account_name,
		account_share
	INTO
		:d_account_name,
		:d_account_share
	FROM
		accounts
	WHERE
		account_id = :d_account_id;

	if (sqlca.sqlcode)
	{
	  	SqlError _err = SqlError ();
		logStream (MESSAGE_ERROR) << "Loading account " << id
			<< " gives error " << _err << sendLog;
		return -1;
	}
	name = std::string (d_account_name.arr);
	share = d_account_share;
	return 0;
}
