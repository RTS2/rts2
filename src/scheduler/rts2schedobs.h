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

#include "../utilsdb/target.h"
#include <ostream>

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
		Target *target;
		double startJD;
		unsigned int loopCount;

	public:
		Rts2SchedObs (Target *_target, double _startJD, unsigned int _loopCount);
		virtual ~Rts2SchedObs (void);

		/**
		 * Return observation start JD.
		 *
		 * @return Start JD of the observation.
		 */
		double getJDStart ()
		{
			return startJD;
		}

		/**
		 * Returns target ID of the observation target.
		 *
		 * @return Target ID of the target.
		 */
		int getTargetID ()
		{
			return target->getTargetID ();
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
		 * Determines schedule target visibility.
		 *
		 * @return True is observation is visible.
		 */
		bool isVisible ()
		{
			return target->isGood (getJDStart ());
		}

		/**
		 * Return observation altitude merit computed from given period.
		 */
		double altitudeMerit (double _start, double _end);
};

std::ostream & operator << (std::ostream & _os, Rts2SchedObs & schedobs);

#endif // !__RTS2_SCHEDOBS__
