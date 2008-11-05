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
#include "ticketset.h"

#include <vector>

typedef enum {
	VISIBILITY,     // ratio of visible targets. 1 = all targets are visible
	ALTITUDE,       // average altitude of the target. 1 = highest possible target altitude, 0 = on or bellow horizon
	ACCOUNT,        // ratio of account share. Higher = better
	DISTANCE,       // average distance between observations
	SCHEDULE_TIME,  // number of targets with scheduling constrain that are satisfied
	SINGLE		// single scheduling criteria
} objFunc;

typedef enum {
	CONSTR_VISIBILITY,	   // visibility violation - target is not visible
	CONSTR_SCHEDULE_TIME,	   // schedule violation - target is scheduled outside its allowed time OR there exists target(s) which was not scheduled while it must be scheduled
	CONSTR_UNOBSERVED_TICKETS, // number of unobserved tickets - tickets which have to time in schedule period, have some numObs left, but were not observed
	CONSTR_OBS_NUM		   // target is observed more times then it should be observed
} constraintFunc;

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

		// variables used for non-dominated sorting
		// rank - pareto front index
		int NSGARank;
		// crowding distance
		double NSGADistance;

		TicketSet *ticketSet;

		// following variables are lazy initialized
		double visRatio;
		double altMerit;
		double accMerit;
		double distMerit;

		unsigned int visible;
		unsigned int unvisible;

		unsigned int violatedSch;
		unsigned int unobservedSch;

		unsigned int violatedObsN;

		// used to calculate how many observations were performed on a given ticket
		std::map <int, unsigned int> ticketObs;

		// sets lazy merits to nan
		void nanLazyMerits ()
		{
			visRatio = nan ("f");
			altMerit = nan ("f");
			accMerit = nan ("f");
			distMerit = nan ("f");

			visible = UINT_MAX;
			unvisible = UINT_MAX;

			violatedSch = UINT_MAX;
			unobservedSch = UINT_MAX;

			violatedObsN = UINT_MAX;

			ticketObs.clear ();

			NSGARank = INT_MAX;
			NSGADistance = 0;
		}
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
		 * Get non-dominated rank (front) of the schedule.
		 */
		int getNSGARank ()
		{
			return NSGARank;
		}

		/**
		 * Set non-dominated rank (front) of the schedule.
		 *
		 * @param _rank New NSGA rank.
		 */
		void setNSGARank (int _rank)
		{
			NSGARank = _rank;
		}

		/**
		 * Returns NSGA crowding distance of the member.
		 */
		double getNSGADistance ()
		{
			return NSGADistance;
		}

		/**
		 * Set NSGA crowding distance of the schedule.
		 *
		 * @param _distance New NSGA crowding distance.
		 */
		void setNSGADistance (double _distance)
		{
			NSGADistance = _distance;
		}

		/**
		 * Increment NSGA crowding distance of the schedule.
		 *
		 * @param _inc Increment value.
		 */
		void incNSGADistance (double _inc)
		{
			NSGADistance += _inc;
		}

		/**
		 * Check if given ticket is properly scheduled in schedule.
		 */
		bool isScheduled (Ticket *_ticket);

		/**
		 * Generate random target for schedule.
		 *
		 * @return New Target object, or NULL if generation failed.
		 */
		Ticket * randomTicket ();

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
		int constructSchedule (TicketSet *_ticketSet);

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

		/**
		 * Returns schedule sharing differences merit. Lower value = better.
		 *
		 * @return 1 / sum of weighted deviations of the schedule from a
		 * requested time share. Higher value means better schedule.
		 */
		double accountMerit ();

		/**
		 * Returns merit based on distance between schedules entries.
		 *
		 * @return 1 / average distance between schedule entries.
		 */
		double distanceMerit ();

		/**
		 * Return used merit function.
		 */
		double singleOptimum ()
		{
			return distanceMerit ();
		}


		/**
		 * Return number of targets which are scheduled outside their scheduling
		 * interval.
		 */
		unsigned int violateSchedule ();

		/**
		 * Return number of targets which schedule will never be satisfied.
		 */
		unsigned int unobservedSchedules ();


		/**
		 * Return number of tickets, which violate their observing number.
		 */
		unsigned int violatedObsNum ();

		/**
		 * Return constraint function. Constraint is satisfied, if return is = 0. Otherwise
		 * number of constraint violations is returned.
		 *
		 * @param _type Constraint function type.
		 *
		 * @return Number of targets which are infeasible with respect to given constraint.
		 */
		double getConstraintFunction (constraintFunc _type)
		{
			switch (_type)
			{
				case CONSTR_VISIBILITY:
					visibilityRatio ();
					return unvisible;
				case CONSTR_SCHEDULE_TIME:
					return violateSchedule ();
				case CONSTR_UNOBSERVED_TICKETS:
					return unobservedSchedules ();
				case CONSTR_OBS_NUM:
					return violatedObsNum ();
			}
			return nan ("f");
		}

		/**
		 * Return some objective function based on the parameter.
		 *
		 * @param _type Objective function type.
		 *
		 * @return Objective function value.
		 *
		 * @see objFunc
		 */
		double getObjectiveFunction (objFunc _type)
		{
			switch (_type)
			{
				case VISIBILITY:
					return visibilityRatio ();
				case ALTITUDE:
					return altitudeMerit ();
				case ACCOUNT:
					return accountMerit ();
				case DISTANCE:
					return distanceMerit ();
				case SINGLE:
					break;
			}
			return singleOptimum ();
		}
};

std::ostream & operator << (std::ostream & _os, Rts2Schedule & schedule);
#endif							 // !__RTS2_SCHEDULE__
