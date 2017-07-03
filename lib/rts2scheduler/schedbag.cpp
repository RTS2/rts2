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

#include "rts2scheduler/schedbag.h"

#include "configuration.h"
#include "utilsfunc.h"

#include <algorithm>
#include <stdexcept>

void Rts2SchedBag::mutateObs (Rts2Schedule * sched)
{
	int gen = randomNumber (0, sched->size () - 1);

	double JD = (*sched)[gen]->getJDStart ();
	double dur = (*sched)[gen]->getTotalDuration ();

	delete (*sched)[gen];
	(*sched)[gen] = sched->randomSchedObs (JD, dur);
}

void Rts2SchedBag::mutateDuration (Rts2Schedule * sched)
{
	if (sched->size () == 1)
	{
		// decrease current schedule time..
		int dur = randomNumber (minObsDuration, (JDend - JDstart) * 86400 - 2 * minObsDuration);
		(*(sched->begin ()))->incTotalDuration (-1 * dur);
		// and add new schedule
		sched->push_back (sched->randomSchedObs ((*(sched->begin ()))->getJDEnd (), dur));
		return;
	}

	double adj = randomNumber (0, maxTimeChange * 2);
	// allow time decreasing as well..
	adj -= maxTimeChange;

	// pick observation which will be mutated
	unsigned int gen = randomNumber (0, sched->size () - 1);
	(*sched)[gen]->incTotalDuration (adj);
	if ((*sched)[gen]->getTotalDuration () < minObsDuration)
	{
		if (adj < 0)
		{
			// in this case, change sign of duration..
			(*sched)[gen]->incTotalDuration (-2 * adj);
			adj *= -1;
		}
		else
		{
			// otherwise repair minimal duration
			double adj2 = (minObsDuration + 2 * randomNumber (0, minObsDuration) - (*sched)[gen]->getTotalDuration ());
			(*sched)[gen]->incTotalDuration (adj2);
			adj += adj2;
		}
	}
	
	// when adjusting, adjust by opossite time..
	adj = adj / -2.0;
	// adjust schedule time..
	// special cases first
	if (gen == 0)
	{
		sched->adjustDuration (sched->begin() + 1, adj);
		sched->adjustDuration (sched->end () - 1, adj);
	}
	else if (gen == sched->size () - 1)
	{
		sched->adjustDuration (sched->begin(), adj);
		sched->adjustDuration (sched->end () - 2, adj);
	}
	else
	{
		sched->adjustDuration (sched->begin () + (gen - 1), adj);
		sched->adjustDuration (sched->begin () + (gen + 1), adj);
	}
	sched->repairStartTimes ();
}

void Rts2SchedBag::mutateDelete (Rts2Schedule * sched)
{
	// pick random schedule..
	int rs = randomNumber (0, sched->size () - 1);
	
	Rts2Schedule::iterator picked = sched->begin () + rs;

	double adj = (*picked)->getTotalDuration ();

	delete (*picked);

	if (picked == sched->begin ())
	{
		picked = sched->erase (picked);
		(*(picked))->setJDStart (JDstart);
	}
	else
	{
		picked = sched->erase (picked);
	}
	
	// pick shortest schedule 
	(*(sched->findShortest ()))->incTotalDuration (adj);

	sched->repairStartTimes (); 
}

void Rts2SchedBag::mutate (Rts2Schedule * sched)
{
	int rn = randomNumber (0, 100);
	if (rn < mutateDurationRatio * 100 || sched->size () == 1)
	{
		mutateObs (sched);
	}
	else if (rn < (mutateDurationRatio + mutateSchedRatio) * 100)
	{
	  	mutateDuration (sched);
	}
	else
	{
		mutateDelete (sched);
	}
}

void Rts2SchedBag::cross (Rts2Schedule *parent1, Rts2Schedule *parent2)
{
	// select crossing point - second in schedule
	unsigned int crossPoint = randomNumber (minObsDuration, (JDend - JDstart) * 86400 - 2 * minObsDuration);

	// have a sex
	Rts2Schedule *child1 = new Rts2Schedule (parent1, parent2, crossPoint);
	Rts2Schedule *child2 = new Rts2Schedule (parent2, parent1, crossPoint);

	push_back (child1);
	push_back (child2);
}

