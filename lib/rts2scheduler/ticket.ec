/*
 * Scheduling ticket.
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

#include "rts2scheduler/ticket.h"

#include "configuration.h"
#include "rts2db/sqlerror.h"
#include "rts2db/devicedb.h"

using namespace rts2sched;

Ticket::Ticket (int _schedTicketId)
{
	ticketId = _schedTicketId;
	target = NULL;
}

Ticket::Ticket (int _schedTicketId, rts2db::Target *_target, int _accountId, unsigned int _obs_num, double _sched_from, double _sched_to, double _sched_interval_min, double _sched_interval_max)
{
	ticketId = _schedTicketId;
	target = _target;
	accountId = _accountId;
	obs_num = _obs_num;
	sched_from = _sched_from;
	sched_to = _sched_to;
	sched_interval_min = _sched_interval_min;
	sched_interval_max = _sched_interval_max;
}

void Ticket::load ()
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

	d_schedticket_id = ticketId;

	EXEC SQL SELECT
			tar_id,
			account_id,
			obs_num,
			EXTRACT (EPOCH FROM sched_from),
			EXTRACT (EPOCH FROM sched_to),
			EXTRACT (EPOCH FROM sched_interval_min),
			EXTRACT (EPOCH FROM sched_interval_max)
		INTO
			:d_tar_id,
			:d_account_id,
			:d_obs_num :d_obs_num_ind,
			:d_sched_from :d_sched_from_ind,
			:d_sched_to :d_sched_to_ind,
			:d_sched_interval_min :d_sched_interval_min_ind,
			:d_sched_interval_max :d_sched_interval_max_ind
		FROM
			tickets
		WHERE
			schedticket_id = :d_schedticket_id;
	if (sqlca.sqlcode)
		throw rts2db::SqlError ();

	target = createTarget (d_tar_id, rts2core::Configuration::instance ()->getObserver (), rts2core::Configuration::instance ()->getObservatoryAltitude ());
	if (target == NULL)
		throw rts2db::SqlError ();

	accountId = d_account_id;

	if (d_obs_num_ind < 0)
		obs_num = UINT_MAX;
	else
		obs_num = d_obs_num;

	if (d_sched_from_ind < 0)
		sched_from = NAN;
	else
		sched_from = ln_get_julian_from_timet (&d_sched_from);

	if (d_sched_to_ind < 0)
		sched_to = NAN;
	else
		sched_to = ln_get_julian_from_timet (&d_sched_to);

	if (d_sched_interval_min_ind < 0)
		sched_interval_min = -1;
	else
		sched_interval_min = d_sched_interval_min;

	if (d_sched_interval_max_ind < 0)
		sched_interval_max = -1;
	else
		sched_interval_max = d_sched_interval_max;
}

bool Ticket::violateSchedule (double _from, double _to)
{
	if (std::isnan (sched_from))
	{
		if (std::isnan (sched_to))
			return false;
		return _to > sched_to;
	}
	if (std::isnan (sched_to))
	{
		return _from < sched_from;
	}
	return (_from < sched_from) || (_to > sched_to);
}
