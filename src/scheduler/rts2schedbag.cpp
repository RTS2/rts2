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
		sched->constructSchedule ();
		push_back (sched);
	}
	return 0;
}