/**
 * Class for elite sorting. Compares schedules based on a result of
 * specified fittness function.
 */
class objectiveFunctionSort
{
	private:
		// comparsion objective
		objFunc objective;
	public:
		/**
		 * Construct new objective sort class.
		 *
		 * @param _objective  Objective on which comparsion will be based.
		 */
		objectiveFunctionSort (objFunc _objective) { objective = _objective; }
	
		/**
		 * Operator to compare schedules.
		 *
		 * @param sched1 First schedule to compare.
		 * @param sched2 Second schedule to compare.
		 *
		 * @return True if singleOptimum function of the first schedule is higher then second.
		 */
		bool operator () (Rts2Schedule *sched1, Rts2Schedule * sched2) { return sched1->getObjectiveFunction (objective) > sched2->getObjectiveFunction (objective); }
};

void Rts2SchedBag::pickElite (int _size, Rts2SchedBag::iterator _begin, Rts2SchedBag::iterator _end)
{
	// check if we can do it..
	if (_end - _begin < _size)
	{
		std::cerr << std::endl << "Too small size to choose elite. There are only " << size ()
			<< " elements, but " << eliteSize << " are required. Most probably some parameters are set wrongly."
			<< std::endl;
		exit (0);
	}

	std::sort (_begin, _end, objectiveFunctionSort (SINGLE));

	// delete last members
	for (Rts2SchedBag::iterator iter = _end - _size; iter != _end; iter++)
		delete (*iter);

	// remove deleted members..
	erase (_end - _size, _end);
}

