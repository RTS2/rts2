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
#include "../utilsdb/rts2targetset.h"

Rts2Schedule::Rts2Schedule (double _JDstart, double _JDend, struct ln_lnlat_posn *_obs):
std::vector <Rts2SchedObs*> ()
{
	JDstart = _JDstart;
	JDend = _JDend;
	observer = _obs;

	tarSet = NULL;
}


Rts2Schedule::~Rts2Schedule (void)
{
	for (Rts2Schedule::iterator iter = begin (); iter != end (); iter++)
		delete (*iter);
	clear ();

	delete tarSet;
}


Target *
Rts2Schedule::randomTarget ()
{
	// random selection of observation
	int sel = random () * tarSet->size () / RAND_MAX;
	return createTarget ((*tarSet)[sel]->getTargetID (), observer);
}


Rts2SchedObs *
Rts2Schedule::randomSchedObs (double JD)
{
	return new Rts2SchedObs (randomTarget (), JD, 1);
}


int
Rts2Schedule::constructSchedule ()
{
	if (!tarSet)
  		tarSet = new Rts2TargetSetSelectable (observer);
	double JD = JDstart;
	while (JD < JDend)
	{
		push_back (randomSchedObs (JD));
		JD += (double) 10.0 / 1440.0;
	}
	return 0;
}


double
Rts2Schedule::visibilityRation ()
{
	unsigned int visible = 0;
	for (Rts2Schedule::iterator iter = begin (); iter != end (); iter++)
	{
		if ((*iter)->isVisible ())
			visible ++;
	}
	return (double) visible / size ();
}


double
Rts2Schedule::altitudeMerit ()
{
	double aMerit = 0;
	for (Rts2Schedule::iterator iter = begin (); iter != end (); iter++)
	{
		aMerit += (*iter)->altitudeMerit (JDstart, JDend);
	}
	return aMerit / size ();
}


std::ostream & operator << (std::ostream & _os, Rts2Schedule & schedule)
{
	for (Rts2Schedule::iterator iter = schedule.begin (); iter != schedule.end (); iter++)
	{
		_os << *(*iter) << std::endl;
	}
	return _os;
}
