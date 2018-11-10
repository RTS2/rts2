/* 
 * Plan set class.
 * Copyright (C) 2003-2007 Petr Kubanek <petr@kubanek.net
 * Copyright (C) 2011 Petr Kubanek, Institute of Physics <kubanek.net>
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

#ifndef __RTS2_PLANSET__
#define __RTS2_PLANSET__

#include "plan.h"

#include <list>
#include <ostream>
#include <string>
#include <time.h>

namespace rts2db
{

/**
 * List of plan entries, selected from various
 * filtering criteria.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class PlanSet:public std::list < Plan >
{
	public:
		PlanSet ();
		PlanSet (int prop_id);
		/**
		 * Extract plan entries from given time range.
		 *
		 * @param t_from time from (ctime)
		 * @param t_to   time to (ctime, can be null to indicate unlimited t_to)
		 */
		PlanSet (double t_from, double t_to);
		virtual ~PlanSet (void);

		void load ();

		friend std::ostream & operator << (std::ostream & _os, PlanSet & plan_set)
		{
			PlanSet::iterator plan_iter;
			for (plan_iter = plan_set.begin (); plan_iter != plan_set.end (); plan_iter++)
			{
				_os << *plan_iter;
			}
			return _os;
		}

		friend std::istream & operator >> (std::istream &is, PlanSet &plan_set)
		{
			while (true)
			{
				std::string buf;
				getline (is, buf);
				if (is.eof () || is.fail ())
					break;
				// ignore comments	
				if (buf[0] == '#')
					continue;
				std::istringstream iss (buf);
				Plan p;
				try
				{
					iss >> p;
				}
				catch (rts2core::Error &er)
				{
					throw rts2core::Error (buf + " " + er.what ());
				}
				plan_set.push_back (p);
			}
			return is;
		}

	protected:
		/**
		 * Fill in where part for given dates.
		 *
		 * @param t_from   time from (in ctime, e.g. seconde from 1-1-1970)
		 * @param t_to     time to (in ctime)
		 */
		void planFromTo (double t_from, double t_to);
		std::string where;
};

/**
 * Set of plan entries for given target.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class PlanSetTarget:public PlanSet
{
	public:
		PlanSetTarget (int tar_id);
};

/**
 * Plan set for given night.
 *
 * @author Petr Kubanek <kubanek@fzu.cz>
 */
class PlanSetNight:public PlanSet
{
	public:
		/**
		 * Select all plan entries for night which includes JD date specified as parameter.
		 * If JD is nan, it will select actual night.
		 */
		PlanSetNight (double JD = NAN);

		double getFrom () { return from; }
		double getTo () { return to; }

	private:
		double from;
		double to;		
};

}

#endif							 /* !__RTS2_PLANSET__ */
