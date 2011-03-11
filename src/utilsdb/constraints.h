/* 
 * Constraints.
 * Copyright (C) 2003-2007 Petr Kubanek <petr@kubanek.net>
 * Copyright (C) 2011      Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
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

#ifndef __RTS2_CONSTRAINTS__
#define __RTS2_CONSTRAINTS__

#include "target.h"
#include "../utils/counted_ptr.h"

#include <libxml/parser.h>
#include <libxml/tree.h>

#include <xmlerror.h>

static const char* CONSTRAINT_TIME         = "time";
static const char* CONSTRAINT_AIRMASS      = "airmass";
static const char* CONSTRAINT_ZENITH_DIST  = "zenithDistance";
static const char* CONSTRAINT_HA           = "HA";
static const char* CONSTRAINT_LDISTANCE    = "lunarDistance";
static const char* CONSTRAINT_LALTITUDE    = "lunarAltitude";
static const char* CONSTRAINT_LPHASE       = "lunarPhase";
static const char* CONSTRAINT_SDISTANCE    = "solarDistance";
static const char* CONSTRAINT_SALTITUDE    = "sunAltitude";
static const char* CONSTRAINT_MAXREPEATS   = "maxRepeats";

namespace rts2db
{

class Target;

/**
 * Abstract class for constraint.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Constraint
{
	public:
		Constraint () {};

		virtual void load (xmlNodePtr cons) = 0;
		virtual bool satisfy (Target *tar, double JD) = 0;

		Constraint *th (){ return this; }

		/**
		 * Add constraint from string.
		 *
		 * @param arg   argument - parsable string specifing constrain parameters
                 */
		virtual void parse (const char *arg) = 0;

		virtual void print (std::ostream &os) = 0;

		virtual const char* getName () = 0;
};

/**
 * Simple interval for constraints. Has lower and upper bounds.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */ 
class ConstraintDoubleInterval
{
	public:
		ConstraintDoubleInterval (const ConstraintDoubleInterval *i) { lower = i->lower; upper = i->upper; }
		ConstraintDoubleInterval (double _lower, double _upper) { lower = _lower; upper = _upper; }
		bool satisfy (double val);
		friend std::ostream & operator << (std::ostream & os, ConstraintDoubleInterval &cons)
		{
			if (!isnan (cons.lower))
				os << cons.lower << " < ";
			if (!isnan (cons.upper))
			  	os << " < " << cons.upper;
			return os;
		}

		/**
		 * Print interval.
		 */
		void print (std::ostream &os);

	private:
		double lower;
		double upper;
};

/**
 * Abstract class for interval (lower/upper) constraints.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class ConstraintInterval: public Constraint
{
	public:
		ConstraintInterval ():Constraint () {};

		/**
		 * Copy constraint intervals.
		 */
		ConstraintInterval (ConstraintInterval &cons);

		virtual void load (xmlNodePtr cons);

		/**
		 * Add interval from string. String containts colon (:) separating various intervals.
		 *
		 * @param arg   colon separated interval boundaries
                 */
		virtual void parse (const char *arg);

		virtual void print (std::ostream &os);

		void copyIntervals (ConstraintInterval *cs)
		{
			for (std::list <ConstraintDoubleInterval>::iterator i = cs->intervals.begin (); i != cs->intervals.end (); i++)
				intervals.push_back (ConstraintDoubleInterval (*i));
		}

		virtual const char* getName () = 0;
	protected:
		void clearIntervals () { intervals.clear (); }
		void add (const ConstraintDoubleInterval &inte) { intervals.push_back (inte); }
		void addInterval (double lower, double upper) { intervals.push_back (ConstraintDoubleInterval (lower, upper)); }
		virtual bool isBetween (double JD);

	private:
		std::list <ConstraintDoubleInterval> intervals;
};

