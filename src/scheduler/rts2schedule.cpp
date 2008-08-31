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

Rts2Schedule::Rts2Schedule (double _JDstart, double _JDend):
std::set <Rts2SchedObs> ()
{
	JDstart = _JDstart;
	JDend = _JDend;
}


Rts2Schedule::~Rts2Schedule (void)
{

}


int
Rts2Schedule::constructSchedule ()
{

}


std::ostream & operator << (std::ostream & _os, Rts2Schedule & schedule)
{
	for (Rts2Schedule::iterator iter = schedule.begin (); iter != schedule.end (); iter++)
	{
		_os << (*iter) << std::endl;
	}
	return _os;
}
