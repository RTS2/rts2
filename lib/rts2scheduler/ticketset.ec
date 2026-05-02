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

#include "rts2scheduler/ticketset.h"
#include "rts2db/sqlerror.h"
#include "rts2db/devicedb.h"

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
TicketSet::load (rts2db::TargetSet *tarSet)
{
	if (rts2db::checkDbConnection ())
		throw rts2db::SqlError ();

	EXEC SQL BEGIN DECLARE SECTION;
	int d_schedticket_id;
	int d_tar_id;
	int d_account_id;
	unsigned int d_obs_num;
	int d_obs_num_ind;
	long d_sched_from;
	int d_sched_from_ind;
	long d_sched_to;
	int d_sched_to_ind;
	double d_sched_interval_min;
	int d_sched_interval_min_ind;
	double d_sched_interval_max;
	int d_sched_interval_max_ind;
	EXEC SQL END DECLARE SECTION;

	double sched_from, sched_to;

	EXEC SQL DECLARE cur_tickets CURSOR FOR SELECT
		schedticket_id,
		tar_id,
		account_id,
		obs_num,
		EXTRACT (EPOCH FROM sched_from),
		EXTRACT (EPOCH FROM sched_to),
		EXTRACT (EPOCH FROM sched_interval_min),
		EXTRACT (EPOCH FROM sched_interval_max)
	FROM
		tickets
	WHERE
		((obs_num is NULL) or (obs_num > 0));

	EXEC SQL OPEN cur_tickets;
	if (sqlca.sqlcode)
		throw rts2db::SqlError();

	while (true)
	{
		EXEC SQL FETCH next FROM cur_tickets INTO
			:d_schedticket_id,
			:d_tar_id,
			:d_account_id,
			:d_obs_num :d_obs_num_ind,
			:d_sched_from :d_sched_from_ind,
			:d_sched_to :d_sched_to_ind,
			:d_sched_interval_min :d_sched_interval_min_ind,
			:d_sched_interval_max :d_sched_interval_max_ind;

		if (sqlca.sqlcode == ECPG_NOT_FOUND)
			break;
		else if (sqlca.sqlcode)
		  	throw rts2db::SqlError ();

		if (d_obs_num_ind < 0)
		  	d_obs_num = UINT_MAX;

		if (d_sched_from_ind < 0)
			sched_from = NAN;
		else
			sched_from = ln_get_julian_from_timet (&d_sched_from);

		if (d_sched_to_ind < 0)
		  	sched_to = NAN;
		else
		  	sched_to = ln_get_julian_from_timet (&d_sched_to);

		if (d_sched_interval_min_ind < 0)
			d_sched_interval_min = -1;
		if (d_sched_interval_max_ind < 0)
		  	d_sched_interval_max = -1;

		(*this)[d_schedticket_id] = new Ticket (d_schedticket_id, tarSet->getTarget (d_tar_id),
			d_account_id, d_obs_num, sched_from, sched_to,
			d_sched_interval_min, d_sched_interval_max);
	}
}


void
TicketSet::constructFromObsSet (rts2db::TargetSet *tarSet, rts2db::ObservationSet &obsSet)
{
	int ticket_id = 1;
	std::map <int, int> tarObs = obsSet.getTargetObservations ();
	for (std::map <int, int>::iterator iter = tarObs.begin (); iter != tarObs.end (); iter++, ticket_id++)
	{
		(*this)[ticket_id] = new Ticket (ticket_id, tarSet->getTarget ((*iter).first),
			0, (*iter).second, NAN, NAN, -1, -1);
	}
}
