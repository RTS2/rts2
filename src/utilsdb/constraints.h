/* 
 * Constraints.
 * Copyright (C) 2003-2007 Petr Kubanek <petr@kubanek.net>
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

#include "target.h"

#include <libxml/parser.h>
#include <libxml/tree.h>

namespace rts2db
{

/**
 * Abstract class for constraints.
 */
class Constraint
{
	public:
		Constraint () {};
		
		virtual bool satisfy (Target *tar, double JD) = 0;
};

class ConstraintTimeInterval
{
	public:
		ConstraintTimeInterval (double _from, double _to) { from = _from; to = _to; }
		virtual bool satisfy (double JD);
	private:
		double from;
		double to;
};

class ConstraintTime:public Constraint
{
	public:
		void load (xmlNodePtr cons);
		virtual bool satisfy (Target *tar, double JD);
	private:
		void addInterval (double from, double to) { intervals.push_back (ConstraintTimeInterval (from, to)); }	
		std::list <ConstraintTimeInterval> intervals;
};

class ConstraintDoubleInterval
{
	public:
		ConstraintDoubleInterval (double _lower, double _upper) { lower = _lower; upper = _upper; }
		bool satisfy (double val);
	private:
		double lower;
		double upper;
};

class ConstraintDouble:public Constraint
{
	public:
		void load (xmlNodePtr cons);
	protected:
		virtual bool isBetween (double JD);
	private:
		void addInterval (double lower, double upper) { intervals.push_back (ConstraintDoubleInterval (lower, upper)); };
		std::list <ConstraintDoubleInterval> intervals;
};

class ConstraintAirmass:public ConstraintDouble
{
	public:
		virtual bool satisfy (Target *tar, double JD);
};

class ConstraintHA:public ConstraintDouble
{
	public:
		virtual bool satisfy (Target *tar, double JD);
};

class ConstraintLunarDistance:public ConstraintDouble
{
	public:
		virtual bool satisfy (Target *tar, double JD);
};

class ConstraintLunarPhase:public ConstraintDouble
{
	public:
		virtual bool satisfy (Target *tar, double JD);
};

class ConstraintSolarDistance:public ConstraintDouble
{
	public:
		virtual bool satisfy (Target *tar, double JD);
};

class ConstraintSunAltitude:public ConstraintDouble
{
	public:
		virtual bool satisfy (Target *tar, double JD);
};

class Constraints:public std::vector <Constraint *>
{
	public:
		Constraints () {}

		~Constraints ();
		/**
		 * Check if constrains are satisfied.
		 *
		 * @param tar   target for which constraints will be checked
		 * @param JD    Julian date of constraints check
		 */
		bool satisfy (Target *tar, double JD);

		/**
		 * Return number of violated constainst.
		 *
		 * @param tar   target for which violated constrains will be calculated
		 * @param JD    Julian date of constraints check
		 */
		size_t violated (Target *tar, double JD);

		/**
		 * Load constraints from XML constraint node. Please see constraints.xsd for details.
		 *
		 * @throw XmlError
		 */
		void load (xmlNodePtr _node);

		/**
		 * Load constraints from file.
		 *
		 * @param filename   name of file holding constraints in XML
		 */
		void load (const char *filename);
};

}
