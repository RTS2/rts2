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

#include "rts2db/accountset.h"
#include "rts2db/sqlerror.h"
#include "rts2db/devicedb.h"

using namespace rts2db;

AccountSet *AccountSet::pInstance = NULL;

AccountSet::AccountSet (): std::map <int, Account * > ()
{
}


AccountSet::~AccountSet ()
{
	for (AccountSet::iterator iter = begin (); iter != end (); iter++)
		delete (*iter).second;

	clear ();
}


void
AccountSet::load ()
{
	if (checkDbConnection ())
		throw SqlError();

	EXEC SQL BEGIN DECLARE SECTION;
	int d_account_id;
	VARCHAR d_account_name[150];
	float d_account_share;
	EXEC SQL END DECLARE SECTION;

	EXEC SQL DECLARE cur_accounts CURSOR FOR SELECT
		account_id,
		account_name,
		account_share
	FROM
		accounts;

	EXEC SQL OPEN cur_accounts;
	if (sqlca.sqlcode)
		throw SqlError();

	shareSum = 0;

	while (true)
	{
		EXEC SQL FETCH next FROM cur_accounts INTO
			:d_account_id,
			:d_account_name,
			:d_account_share;

		if (sqlca.sqlcode == ECPG_NOT_FOUND)
			break;
		else if (sqlca.sqlcode)
			throw SqlError();

		(*this)[d_account_id] = new Account (d_account_id, std::string (d_account_name.arr), d_account_share);

		shareSum += d_account_share;
	}
}


AccountSet *
AccountSet::instance ()
{
	if (pInstance)
		return pInstance;
	pInstance = new AccountSet ();
	pInstance->load ();
	return pInstance;
}
