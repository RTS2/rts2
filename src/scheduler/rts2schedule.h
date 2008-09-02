/*
 * One observing schedule.
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

#ifndef __RTS2_SCHEDULE__
#define __RTS2_SCHEDULE__

#include "rts2schedobs.h"

#include <vector>

/**
 * Observing schedule. This class provides holder of observing schedule, which
 * is a set of Rts2SchedObs objects. It also provides methods to manimulate
 * schedule.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Rts2Schedule: public std::vector <Rts2SchedObs*>
{
	private:
		double JDstart;
		double JDend;
		struct ln_lnlat_posn *observer;

		Rts2TargetSet *tarSet;

		// this variable is lazy initialized
		double visRatio;
	public:
		/**
		 * Create empty schedule.
		 *
		 * @param _JDstart Schedule start in julian date.
		 * @param _JDend   Schedule end in julian date.
		 * @param _obs     Observer position.
		 */
		Rts2Schedule (double _JDstart, double _JDend, struct ln_lnlat_posn *_obs);

		/**
		 * Create schedule by crossing two previous schedules.
		 *
		 * @param sched1      1st schedule to cross.
		 * @param sched2      2nd schedule to cross.
		 * @param crossPoint  Index of an element at which schedules will cross.
		 */
		Rts2Schedule (Rts2Schedule *sched1, Rts2Schedule *sched2, unsigned int crossPoint);

		/**
		 * Destroy observation schedule. Delete all scheduled observations.
		 */
		~Rts2Schedule (void);

		/**
		 * Generate random target for schedule.
		 *
		 * @return New Target object, or NULL if generation failed.
		 */
		Target * randomTarget ();

		/**
		 * Generate random observation schedule entry.
		 *
		 * @param JD Observation start interval.
		 *
		 * @return New schedule entry with random target.
		 */
		Rts2SchedObs * randomSchedObs (double JD);

		/**
		 * Construct observation schedule which fills time from JDstart to JDend.
		 *
		 * @param _tarSet Target set which contains targets which schedule will hold.
		 *
		 * @return 0 on success, -1 on error.
		 */
		int constructSchedule (Rts2TargetSet *_tarSet);

		/**
		 * Ratio of observations from schedule which are visible.
		 *
		 * @return Ration of visible targets. Higher means better schedule.
		 */
		double visibilityRatio ();

		/**
		 * Returns averaged altitude merit function.
		 *
		 * @return Averaged altitudu merit function of the observations.
		 */
		double altitudeMerit ();

};

std::ostream & operator << (std::ostream & _os, Rts2Schedule & schedule);
#endif							 // !__RTS2_SCHEDULE__
