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
#ifndef __RTS2SCHED_TICKET__
#define __RTS2SCHED_TICKET__

#include "infoval.h"
#include "rts2db/target.h"

namespace rts2sched
{
/**
 * Holds information about target scheduling ticket - account to which time will be
 * accounted, number of target observations, and target which will be observed.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Ticket
{
	public:
		/**
		 * Create ticket from given id.
		 *
		 * @param _schedTicketId   Ticket ID.
		 */
		Ticket (int _schedTicketId);

		/**
		 * Create scheduling ticket with a given parameters.
		 *
		 * @param _ticketId   ID of scheduling ticket.
		 * @param _target     Target class.
		 * @param _accountId  Id of ticket account.
		 * @param _obs_num    Number of observations which can be taken with this ticket.
		 * @param _sched_from Scheduling from this time. When nan, there is not from restricion.
		 * @param _sched_to   Scheduling to this time. When nan, there is not to restriction.
		 * @param _sched_interval_min
		 * @param _sched_interval_max
		 */
		Ticket (int _schedTicketId, rts2db::Target *_target, int _accountId,
			unsigned int _obs_num, double _sched_from, double _sched_to,
			double _sched_interval_min, double _sched_interval_max);

		/**
		 * Load ticket from database.
		 *
		 * @throw rts2db::SqlError in case of DB error.
		 */
		void load ();

		/**
		 * Returns ticket ID.
		 *
		 * @return Ticket ID.
		 */
		int getTicketId () { return ticketId; }

		/**
		 * Return target associated with scheduling ticket.
		 *
		 * @return Target object associated with scheduling.
		 */
		rts2db::Target *getTarget () { return target; }

		/**
		 * Return ID of target associated with the scheduling ticket.
		 *
		 * @return Id of target associated with scheduling ticket.
		 */
		int getTargetId () { return target->getTargetID (); }

		/**
		 * Return ID of time scharing account associated with the
		 * scheduling ticket.
		 *
		 * @return Id of account associated with scheduling ticket.
		 */
		int getAccountId () { return accountId; }

		/**
		 * Returns true if ticket is violated if scheduled in given interval.
		 *
		 * @param _from  Test from this interval.
		 * @param _to    Test to this interval.
		 *
		 * @return True if scheduling ticket is violated in this interval.
		 */
		bool violateSchedule (double _from, double _to);

		/**
		 * Returns number of observations allocated to this ticket.
		 *
		 * @return obsNum;
		 */
		unsigned int getObsNum () { return obs_num; }

		/**
		 * Test if schedule should be observed during given interval.
		 *
		 * @param _start Test interval start date in JD.
		 * @param _end   Test interval end date in JD.
		 *
		 * @return
		 */
		bool shouldBeObservedDuring (double _start, double _end)
		{
			if (std::isnan (sched_from))
			{
				if (std::isnan (sched_to))
					return false;
				return sched_to > _start;
			}
			if (std::isnan (sched_to))
				return _end > sched_from;
			return !(_start > sched_to || _end < sched_from);
		}

		friend Rts2InfoValStream & operator << (Rts2InfoValStream & _os, Ticket & ticket)
		{
			_os
				<< InfoVal <int> ("TICKET_ID", ticket.ticketId)
				<< InfoVal <int> ("TARGET_ID", ticket.getTargetId ())
				<< InfoVal <int> ("ACCOUNT_ID", ticket.accountId)
				<< InfoVal <unsigned int> ("OBS_NUM", ticket.obs_num)
				<< InfoVal <TimeJD> ("SCHED_FROM", ticket.sched_from)
				<< InfoVal <TimeJD> ("SCHED_TO", ticket.sched_to)
				<< InfoVal <TimeDiff> ("SCHED_INTERVAL_MIN", ticket.sched_interval_min)
				<< InfoVal <TimeDiff> ("SCHED_INTERVAL_MAX", ticket.sched_interval_max);
			return _os;
		}
	private:
		int ticketId;
		rts2db::Target *target;
		int accountId;

		unsigned int obs_num;

		double sched_from;
		double sched_to;

		double sched_interval_min;
		double sched_interval_max;
};

}

#endif				// !__RTS2SCHED_TICKET__
