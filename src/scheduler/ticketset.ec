/*
 * Set of scheduling tickets.
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

#include "ticketset.h"
#include "../utilsdb/sqlerror.h"

using namespace rts2sched;

TicketSet::TicketSet (): std::map <int, Ticket *> ()
{
}


TicketSet::~TicketSet ()
{
	for (TicketSet::iterator iter = begin (); iter != end (); iter++)
		delete (*iter).second;
	
	clear ();
}


void
TicketSet::load (Rts2TargetSet *tarSet)
{
	EXEC SQL BEGIN DECLARE SECTION;
	int d_schedticket_id;
	int d_tar_id;
	int d_account_id;
	EXEC SQL END DECLARE SECTION;

	EXEC SQL DECLARE cur_tickets CURSOR FOR SELECT
		schedticket_id,
		tar_id,
		account_id
	FROM
		tickets;

	EXEC SQL OPEN cur_tickets;
	if (sqlca.sqlcode)
		throw rts2db::SqlError();
	
	while (true)
	{
		EXEC SQL FETCH next FROM cur_tickets INTO
			:d_schedticket_id,
			:d_tar_id,
			:d_account_id;

		if (sqlca.sqlcode == ECPG_NOT_FOUND)
			break;
		else if (sqlca.sqlcode)
		  	throw rts2db::SqlError ();

		(*this)[d_schedticket_id] = new Ticket (d_schedticket_id, tarSet->getTarget (d_tar_id), d_account_id);
	}
}
