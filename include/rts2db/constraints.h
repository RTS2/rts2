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

#include "connnotify.h"
#include "target.h"

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

		bool isInvalid () { return isnan (lower) && isnan (upper); }

		/**
		 * Print interval.
		 */
		void printXML (std::ostream &os);
		void printJSON (std::ostream &os);

		double getLower () { return lower; }
		double getUpper () { return upper; }

	private:
		double lower;
		double upper;
};

/**
 * Abstract class for constraint.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Constraint
{
	public:
		Constraint () {}
		virtual ~Constraint () {}

		virtual void load (xmlNodePtr cons) = 0;
		/**
		 * Check if constraint is satisfied at given time.
		 *
		 * @param tar  target which is checked for constraint
		 * @param JD   date (Julian Day) checked
		 * @param nextJD  returned hint about next time a change in satisfy/violated might occur; nan if the hint is not available, 0 if it cannot be computed
		 *
		 * @return true if constraint is satisfied
		 *
		 */
		virtual bool satisfy (Target *tar, double JD, double *nextJD) = 0;

		Constraint *th () { return this; }

		/**
		 * Add constraint from string.
		 *
		 * @param arg   argument - parsable string specifing constrain parameters
                 */
		virtual void parse (const char *arg) = 0;

		virtual bool isInvalid () = 0;

		virtual void printXML (std::ostream &os) = 0;
		virtual void printJSON (std::ostream &os) = 0;

		virtual const char* getName () = 0;

		/**
		 * Return array with intervals when constraint for given target is satisfied.
		 *
		 * @param tar
		 * @param from
		 * @param to
		 * @param step
		 * @param ret   returned array of violated time intervals
		 */
		virtual void getSatisfiedIntervals (Target *tar, time_t from, time_t to, int step, interval_arr_t &ret);

		/**
		 * Return array with intervals when constraint for given target is violated.
		 *
		 * @param tar
		 * @param from
		 * @param to
		 * @param step
		 * @param ret   returned array of violated time intervals
		 */
		void getViolatedIntervals (Target *tar, time_t from, time_t to, int step, interval_arr_t &ret);

		/**
		 * Return list of altitude intervals. Usefull only for
		 * altitude-based constraints - e.g. airmass and zenith
		 * distance.
		 */
		virtual void getAltitudeIntervals (std::vector <ConstraintDoubleInterval> &ac) { throw rts2core::Error ("getAltitudeIntervals is not supported"); }

		void getAltitudeViolatedIntervals (std::vector <ConstraintDoubleInterval> &ac);
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

		virtual bool isInvalid ();

		virtual void printXML (std::ostream &os);
		virtual void printJSON (std::ostream &os);

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
		virtual bool satisfy (Target *tar, double JD, double *nextJD);

		virtual const char* getName () { return CONSTRAINT_TIME; }

		virtual void getSatisfiedIntervals (Target *tar, time_t from, time_t to, int step, interval_arr_t &ret);
};

class ConstraintAirmass:public ConstraintInterval
{
	public:
		virtual bool satisfy (Target *tar, double JD, double *nextJD);

		virtual const char* getName () { return CONSTRAINT_AIRMASS; }

		virtual void getAltitudeIntervals (std::vector <ConstraintDoubleInterval> &ac);
};

class ConstraintZenithDistance:public ConstraintInterval
{
	public:
		virtual bool satisfy (Target *tar, double JD, double *nextJD);

		virtual const char* getName () { return CONSTRAINT_ZENITH_DIST; }

		virtual void getAltitudeIntervals (std::vector <ConstraintDoubleInterval> &ac);
};

class ConstraintHA:public ConstraintInterval
{
	public:
		virtual bool satisfy (Target *tar, double JD, double *nextJD);

		virtual const char* getName () { return CONSTRAINT_HA; }
};

class ConstraintLunarDistance:public ConstraintInterval
{
	public:
		virtual bool satisfy (Target *tar, double JD, double *nextJD);

		virtual const char* getName () { return CONSTRAINT_LDISTANCE; }

		virtual void getSatisfiedIntervals (Target *tar, time_t from, time_t to, int step, interval_arr_t &ret);
};

class ConstraintLunarAltitude:public ConstraintInterval
{
	public:
		virtual bool satisfy (Target *tar, double JD, double *nextJD);

		virtual const char* getName () { return CONSTRAINT_LALTITUDE; }
};

class ConstraintLunarPhase:public ConstraintInterval
{
	public:
		virtual bool satisfy (Target *tar, double JD, double *nextJD);

		virtual const char* getName () { return CONSTRAINT_LPHASE; }
};

