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

	public:
		/**
		 * Create scheduling ticket with a given parameters.
		 *
		 * @param _ticketId  ID of scheduling ticket.
		 * @param _target  Target class.
		 * @param _accountId Id of 
		 */
		Ticket (int _schedTicketId, Target *_target, int _accountId);

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
};

}

#endif // !__RTS2SCHED_TICKET__