Rts2SchedBag::Rts2SchedBag (double _JDstart, double _JDend)
{
	JDstart = _JDstart;
	JDend = _JDend;

	struct ln_lnlat_posn *observer = rts2core::Configuration::instance ()->getObserver ();

	tarSet = new rts2db::TargetSet (observer);
	tarSet->load ();

	ticketSet = new rts2sched::TicketSet ();

	mutationNum = -1;
	popSize = 0;

	mutateDurationRatio = 0.45;
	mutateSchedRatio = 0.75;

	maxTimeChange = 300;
	minObsDuration = 10;

	eliteSize = 0;

	// fill in parameters for NSGA
	objectives.push_back (ALTITUDE);
	objectives.push_back (ACCOUNT);
	// objectives.push_back (DISTANCE);
	objectives.push_back (DIVERSITY_TARGET);
	objectives.push_back (DIVERSITY_OBSERVATIONS);

	constraints.push_back (CONSTR_VISIBILITY);
	constraints.push_back (CONSTR_SCHEDULE_TIME);
	constraints.push_back (CONSTR_UNOBSERVED_TICKETS);
	constraints.push_back (CONSTR_OBS_NUM);
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

int Rts2SchedBag::constructSchedules (int num)
{
	struct ln_lnlat_posn *observer = rts2core::Configuration::instance ()->getObserver ();

	ticketSet->load (tarSet);
	if (ticketSet->size () == 0)
	{
		logStream (MESSAGE_ERROR) << "There aren't any scheduling tickets in database (tickets table)" << sendLog;
		return -1;
	}

	for (int i = 0; i < num; i++)
	{
		Rts2Schedule *sched = new Rts2Schedule (JDstart, JDend, minObsDuration, observer);
		if (sched->constructSchedule (ticketSet))
			return -1;
		push_back (sched);
	}

	popSize = size ();

	mutationNum = popSize / 2;

	reserve (popSize * 2);

	return 0;
}

int Rts2SchedBag::constructSchedulesFromObsSet (int num, struct ln_date *obsNight)
{
  	Rts2Night night = Rts2Night (obsNight, rts2core::Configuration::instance ()->getObserver ());

	rts2db::ObservationSet obsSet = rts2db::ObservationSet ();
	obsSet.loadTime (night.getFrom (), night.getTo ());

	if (obsSet.size () == 0)
	{
		logStream (MESSAGE_ERROR) << "Empty observation set" << sendLog;
		return -1;
	}

	JDstart = night.getJDFrom ();
	JDend = night.getJDTo ();

	if (std::isnan (JDstart) || std::isnan (JDend))
	{
	  	logStream (MESSAGE_ERROR) << "Null start or end dates" << sendLog;
		return -1;
	}

	struct ln_lnlat_posn *observer = rts2core::Configuration::instance ()->getObserver ();

	ticketSet->constructFromObsSet (tarSet, obsSet);
	if (ticketSet->size () == 0)
	{
		logStream (MESSAGE_ERROR) << "There aren't any scheduling tickets constructed from observations." << sendLog;
		return -1;
	}

	for (int i = 0; i < num; i++)
	{
		Rts2Schedule *sched = new Rts2Schedule (JDstart, JDend, minObsDuration, observer);
		if (sched->constructSchedule (ticketSet))
			return -1;
		push_back (sched);
	}

	popSize = size ();

	mutationNum = popSize / 2;

	reserve (popSize * 2);

	return 0;
}

void Rts2SchedBag::getStatistics (double &_min, double &_avg, double &_max, objFunc _type)
{
	_min = 1000000;
	_max = -1000000;
	_avg = 0;
	for (Rts2SchedBag::iterator iter = begin (); iter != end (); iter++)
	{
		double cur = (*iter)->getObjectiveFunction (_type);
		if (cur < _min)
			_min = cur;
		if (cur > _max)
		  	_max = cur;
		_avg += cur;	
	}
	_avg /= size ();
}

void Rts2SchedBag::getNSGAIIBestStatistics (double &_min, double &_avg, double &_max, objFunc _type)
{
	_min = 1000000;
	_max = -1000000;
	_avg = 0;
	int s = 0;
	for (Rts2SchedBag::iterator iter = begin (); iter != end (); iter++)
	{
		if ((*iter)->getNSGARank () != 0)
			continue;
		double cur = (*iter)->getObjectiveFunction (_type);
		if (cur < _min)
			_min = cur;
		if (cur > _max)
		  	_max = cur;
		_avg += cur;
		s++;
	}
	_avg /= s;
}

void Rts2SchedBag::getNSGAIIAverageDistance (double &_min, double &_avg, double &_max)
{
	_min = 1000000;
	_max = -1000000;
	_avg = 0;
	int s = 0;
	for (Rts2SchedBag::iterator iter = begin (); iter != end (); iter++)
	{
		if ((*iter)->getNSGARank () != 0)
			continue;
		double cur = (*iter)->averageDistance ();
		if (cur < _min)
			_min = cur;
		if (cur > _max)
		  	_max = cur;
		_avg += cur;
		s++;
	}
	_avg /= s;
}

unsigned int Rts2SchedBag::constraintViolation (constraintFunc _type)
{
	unsigned int ret = 0;
	for (Rts2SchedBag::iterator iter = begin (); iter != end (); iter++)
	{
		ret += (*iter)->getConstraintFunction (_type);
	}
	return ret;
}

void Rts2SchedBag::doGAStep ()
{
	Rts2SchedBag::iterator iter;

	// only the best..
	pickElite (popSize / 2);

	// random generation based on fittness
	double sumFitness = 0;
	for (iter = begin (); iter != end (); iter++)
		sumFitness += (*iter)->singleOptimum ();

	// have some sex..
	while (size () < popSize * 2)
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

	// mutate population
	int rMax = randomNumber (0, mutationNum);

	for (int i = 0; i < rMax; i++)
	{
		mutate ((*this)[randomNumber (0, size () - 1)]);
	}
}

int Rts2SchedBag::dominatesNSGA (Rts2Schedule *sched1, Rts2Schedule *sched2)
{
	// check for constraints
	bool dom1 = false;
	bool dom2 = false;
	for (std::list <constraintFunc>::iterator constIter  = constraints.begin (); constIter != constraints.end (); constIter++)
	{
		double cons1 = sched1->getConstraintFunction (*constIter);
		double cons2 = sched2->getConstraintFunction (*constIter);
		// if some schedule violate, prefer the one which does not violate..
		if (cons1 == 0 && cons2 > 0)
		  	return -1;
		if (cons1 > 0 && cons2 == 0)
			return 1;
		// if both are infeasible, prefer one which is closer to be feasible
		if (cons1 > 0 && cons2 > 0)
		{
			if (cons1 < cons2)
				dom1 = true;
			else if (cons1 > cons2)
			  	dom2 = true;
		}
	}
	for (std::list <objFunc>::iterator objIter = objectives.begin (); objIter != objectives.end (); objIter++)
	{
		double obj1 = sched1->getObjectiveFunction (*objIter);
		double obj2 = sched2->getObjectiveFunction (*objIter);
		if (obj1 > obj2)
			dom1 = true;
		else if (obj2 > obj1)
		  	dom2 = true;
	}
	if (dom1 && !dom2)
		return -1;
	else if (!dom1 && dom2)
	  	return 1;
	return 0;
}

void Rts2SchedBag::calculateNSGARanks ()
{
	// temporary structure which holds informations about ranks.
	// Indexed by population (=schedule) number
	struct {std::list <int> dominates; int dominated;} domStruct[size()];
	// list of schedules in fronts
	std::list <int> fronts[size ()];

	NSGAfronts.clear ();
	NSGAfrontsSize.clear ();

	NSGAfronts.push_back (std::vector <Rts2Schedule *> ());
	NSGAfrontsSize.push_back (0);

	for (unsigned int p = 0; p < size (); p++)
	{
		domStruct[p].dominated = 0;
		Rts2Schedule *sched_p = (*this)[p];
		for (unsigned int q = 0; q < size (); q++)
		{
		  	// do not calculate for ourselfs..
			if (p == q)
				continue;
			Rts2Schedule *sched_q = (*this)[q];
			int dom = dominatesNSGA (sched_p, sched_q);
			if (dom == -1)
				domStruct[p].dominates.push_back (q);
			else if (dom == 1)
			  	domStruct[p].dominated++;
		}
		if (domStruct[p].dominated == 0)
		{
			sched_p->setNSGARank (0);
			fronts[0].push_back (p);
			NSGAfronts[0].push_back (sched_p);
			NSGAfrontsSize[0]++;
		}
	}
	int i = 0;
	while (fronts[i].size () > 0)
	{
		NSGAfronts.push_back (std::vector <Rts2Schedule *> ());
		NSGAfrontsSize.push_back (0);
		for (std::list <int>::iterator p = fronts[i].begin (); p != fronts[i].end (); p++)
		{
			for (std::list <int>::iterator q = domStruct[*p].dominates.begin (); q != domStruct[*p].dominates.end (); q++)
			{
				domStruct[*q].dominated--;
				if (domStruct[*q].dominated == 0)
				{
				 	Rts2Schedule *sched_q = (*this)[*q];
					sched_q->setNSGARank (i + 1);
					fronts[i + 1].push_back (*q);
					NSGAfronts[i + 1].push_back (sched_q);
					NSGAfrontsSize[i + 1]++;
				}
			}
		}
		i++;
	}
}

// temporary operator for sorting based on crowding distance
struct crowdingComp {
	bool operator () (Rts2Schedule *sched1, Rts2Schedule *sched2)
	{
		return sched1->getNSGADistance () > sched2->getNSGADistance ();
	};
};

void Rts2SchedBag::calculateNSGACrowdingDistance (unsigned int f)
{
	std::vector <Rts2Schedule *>::iterator iter;

	if (NSGAfronts[f].size () <= 2)
	{
		// set to infinty and exit..
		for (iter = NSGAfronts[f].begin (); iter != NSGAfronts[f].end (); iter++)
			(*iter)->setNSGADistance (INFINITY);
		return;
	}

	// null distance
	for (iter = NSGAfronts[f].begin (); iter != NSGAfronts[f].end (); iter++)
		(*iter)->setNSGADistance (0);

	for (std::list <objFunc>::iterator objIter = objectives.begin (); objIter != objectives.end (); objIter++)
	{
		// sort front by objective i
		objFunc objective = *objIter;
		std::sort (NSGAfronts[f].begin (), NSGAfronts[f].end (), objectiveFunctionSort (objective));
		// assign infiniti values to first and last
		Rts2Schedule *begSched = *(NSGAfronts[f].begin ());
		Rts2Schedule *endSched = *(NSGAfronts[f].end () - 1);
		begSched->setNSGADistance (INFINITY);
		endSched->setNSGADistance (INFINITY);
		// function maxima and minima
		double f_max = begSched->getObjectiveFunction (objective);
		double f_min = endSched->getObjectiveFunction (objective);

		// function values, so getObjectiveFunction is called only once
		double f_1 = f_max;
		double f_2 = (*(NSGAfronts[f].begin () + 1))->getObjectiveFunction (objective);
		double f_3;

		for (iter = NSGAfronts[f].begin () + 1; iter != (NSGAfronts[f].end () - 1); iter++)
		{
			f_3 = (*(iter + 1))->getObjectiveFunction (objective);
			if (finite ((*iter)->getNSGADistance ()))
				(*iter)->incNSGADistance ((f_1 - f_3) / (f_max - f_min));
			f_1 = f_2;
			f_2 = f_3;
		}
	}
}

Rts2Schedule * Rts2SchedBag::tournamentNSGA (Rts2Schedule *sched1, Rts2Schedule *sched2)
{
	if (sched1->getNSGARank () < sched2->getNSGARank ())
		return sched1;
	if (sched1->getNSGADistance () > sched2->getNSGADistance ())
	  	return sched1;
	return (randomNumber (0, 100) > 50) ? sched1 : sched2;
}

void Rts2SchedBag::doNSGAIIStep ()
{
	// we hold pointers to both parent and child population used/produced by previous step
	calculateNSGARanks ();
	// pick n members as parents of new population
	Rts2Schedule *new_pop[popSize];
	unsigned int n = 0;
	unsigned int f;
	unsigned int i;
	for (f = 0; f < NSGAfronts.size (); f++)
	{
		calculateNSGACrowdingDistance (f);
		if (NSGAfronts[f].size () < (popSize - n))
		{
			// copy schedules..
			std::vector <Rts2Schedule *>::iterator iter;
			for (iter = NSGAfronts[f].begin (); iter != NSGAfronts[f].end(); iter++, n++)
				new_pop[n] = (*iter);
			
			NSGAfronts[f].clear ();
		}
		else
		{
			// sort based on crowding distance
			std::sort (NSGAfronts[f].begin (), NSGAfronts[f].end (), crowdingComp ());
			// copy missing entries..
			std::vector <Rts2Schedule *>::iterator iter = NSGAfronts[f].begin ();
			for (i = 0; n < popSize; i++, n++)
			{
				new_pop[n] = (*iter);
				iter = NSGAfronts[f].erase (iter);
			}
			// delete rest of NSGAfronts..
			while (iter != NSGAfronts[f].end ())
			{
				delete (*iter);
				iter = NSGAfronts[f].erase (iter);
			}
			while (f < NSGAfronts.size ())
			{
				iter = NSGAfronts[f].begin ();
				while (iter != NSGAfronts[f].end ())
				{
					delete (*iter);
					iter = NSGAfronts[f].erase (iter);
				}
				f++;
			}
		}
	}
	
	// erase current population - its pointer are either in new_pop or deleted..
	clear ();
	reserve (popSize * 2);

	// put to bag remaining schedules..
	for (i = 0; i < popSize; i++)
		push_back (new_pop[i]);

	// now new_pop holds members of new population ready for binary tournament..
	// we need to calculate indices of population for tournament
	unsigned int a1[popSize], a2[popSize];
	for (i = 0; i < popSize; i++)
		a1[i] = a2[i] = i;

	for (i = 0; i < popSize; i++)
	{
		unsigned int rand = randomNumber (i, popSize - 1);
		unsigned int temp = a1[rand];
		a1[rand] = a1[i];
		a1[i] = temp;

		rand = randomNumber (i, popSize - 1);
		temp = a2[rand];
		a2[rand] = a2[i];
		a2[i] = temp;
	}

	// do tournament
	for (i = 0; i < popSize; i+=4)
	{
		Rts2Schedule *parent1 = tournamentNSGA (new_pop[a1[i]], new_pop[a1[i+1]]);
		Rts2Schedule *parent2 = tournamentNSGA (new_pop[a1[i+2]], new_pop[a1[i+3]]);
		cross (parent1, parent2);

		parent1 = tournamentNSGA (new_pop[a2[i]], new_pop[a2[i+1]]);
		parent2 = tournamentNSGA (new_pop[a2[i+2]], new_pop[a2[i+3]]);
		cross (parent1, parent2);
	}

	for (int m = 0; m < mutationNum; m++)
	{
		mutate ((*this)[randomNumber (popSize, popSize * 2 - 1)]);
	}
}

int Rts2SchedBag::getNSGARankSize (int _rank)
{
	try
	{
		return NSGAfrontsSize.at(_rank);
	}
	catch (std::out_of_range _err)
	{
		return 0;
	}
}
