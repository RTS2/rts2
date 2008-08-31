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

#include "rts2schedobs.h"

#include <set>

/**
 * Observing schedule. This class provides holder of observing schedule, which
 * is a set of Rts2SchedObs objects. It also provides methods to manimulate
 * schedule.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Rts2Schedule: public std::set <Rts2SchedObs>
{
	private:
		double JDstart;
		double JDend;
	public:
		Rts2Schedule (double _JDstart, double _JDend);
		virtual ~Rts2Schedule (void);

		/**
		 * This will construct observation schedule from JDstart to JDend.
		 *
		 * @return Number of elements in the constructed schedule, -1 on error.
		 */
		int constructSchedule ();
};

std::ostream & operator << (std::ostream & _os, Rts2Schedule & schedule);
