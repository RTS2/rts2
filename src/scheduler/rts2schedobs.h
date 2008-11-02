/*
 * Scheduled observation.
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

#ifndef __RTS2_SCHEDOBS__
#define __RTS2_SCHEDOBS__

#include "ticket.h"
#include <ostream>

#include "utils.h"

using namespace rts2sched;

/**
 * Single observation class. This class provides interface for a single
 * observing schedule entry - single observation. It provides methods to
 * calculate various observation fittens parameters.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Rts2SchedObs
{
	private:
		Ticket *ticket;
		double startJD;
		unsigned int loopCount;

	public:
		Rts2SchedObs (Ticket *_ticket, double _startJD, unsigned int _loopCount);
		virtual ~Rts2SchedObs (void);

		/**
		 * Return julian date of observation start.
		 *
		 * @return Start JD of the observation.
		 */
		double getJDStart ()
		{
			return startJD;
		}

		/**
		 * Return julian date of observation end.
		 *
		 * @return End JD of the observation.
		 */
		double getJDEnd ()
		{
			return startJD + 10 / 1440.0;
		}

		/**
		 * Return pointer to used observing ticket.
		 *
		 * @return Pointer to used observing ticket.
		 */
		Ticket *getTicket ()
		{
			return ticket;
		}

		/**
		 * Return pointer to used target object.
		 *
		 * @return Pointer to target object.
		 */
		Target *getTarget ()
		{
			return ticket->getTarget ();
		}

		/**
		 * Returns target ID of the observation target.
		 *
		 * @return Target ID of the target.
		 */
		int getTargetId ()
		{
			return ticket->getTargetId ();
		}

		/**
		 * Return id of account for which target should be assigned.
		 *
		 * @return Account id.
		 */
		int getAccountId ()
		{
			return ticket->getAccountId ();
		}

		/**
		 * Returns number of loops observation sequence will be carried.
		 *
		 * @return Number of loops observation sequence will be carried.
		 */
		int getLoopCount ()
		{
			return loopCount;
		}

		/**
		 * Get duration of whole target observation in seconds.
		 */
		float getTotalDuration ()
		{
			// TODO calculate this value
			return 10.0;
		}

		/**
		 * Determines schedule target visibility.
		 *
		 * @return True is observation is visible.
		 */
		bool isVisible ()
		{
			return getTarget()->isGood (getJDStart ());
		}

		/**
		 * Return observation altitude merit computed from given period.
		 */
		double altitudeMerit (double _start, double _end);

		/**
		 * Get equatiorial position of the target at the beginning of the observation.
		 *
		 * @param _pos Returned position.
		 */
		void getStartPosition (struct ln_equ_posn &_pos)
		{
			getTarget ()->getPosition (&_pos, getJDStart ());
		}

		/**
		 * Get equatiorial position of the target at the end of the observation.
		 *
		 * @param _pos Returned position.
		 */
		void getEndPosition (struct ln_equ_posn &_pos)
		{
			getTarget ()->getPosition (&_pos, getJDEnd ());
		}

		/**
		 * Returns schedule position at give julian date.
		 */
		void getPosition (struct ln_equ_posn &_pos, double JD)
		{
			getTarget ()->getPosition (&_pos, JD);
		}

		/**
		 * Return true if schedule for given ticket is violated.
		 */
		bool violateSchedule ()
		{
			return ticket->violateSchedule (getJDStart (), getJDEnd ());
		}

};

std::ostream & operator << (std::ostream & _os, Rts2SchedObs & schedobs);
#endif							 // !__RTS2_SCHEDOBS__
