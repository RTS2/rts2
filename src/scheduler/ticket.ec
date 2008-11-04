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

#include "ticket.h"

using namespace rts2sched;

Ticket::Ticket (int _schedTicketId, Target *_target, int _accountId,
	unsigned int _obs_num, double _sched_from, double _sched_to,
	double _sched_interval_min, double _sched_interval_max)
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


bool
Ticket::violateSchedule (double _from, double _to)
{
	if (isnan (sched_from))
	{
		if (isnan (sched_to))
			return false;
		return _to > sched_to;
	}
	if (isnan (sched_to))
	{
	  	return _from < sched_from;
	}
	return (_from < sched_from) || (_to > sched_to);
}
