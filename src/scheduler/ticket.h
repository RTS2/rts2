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

#include "../utilsdb/target.h"

namespace rts2sched {

/**
 * Holds information about target scheduling ticket - account to which time will be
 * accounted, number of target observations, and target which will be observed.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Ticket
{
  	private:
		int ticketId;
		Target *target;
		int accountId;
		double sched_from;
		double sched_to;

	public:
		/**
		 * Create scheduling ticket with a given parameters.
		 *
		 * @param _ticketId   ID of scheduling ticket.
		 * @param _target     Target class.
		 * @param _accountId  Id of ticket account.
		 * @param _sched_from Scheduling from this time. When nan, there is not from restricion.
		 * @param _sched_to   Scheduling to this time. When nan, there is not to restriction.
		 */
		Ticket (int _schedTicketId, Target *_target, int _accountId, double _sched_from, double _sched_to);

		/**
		 * Return target associated with scheduling ticket.
		 *
		 * @return Target object associated with scheduling.
		 */
		Target *getTarget ()
		{
			return target;
		}

		/**
		 * Return ID of target associated with the scheduling ticket.
		 *
		 * @return Id of target associated with scheduling ticket.
		 */
		int getTargetId ()
		{
			return target->getTargetID ();
		}

		/**
		 * Return ID of time scharing account associated with the
		 * scheduling ticket.
		 *
		 * @return Id of account associated with scheduling ticket.
		 */
		int getAccountId ()
		{
			return accountId;
		}

		/**
		 * Returns true if ticket is violated if scheduled in given interval.
		 *
		 * @param _from  Test from this interval.
		 * @param _to    Test to this interval.
		 *
		 * @return True if scheduling ticket is violated in this interval.
		 */
		bool violateSchedule (double _from, double _to);
};

}

#endif // !__RTS2SCHED_TICKET__
