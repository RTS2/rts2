/* 
 * Sets of Auger targets.
 * Copyright (C) 2009 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_AUGERSET__
#define __RTS2_AUGERSET__

#include <map>

#include "target_auger.h"

namespace rts2db
{

class AugerSet:public std::map <int, TargetAuger>
{
	public:
		AugerSet () {}

		/**
		 * Load target from given date range.
		 */
		void load (double from, double to);

		void printHTMLTable (std::ostringstream &_os);
};

/**
 * Load data from auger table, grouped by various depth of grouping time parameter.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class AugerSetDate:public std::map <int, int>
{
	public:
		AugerSetDate () {}

		/**
		 * Construct set targets for Auger target. It select targets from auger informations. The parameters
		 * select which part of the date should be considered. If the date paremeter is -1, the load method
		 * select all targets, regardless of the date value. If it is > 0, select run on all
		 * dates which have the component equal to this value.
		 * This works great for hierarchical browsing. Suppose that we would like to select all targets
		 * in year 2009 and 2010. We offer user a choice which month are available. If he click on December 2009,
		 * we select all targets for December 2009, and allow user to select day. For each entry, it then creates entry in the map,
		 * together with number of targets.
		 *
		 * @param year    Year of selection.
		 * @param month   Month of selection.
		 * @param day     Selection day.
		 */
		void load (int year = -1, int month = -1, int day = -1, int hour = 1, int minutes = -1);
};

}

#endif	 /* !__RTS2_AUGERSET__ */
