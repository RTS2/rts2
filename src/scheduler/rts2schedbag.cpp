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

#include "rts2schedbag.h"

#include "../utils/rts2config.h"

void
Rts2SchedBag::mutate (Rts2Schedule * sched)
{
	int gen = randomNumber (0, sched->size ());

	double JD = (*sched)[gen]->getJDStart ();

	delete (*sched)[gen];
	(*sched)[gen] = sched->randomSchedObs (JD);
}


void
Rts2SchedBag::cross (int sched1, int sched2)
{
	Rts2Schedule *parent1 = (*this)[sched1];
	Rts2Schedule *parent2 = (*this)[sched2];

	// select crossing point
	int crossPoint = randomNumber (1, parent1->size() - 1);

	// have a sex
	Rts2Schedule *child1 = new Rts2Schedule (parent1, parent2, crossPoint);
	Rts2Schedule *child2 = new Rts2Schedule (parent2, parent1, crossPoint);

	push_back (child1);
	push_back (child2);
}


/**
 * Class for elite sorting. Compares schedules based on a result of
 * singleOptimum fittness function.
 */
struct singleEliteSort
{
	/**
	 * Operator to pick elite.
	 *
	 * @param sched1 First schedule to compare.
	 * @param
	 */
	bool operator() (Rts2Schedule * sched1, Rts2Schedule *sched2)
	{
		return sched1->singleOptimum () > sched2->singleOptimum ();
	}
};

void
Rts2SchedBag::pickElite (unsigned int _size, Rts2SchedBag::iterator _begin, Rts2SchedBag::iterator _end)
{
	// check if we can do it..
	if (_end - _begin < _size)
	{
		std::cerr << std::endl << "Too small size to choose elite. There are only " << size ()
			<< " elements, but " << eliteSize << " are required. Most probably some parameters are set wrongly."
			<< std::endl;
		exit (0);
	}

	std::sort (_begin, _end, singleEliteSort ());

	// remove last n members..
	erase (_end - _size, _end);
}


Rts2SchedBag::Rts2SchedBag (double _JDstart, double _JDend)
{
	JDstart = _JDstart;
	JDend = _JDend;

	struct ln_lnlat_posn *observer = Rts2Config::instance ()->getObserver ();

	tarSet = new Rts2TargetSet (observer, true);
	ticketSet = new rts2sched::TicketSet ();

	mutationNum = -1;
	popSize = -1;

	eliteSize = 0;
}


Rts2SchedBag::~Rts2SchedBag (void)
{
	for (Rts2SchedBag::iterator iter = begin (); iter != end (); iter++)
	{
		delete *iter;
	}
	clear ();

	delete ticketSet;
	delete tarSet;
}


int
Rts2SchedBag::constructSchedules (int num)
{
	struct ln_lnlat_posn *observer = Rts2Config::instance ()->getObserver ();

	ticketSet->load (tarSet);
	if (ticketSet->size () == 0)
	{
		logStream (MESSAGE_ERROR) << "There aren't any scheduling tickets in database (table tickets)" << sendLog;
		return -1;
	}

	for (int i = 0; i < num; i++)
	{
		Rts2Schedule *sched = new Rts2Schedule (JDstart, JDend, observer);
		if (sched->constructSchedule (ticketSet))
			return -1;
		push_back (sched);
	}

	mutationNum = 2;

	popSize = size ();

	return 0;
}


void
Rts2SchedBag::getStatistics (double &_min, double &_avg, double &_max)
{
	_min = 1000000;
	_max = -1000000;
	_avg = 0;
	for (Rts2SchedBag::iterator iter = begin (); iter != end (); iter++)
	{
		double cur = (*iter)->singleOptimum ();
		if (cur < _min)
			_min = cur;
		if (cur > _max)
		  	_max = cur;
		_avg += cur;	
	}
	_avg /= size ();
}


void
Rts2SchedBag::doGAStep ()
{
	// mutate population
	int rMax = randomNumber (0, mutationNum);

	Rts2SchedBag::iterator iter;

	// random generation based on fittness
	double sumFitness = 0;
	for (iter = begin (); iter != end (); iter++)
		sumFitness += (*iter)->singleOptimum ();

	// only the best..
	pickElite (popSize / 2);

	// have some sex..
	while ((int) size () < popSize * 2)
	{
	  	// select comulative indices..
		double rnum1 = sumFitness * random () / RAND_MAX;
		double rnum2 = sumFitness * random () / RAND_MAX;

		// parents indices
		int p1 = 0, p2 = 0;

		iter = begin ();
		int j = 0;
		double pSum = 0;
		double pNext = 0;
		// select parents
		while ((p1 == 0 || p2 == 0) && iter != end ())
		{
			pNext += (*iter)->singleOptimum ();
			if (rnum1 >= pSum && rnum1 < pNext)
				p1 = j;
			if (rnum2 >= pSum && rnum2 < pNext)
			  	p2 = j;
			iter ++;
			j++;
			pSum = pNext;
		}
		// care about end values
		if (iter == end ())
		{
			j--;
			if (p1 == 0)
				p1 = j;
			if (p2 == 0)
			  	p2 = j;
		}
		if (p1 != p2)
			cross (p1, p2);
	}

	for (int i = 0; i < rMax; i++)
	{
		mutate ((*this)[randomNumber (0, size ())]);
	}
}
