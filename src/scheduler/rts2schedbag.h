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
		int popSize;

		double JDstart, JDend;

		Rts2TargetSetSelectable *tarSet;

		/**
		 * Do mutation on genetic algorithm. The algorithm select
		 * mutation gene with random generator.
		 *
		 * @param sched Schedule which will be mutated.
		 */
		void mutate (Rts2Schedule * sched);

		/**
		 * Do crossing of two schedules. This method calculate crossing
		 * parameters.
		 *
		 * @param sched1 Index of the 1st schedule to cross.
		 * @param sched2 Index of the 2nd schedule to croos.
		 */
		void cross (int sched1, int sched2);

		/**
		 * Select elite member of the population, delete non-elite members.
		 *
		 * @param eliteSize Elite size.
		 */
		void pickElite (unsigned int eliteSize);

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
		 */
		void getStatistics (double &_min, double &_avg, double &_max);


		/**
		 * Do one step of GA.
		 */
		void doGAStep ();
};
