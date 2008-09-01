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
	int gen = random () * sched->size () / RAND_MAX;

	double JD = (*sched)[gen]->getJDStart ();

	delete (*sched)[gen];
	(*sched)[gen] = sched->randomSchedObs (JD);
}

void
Rts2SchedBag::cross (Rts2Schedule * sched1, Rts2Schedule * sched2)
{

}

Rts2SchedBag::Rts2SchedBag (double _JDstart, double _JDend)
{
	JDstart = _JDstart;
	JDend = _JDend;
}


Rts2SchedBag::~Rts2SchedBag (void)
{
	for (Rts2SchedBag::iterator iter = begin (); iter != end (); iter++)
	{
		delete *iter;
	}
	clear ();
}


int
Rts2SchedBag::constructSchedules (int num)
{
	struct ln_lnlat_posn *observer = Rts2Config::instance ()->getObserver ();

	for (int i = 0; i < num; i++)
	{
		Rts2Schedule *sched = new Rts2Schedule (JDstart, JDend, observer);
		if (sched->constructSchedule ())
			return -1;
		push_back (sched);
	}
  	mutationNum = size () / 5;

	return 0;
}


void
Rts2SchedBag::doGAStep ()
{
	// mutate population
  	int rMax = random () * mutationNum / RAND_MAX;
	for (int i = 0; i < rMax; i++)
	{
		mutate ((*this)[random () * size () / RAND_MAX]);	
	}
}
