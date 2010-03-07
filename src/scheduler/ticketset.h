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
#include "ticket.h"

#include "../utilsdb/observationset.h"

#include <map>

namespace rts2sched {

/**
 * Holds scheduling tickets, allow quick acces to them via target ID or 
 * ticket ID.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class TicketSet: public std::map <int, Ticket *>
{
	public:
		/**
		 * Construct an empty set of tickets.
		 */
		TicketSet ();

		/**
		 * Destroy all tickets entries which belongs to ticket set.
		 */
		~TicketSet ();

		/**
		 * Load ticket set from database.
		 *
		 * @param tarSet rts2db::TargetSet which contains target objects for scheduling tickets.
		 *
		 * @throw rts2db::SqlError if some database error occurred.
		 */
		void load (rts2db::TargetSet *tarSet);

		/**
		 * Construct ticket set from observation set. Used for testing of GA algorithm agains
		 * clasicall scheduling, and for evaluation of criteria of pareto fron selection.
		 *
		 * @param tarSet rts2db::TargetSet which contains target objects for scheduling tickets.
		 * @param obsSet Observation set.
		 */
		void constructFromObsSet (rts2db::TargetSet *tarSet, rts2db::ObservationSet &obsSet);
};

}