/**
 * Class for time intervals.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class ConstraintTime:public ConstraintInterval
{
	public:
		virtual void load (xmlNodePtr cons);
		virtual bool satisfy (Target *tar, double JD);

		virtual const char* getName () { return CONSTRAINT_TIME; }
};

class ConstraintAirmass:public ConstraintInterval
{
	public:
		virtual bool satisfy (Target *tar, double JD);

		virtual const char* getName () { return CONSTRAINT_AIRMASS; }
};

class ConstraintZenithDistance:public ConstraintInterval
{
	public:
		virtual bool satisfy (Target *tar, double JD);
		virtual const char* getName () { return CONSTRAINT_ZENITH_DIST; }
};

class ConstraintHA:public ConstraintInterval
{
	public:
		virtual bool satisfy (Target *tar, double JD);

		virtual const char* getName () { return CONSTRAINT_HA; }
};

class ConstraintLunarDistance:public ConstraintInterval
{
	public:
		virtual bool satisfy (Target *tar, double JD);

		virtual const char* getName () { return CONSTRAINT_LDISTANCE; }
};

class ConstraintLunarAltitude:public ConstraintInterval
{
	public:
		virtual bool satisfy (Target *tar, double JD);

		virtual const char* getName () { return CONSTRAINT_LALTITUDE; }
};

class ConstraintLunarPhase:public ConstraintInterval
{
	public:
		virtual bool satisfy (Target *tar, double JD);

		virtual const char* getName () { return CONSTRAINT_LPHASE; }
};

class ConstraintSolarDistance:public ConstraintInterval
{
	public:
		virtual bool satisfy (Target *tar, double JD);

		virtual const char* getName () { return CONSTRAINT_SDISTANCE; }
};

class ConstraintSunAltitude:public ConstraintInterval
{
	public:
		virtual bool satisfy (Target *tar, double JD);

		virtual const char* getName () { return CONSTRAINT_SALTITUDE; }
};

class ConstraintMaxRepeat:public Constraint
{
	public:
		ConstraintMaxRepeat ():Constraint () { maxRepeat = -1; }
		virtual void load (xmlNodePtr cons);
		virtual bool satisfy (Target *tar, double JD);

		virtual void parse (const char *arg);

		virtual void print (std::ostream &os);

		virtual const char* getName () { return CONSTRAINT_MAXREPEATS; }

		void copyConstraint (ConstraintMaxRepeat *i) { maxRepeat = i->maxRepeat; }
	private:
		int maxRepeat;
};

typedef counted_ptr <Constraint> ConstraintPtr;

/**
 * List of constraints.
 *
 * @author Petr Kubanek <kubanek@fzu.cz>
 */
class ConstraintsList:public std::list <ConstraintPtr>
{
	public:
		ConstraintsList () {};

		friend std::ostream & operator << (std::ostream &os, ConstraintsList clist)
		{
			for (ConstraintsList::iterator citer = clist.begin (); citer != clist.end (); citer++)
			{
				if (citer != clist.begin ())
					os << " ";
				os << (*citer)->getName ();
			}
			return os;
		}
		
		void printJSON (std::ostream &os);
};

class Constraints:public std::map <std::string, ConstraintPtr >
{
	public:
		Constraints () {}

		/**
		 * Copy constructor. Creates new constraint members, so if they are changed in copy, they do not affect master.
		 */
		Constraints (Constraints &cs);

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
		 * @param tar       target for which violated constrains will be calculated
		 * @param JD        Julian date of constraints check
		 * @param violated  list of the violated constraints
		 */
		size_t getViolated (Target *tar, double JD, ConstraintsList &violated);

		/**
		 * Return number of satisfied constainst.
		 *
		 * @param tar       target for which violated constrains will be calculated
		 * @param JD        Julian date of constraints check
		 * @param satisfied list of the satisifed constraints
		 */
		size_t getSatisfied (Target *tar, double JD, ConstraintsList &satisfied);

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

		/**
		 * Parse constraint from arguments.
		 *
		 * @param name        constraint name
		 * @param arg         constraint argument, specifiing constraint parameters
		 */
		void parse (const char *name, const char *arg);

		/**
		 * Print constraints.
		 */
		void print (std::ostream &os);

	private:
		Constraint *createConstraint (const char *name);
};

/**
 * Master constraint singleton.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class MasterConstraints
{
  	public:
		static Constraints & getConstraint ();
};

}

#endif /* ! __RTS2_CONSTRAINTS__ */
