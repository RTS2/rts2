/*
 * Set of schedules.
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

#include "rts2schedule.h"
#include "../utilsdb/accountset.h"

#include <vector>

/**
 * Class which holds schedules. It provides method for population initialization, and GA operations on schedule.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Rts2SchedBag:public std::vector <Rts2Schedule *>
{
	private:
		int mutationNum;
		unsigned int popSize;

		double mutateDurationRatio;
		double mutateSchedRatio;

		int maxTimeChange;
		int minObsDuration;

		// size of elite population 
		// if this is 0, elite GA is not used
		unsigned int eliteSize;

		double JDstart, JDend;

		rts2sched::TicketSet *ticketSet;
		Rts2TargetSet *tarSet;

		/**
		 * The algorithm replace randomly selected observation with randomly picked new
		 * one.
		 *
		 * @param sched Schedule which entry will be mutated.
		 */
		void mutateObs (Rts2Schedule * sched);

		/**
		 * Mutate duration of observation. Randomly adjust duration of the schedule.
		 * Split duration change evenly to both sides of the selected schedule.
		 *
		 * @param sched Schedule which entry will be mutated.
		 */
		void mutateDuration (Rts2Schedule * sched);

		/**
		 * Mutate schedule by removing a schedule entry. Time gained
		 * by schedule removal is added to the shortest schedule in set.
		 *
		 * @param sched Schedule which will be mutated.
		 */
		void mutateDelete (Rts2Schedule * sched);

		/**
		 * Mutate schedule.
		 *
		 * @param sched Schedule which will be mutated.
		 */
		void mutate (Rts2Schedule * sched);

		/**
		 * Do crossing of two schedules. This method calculate crossing
		 * parameters and performs crossing.
		 *
		 * @param sched1 Index of the 1st schedule to cross.
		 * @param sched2 Index of the 2nd schedule to croos.
		 */
		void cross (int sched1, int sched2)
		{
		 	cross ((*this)[sched1], (*this)[sched2]);
		}

		/**
		 * Do crossing of two schedules. This method calculate crossing
		 * parameters and performs crossing.
		 *
		 * @param sched1 Pointer to the 1st schedule to cross.
		 * @param sched2 Pointer to the 2nd schedule to croos.
		 */
		void cross (Rts2Schedule *parent1, Rts2Schedule *parent2);

		/**
		 * Select elite member of the population, delete non-elite members.
		 *
		 * @param _size Elite size.
		 */
		void pickElite (unsigned int _size)
		{
			pickElite (_size, begin (), end ());
		}
		
		void pickElite (unsigned int _size, Rts2SchedBag::iterator _begin, Rts2SchedBag::iterator _end);
	public:
		/**
		 * Construct emtpy schedule bag.
		 */
		Rts2SchedBag (double _JDstart, double _JDend);

		/**
		 * Delete schedules contained in schedule bag.
		 */
		~Rts2SchedBag (void);

		/**
		 * Return size of the elite population.
		 *
		 * @return Size of the elite population, 0 if elitism shall not be used.
		 */
		unsigned int getEliteSize ()
		{
			return eliteSize;
		}

		/**
		 * Construct schedules and add them to schedule bag.
		 *
		 * @param num Number of schedules.
		 *
		 * @return -1 on error, 0 on success.
		 */
		int constructSchedules (int num);


		/**
		 * Return min, average and max fittness of population.
		 *
		 * @param _min Population minimal fittness.
		 * @param _avg Population average fittness.
		 * @param _max Population maximal fittness.
		 * @param _type Statistict type - function which will be evaluated. Please 
		 * 	see objFunc enumaration for possible values.
		 *
		 * @see objFunc
		 */
		void getStatistics (double &_min, double &_avg, double &_max, objFunc _type = SINGLE);

		/**
		 * Return number of given constraint violation
		 *
		 * @param _type  Type of constraint violation which will be tested.
		 *
		 * @return 0 if all schedules in bag does not violate any constraint, otherwise number of constraint violated.
		 */
		unsigned int constraintViolation (constraintFunc _type);


		/**
		 * Do one step of a simple GA.
		 */
		void doGAStep ();

		// private functions used for NSGA-II
	private:
		// objectives which are used
		objFunc objectives[10];
		int objectivesSize;

		// constarint which are used
		constraintFunc constraints[10];
		int constraintSize;


		// vector of NSGA fronts members
		std::vector <std::vector <Rts2Schedule *> > NSGAfronts;

		// vector holding size of individual fronts
		std::vector <int> NSGAfrontsSize;

		/**
		 * Dominance operator.
		 *
		 * @param sched_1  First schedule which will be compared.
		 * @param sched_2  Second schedule which will be compared.
		 *
		 * @return <ul><li>-1 if first schedule dominates second</li><li>1 if second schedule dominates first</li><li>0 if schedules are equal</li>
		 */
		int dominatesNSGA (Rts2Schedule *sched_1, Rts2Schedule *sched_2);

		/** 
		 * Calculates crowding distance of each member in
		 * the set and sort NSGAfronts by crowding distance.
		 *
		 * @param f Front index (0..number of pareto fronts - 1)
		 */
		void calculateNSGACrowdingDistance (unsigned int f);


		/**
		 * Binary tournament selection for NSGA-II. This function uses
		 * rank and crowding distance to select a better schedule.
		 *
		 * @param sched1  First schedule to compare.
		 * @param sched2  Second schedule to compare.
		 *
		 * @return Better schedule.
		 */
		Rts2Schedule *tournamentNSGA (Rts2Schedule *sched1, Rts2Schedule *sched2);
	
	public:
		/**
		 * Calculate ranks of the entire population. Ranks are assigned to schedule
		 * with setNSGARank function.
		 */
		void calculateNSGARanks ();


		/** 
		 * Do one step of NSGA-II algorithm.
		 */
		void doNSGAIIStep ();

		/**
		 * Return population size with given rank.
		 *
		 * @param  _rank  NSGA rank which is queried.
		 *
		 * @return Size of population with getNSGARank == _rank.
		 */
		int getNSGARankSize (int _rank);
};
