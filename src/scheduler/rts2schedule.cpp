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

#include "rts2schedule.h"
#include "../utilsdb/accountset.h"
#include "../utilsdb/rts2targetset.h"

using namespace rts2db;

Rts2Schedule::Rts2Schedule (double _JDstart, double _JDend, struct ln_lnlat_posn *_obs)
{
	JDstart = _JDstart;
	JDend = _JDend;
	observer = _obs;

	ticketSet = NULL;

	nanLazyMerits ();
}


Rts2Schedule::Rts2Schedule (Rts2Schedule *sched1, Rts2Schedule *sched2, unsigned int crossPoint)
{
	// fill in parameters..
  	JDstart = sched1->JDstart;
	JDend = sched1->JDend;
	observer = sched1->observer;

	ticketSet = sched1->ticketSet;

	nanLazyMerits ();

	unsigned int i;
	Rts2SchedObs *parent;
	// fill in schedobs
	for (i = 0; i < crossPoint; i++)
	{
		parent = (*sched1)[i];
		push_back (new Rts2SchedObs (parent->getTicket (), parent->getJDStart (), parent->getLoopCount ()));
	}

	for (; i < sched2->size (); i++)
	{
		parent = (*sched2)[i];
		push_back (new Rts2SchedObs (parent->getTicket (), parent->getJDStart (), parent->getLoopCount ()));
	}
}


Rts2Schedule::~Rts2Schedule (void)
{
	for (Rts2Schedule::iterator iter = begin (); iter != end (); iter++)
		delete (*iter);
	clear ();
}


bool
Rts2Schedule::isScheduled (Ticket *_ticket)
{
	for (Rts2Schedule::iterator iter = begin (); iter != end (); iter++)
	{
		if (_ticket->getTicketId () == (*iter)->getTicketId ())
			return true;
	}
	return false;
}


Ticket *
Rts2Schedule::randomTicket ()
{
	int rn = randomNumber (0, ticketSet->size ());
	TicketSet::iterator iter = ticketSet->begin ();
	while (rn > 0)
	{
		rn--;
		iter++;
	}
	// random selection of observation
	return (*iter).second;
}


Rts2SchedObs *
Rts2Schedule::randomSchedObs (double JD)
{
	nanLazyMerits ();
	return new Rts2SchedObs (randomTicket (), JD, 1);
}


int
Rts2Schedule::constructSchedule (TicketSet *_ticketSet)
{
	ticketSet = _ticketSet;
	double JD = JDstart;
	while (JD < JDend)
	{
		push_back (randomSchedObs (JD));
		JD += (double) 10.0 / 1440.0;
	}
	return 0;
}


double
Rts2Schedule::visibilityRatio ()
{
	if (!isnan (visRatio))
		return visRatio;

	visible = 0;
	unvisible = 0;

	for (Rts2Schedule::iterator iter = begin (); iter != end (); iter++)
	{
		if ((*iter)->isVisible ())
			visible ++;
		else
			unvisible++;
	}
	visRatio = (double) visible / size ();
	return visRatio;
}


double
Rts2Schedule::altitudeMerit ()
{
	if (!isnan (altMerit))
		return altMerit;

	altMerit = 0;
	for (Rts2Schedule::iterator iter = begin (); iter != end (); iter++)
	{
		altMerit += (*iter)->altitudeMerit (JDstart, JDend);
	}
	altMerit /= size ();
	return altMerit;
}


double
Rts2Schedule::accountMerit ()
{
	if (!isnan (accMerit))
		return accMerit;

	AccountSet *accountset = AccountSet::instance ();

	// map for storing calculated account time by account ids
	std::map <int, double> observedShares;

	// and fill map with all possible account IDs
	for (AccountSet::iterator iter = accountset->begin (); iter != accountset->end (); iter++)
	{
		observedShares[(*iter).first] = 0;
	}


	double sumDur = 0;

	for (Rts2Schedule::iterator iter = begin (); iter != end (); iter++)
	{
		observedShares[(*iter)->getAccountId ()] += (*iter)->getTotalDuration ();
		sumDur += (*iter)->getTotalDuration ();
	}

	// deviances and sum them
	accMerit = 0;

	for (AccountSet::iterator iter = accountset->begin (); iter != accountset->end (); iter++)
	{
		double sh = (*iter).second->getShare () / accountset->getShareSum ();
		accMerit += fabs (observedShares[(*iter).first] / sumDur - sh) * sh;
	}

	accMerit = (double) 1.0 / accMerit;

	return accMerit;
}


double
Rts2Schedule::distanceMerit ()
{
	if (!isnan (distMerit))
		return distMerit;

	if (size () <= 1)
	{
		distMerit = 1;
		return distMerit;
	}

	// schedule iterators for two targets..
	Rts2Schedule::iterator iter1 = begin ();
	Rts2Schedule::iterator iter2 = begin () + 1;

	distMerit = 0;

	for ( ; iter2 != end (); iter1++, iter2++)
	{
		struct ln_equ_posn pos1, pos2;
		(*iter1)->getEndPosition (pos1);
		(*iter2)->getStartPosition (pos2);
		distMerit += ln_get_angular_separation (&pos1, &pos2);
	}
	distMerit /= size ();
	if (distMerit == 0)
	{
		distMerit = 20000;
	}
	else
	{
		distMerit = 1.0 / distMerit;
	}
	
	return distMerit;
}


unsigned int
Rts2Schedule::violateSchedule ()
{
	if (violatedSch != UINT_MAX)
		return violatedSch;

	violatedSch = 0;

	for (Rts2Schedule::iterator iter = begin (); iter != end (); iter++)
	{
		if ((*iter)->violateSchedule ())
			violatedSch++;
	}

	return violatedSch;
}


unsigned int
Rts2Schedule::unobservedSchedules ()
{
	if (unobservedSch != UINT_MAX)
		return unobservedSch;

	unobservedSch = 0;

	for (TicketSet::iterator iter = ticketSet->begin (); iter != ticketSet->end (); iter++)
	{
		if ((*iter).second->shouldBeObservedDuring (JDstart, JDend))
		  	if (!isScheduled ((*iter).second))
				unobservedSch++;
	}

	return unobservedSch;
}


unsigned int
Rts2Schedule::violatedObsNum ()
{
	if (violatedObsN != UINT_MAX)
		return violatedObsN;
	
	violatedObsN = 0;
	
	// now calculate how many times each ticket was observed
	for (Rts2Schedule::iterator iter = begin (); iter != end (); iter++)
	{
		if ((++ticketObs[(*iter)->getTicketId ()]) > (*iter)->getTicket ()->getObsNum ())
			violatedObsN++;
	}

	return violatedObsN;
}


std::ostream &
operator << (std::ostream & _os, Rts2Schedule & schedule)
{
	for (Rts2Schedule::iterator iter = schedule.begin (); iter != schedule.end (); iter++)
	{
		_os << *(*iter) << std::endl;
	}
	return _os;
}