class ConstraintSolarDistance:public ConstraintInterval
{
	public:
		virtual bool satisfy (Target *tar, double JD, double *nextJD);

		virtual const char* getName () { return CONSTRAINT_SDISTANCE; }
};

class ConstraintSunAltitude:public ConstraintInterval
{
	public:
		virtual bool satisfy (Target *tar, double JD, double *nextJD);

		virtual const char* getName () { return CONSTRAINT_SALTITUDE; }
};

class ConstraintMaxRepeat:public Constraint
{
	public:
		ConstraintMaxRepeat ():Constraint () { maxRepeat = -1; }
		virtual void load (xmlNodePtr cons);
		virtual bool satisfy (Target *tar, double JD, double *nextJD);

		virtual void parse (const char *arg);

		virtual bool isInvalid () { return maxRepeat <= 0; }

		virtual void printXML (std::ostream &os);
		virtual void printJSON (std::ostream &os);

		virtual const char* getName () { return CONSTRAINT_MAXREPEATS; }

		void copyConstraint (ConstraintMaxRepeat *i) { maxRepeat = i->maxRepeat; }

		virtual void getSatisfiedIntervals (Target *tar, time_t from, time_t to, int step, interval_arr_t &ret);
	private:
		int maxRepeat;
};

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

		std::string toString ()
		{
			std::ostringstream os;
			os << (*this);
			return os.str ();
		}
};

/**
 * Set of constraints.
 *
 * @author Petr Kubanek <kubanek@fzu.cz>
 */
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
		 * Return list of constaints which bound satisfied altitude constraints.
		 */
		size_t getAltitudeConstraints (std::map <std::string, std::vector <ConstraintDoubleInterval> > &ac);

		/**
		 * Return list of constaints which bound violated altitude constraints.
		 */
		size_t getAltitudeViolatedConstraints (std::map <std::string, std::vector <ConstraintDoubleInterval> > &ac);


		size_t getTimeConstraints (std::map <std::string, ConstraintPtr> &cons);

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
		 * Return number of satisfied intervals.
		 *
		 * @param tar       target for which violated constrains will be calculated
		 * @param JD        Julian date of constraints check
		 * @param satisfied list of the satisifed constraints
		 */
		size_t getSatisfied (Target *tar, double JD, ConstraintsList &satisfied);

		/**
		 * Get intervals satisfing conditions in given time range.
		 *
		 * @param tar       target for which satisfied constraints will be calculated
		 * @param from      JD from which constrainst will be checked
		 * @param to        JD to which constraints will be checked
		 * @param length    length (in seconds) of interval which should be checked
		 * @param step      step to take when verifing constraints (in seconds)
		 * @param satisfiedIntervals  pair of double values (JD from - to) of satisfied constraints
		 */
		void getSatisfiedIntervals (Target *tar, time_t from, time_t to, int length, int step, interval_arr_t &satisfiedIntervals);

		void getViolatedIntervals (Target *tar, time_t from, time_t to, int length, int step, interval_arr_t &violatedIntervals);

		/**
		 * Return time until when constraints are satisfied.
		 *
		 * @return   time when constraints will not be satisfied
		 */
		double getSatisfiedDuration (Target *tar, double from, double to, double length, double step);

		/**
		 * Load constraints from XML constraint node. Please see constraints.xsd for details.
		 *
		 * @throw XmlError
		 */
		void load (xmlNodePtr _node, bool overwrite);

		/**
		 * Load constraints from file.
		 *
		 * @param filename   name of file holding constraints in XML
		 * @param overwrite  if existing constraints should be overwriten
		 */
		void load (const char *filename, bool overwrite = true);

		/**
		 * Parse constraint from arguments.
		 *
		 * @param name        constraint name
		 * @param arg         constraint argument, specifiing constraint parameters
		 */
		void parse (const char *name, const char *arg);

		/**
		 * Remove invalid constraints. Useful to remove constraints from constraint file.
		 */
		void removeInvalid ();

		/**
		 * Print constraints.
		 */
		void printXML (std::ostream &os);
		void printJSON (std::ostream &os);

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
		~MasterConstraints ();

		static Constraints & getConstraint ();

		static Constraints * getTargetConstraints (int tar_id);
		static void setTargetConstraints (int tar_id, Constraints * _constraints);

		static void setNotifyConnection (rts2core::ConnNotify *_watchConn);
		static rts2core::ConnNotify *getNotifyConnection ();

		static void clearCache ();
};

}

#endif /* ! __RTS2_CONSTRAINTS__ */
