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

#include "rts2db/constraints.h"
#include "utilsfunc.h"
#include "configuration.h"

#ifndef HAVE_DECL_LN_GET_ALT_FROM_AIRMASS
double ln_get_alt_from_airmass (double X, double airmass_scale)
{
	return ln_rad_to_deg (asin ((2 * airmass_scale + 1 - X * X) / (2 * X * airmass_scale)));
}
#endif // ! HAVE_DECL_LN_GET_ALT_FROM_AIRMASS

bool between (double val, double low, double upper)
{
	if (isnan (val))
		return false;
	if (isnan (low) && isnan (upper))
		return true;
	if (isnan (low))
		return val < upper;
	if (isnan (upper))
		return val >= low;
	return val >= low && val < upper;
}

using namespace rts2db;

bool ConstraintDoubleInterval::satisfy (double val)
{
	return between (val, lower, upper);
}

void ConstraintDoubleInterval::printXML (std::ostream &os)
{
	os << "    <interval>";
	if (!isnan (lower))
		os << std::endl << "      <lower>" << lower << "</lower>";
	if (!isnan (upper))
		os << std::endl << "      <upper>" << upper << "</upper>";
	os << std::endl << "    </interval>" << std::endl;
}

void ConstraintDoubleInterval::printJSON (std::ostream &os)
{
	os << "[";
	if (!isnan (lower))
		os << lower << ",";
	else
		os << "null,";
	if (!isnan (upper))
		os << upper;
	else
		os << "null";
	os << "]";
}



ConstraintInterval::ConstraintInterval (ConstraintInterval &cons)
{
	for (std::list <ConstraintDoubleInterval>::iterator iter = cons.intervals.begin (); iter != cons.intervals.end (); iter++)
		add (*iter);
}

void ConstraintInterval::load (xmlNodePtr cons)
{
	intervals.clear ();
	for (xmlNodePtr inter = cons->children; inter != NULL; inter = inter->next)
	{
		if (xmlStrEqual (inter->name, (xmlChar *) "interval"))
		{
			double lower = NAN;
			double upper = NAN;

			for (xmlNodePtr par = inter->children; par != NULL; par = par->next)
			{
				if (xmlStrEqual (par->name, (xmlChar *) "lower"))
					lower = atof ((const char *) par->children->content);
				else if (xmlStrEqual (par->name, (xmlChar *) "upper"))
					upper = atof ((const char *) par->children->content);
				else if (par->type == XML_COMMENT_NODE)
				  	continue;
				else
					throw XmlUnexpectedNode (par);
			}
			addInterval (lower, upper);
		}
		else if (inter->type == XML_COMMENT_NODE)
		{
			continue;
		}
		else
		{
			throw XmlUnexpectedNode (inter);
		}
	}
}

void ConstraintInterval::parse (const char *arg)
{
	double lower = NAN;
	double upper = NAN;

	char *sint = new char [strlen (arg) + 1];
	strcpy (sint, arg);

	char *cp = strchr (sint, ':');
	if (cp == NULL)
		throw rts2core::Error ((std::string ("the interval must contain a single : - cannot find : in interval ") + arg).c_str ());

	char *endp;
	*cp = '\0';

	if (cp != sint)
	{
#ifdef HAVE_STRTOF
		lower = strtof (sint, &endp);
#else
		lower = strtod (sint, &endp);
#endif
		if (*endp != 0)
			throw rts2core::Error ((std::string ("cannot parse lower intrval - ") + arg).c_str ());
	}
	
	if (*(cp + 1) != '\0')
	{
#ifdef HAVE_STRTOF
		upper = strtof (cp + 1, &endp);
#else
		upper = strtod (cp + 1, &endp);
#endif		
		if (*endp != 0)
			throw rts2core::Error ((std::string ("cannot find : in interval ") + (cp + 1)).c_str ());
	}

	addInterval (lower, upper);

	delete[] sint;
}

bool ConstraintInterval::isInvalid ()
{
	for (std::list <ConstraintDoubleInterval>::iterator iter = intervals.begin (); iter != intervals.end (); iter++)
	{
		if (iter->isInvalid ())
			return true;		
	}
	return false;
}

void ConstraintInterval::printXML (std::ostream &os)
{
	os << "  <" << getName () << ">" << std::endl;
	for (std::list <ConstraintDoubleInterval>::iterator iter = intervals.begin (); iter != intervals.end (); iter++)
	{
		iter->printXML (os);
	}
	os << "  </" << getName () << ">" << std::endl;
}

void ConstraintInterval::printJSON (std::ostream &os)
{
	os << "\"" << getName () << "\":[";
	for (std::list <ConstraintDoubleInterval>::iterator iter = intervals.begin (); iter != intervals.end (); iter++)
	{
		if (iter != intervals.begin ())
			os << ",";
		iter->printJSON (os);
	}
	os << "]";
}

bool ConstraintInterval::isBetween (double val)
{
	for (std::list <ConstraintDoubleInterval>::iterator iter = intervals.begin (); iter != intervals.end (); iter++)
	{
		if (iter->satisfy (val))
			return true;
	}
	return false;
}

// interval functions

// reverse intervals. Intervals must be ordered
void reverseInterval (time_t from, time_t to, interval_arr_t &intervals)
{
	interval_arr_t ret;
	if (intervals.size ())
	{
		interval_arr_t::iterator iter = intervals.begin ();
		time_t t = from;
		
		for (; iter != intervals.end (); iter++)
		{
			if (t < iter->first)
				ret.push_back (std::pair <time_t, time_t> (t, iter->first));
			t = iter->second;
		}
		iter--;
		if (iter->second < to)
			ret.push_back (std::pair <time_t, time_t> (iter->second, to));
	}
	else
	{
		ret.push_back (std::pair <time_t, time_t> (from, to));
	}
	intervals = ret;
}

// find first satisifing interval ending after first violation..
void findFirst (interval_arr_t::const_iterator &si, const interval_arr_t::const_iterator &end, double t)
{
	while (si->second < t && si != end)
		si++;
}

#define min(a,b) ((a < b) ? a : b)
#define max(a,b) ((a > b) ? a : b)

// merge two intervals, find their intersection
void mergeIntervals (const interval_arr_t master, const interval_arr_t &add, interval_arr_t &ret)
{
	if (add.size () == 0 || master.size () == 0)
	{
		ret.clear ();
		return;
	}
	interval_arr_t::const_iterator addi = add.begin ();
	for (interval_arr_t::const_iterator mi = master.begin (); mi != master.end (); mi++)
	{
		while (addi->first < mi->second)
		{
			findFirst (addi, add.end (), mi->first);
			if (addi == add.end ())
				break;
			// intervals have empty conjunction
			if (addi->first > mi->second)
				continue;
			ret.push_back (std::pair <time_t, time_t> (max (mi->first, addi->first), min (mi->second, addi->second)));
			addi++;
		}
	}
}

void Constraint::getSatisfiedIntervals (Target *tar, time_t from, time_t to, int step, interval_arr_t &ret)
{
	double vf = NAN;

	double to_JD = ln_get_julian_from_timet (&to);

	double t;
	for (t = ln_get_julian_from_timet (&from); t < to_JD; t += step / 86400.0)
	{
		if (satisfy (tar, t))
		{
			if (isnan (vf))
				vf = t;
		}
		else if (!isnan (vf))
		{
			ln_get_timet_from_julian (vf, &from);
			ln_get_timet_from_julian (t, &to);
			ret.push_back (std::pair <time_t, time_t> (from, to));
			vf = NAN;
		}
	}
	if (!isnan (vf))
	{
		ln_get_timet_from_julian (vf, &from);
		ln_get_timet_from_julian (t, &to);
		ret.push_back (std::pair <time_t, time_t> (from, to));
	}
}

void Constraint::getViolatedIntervals (Target *tar, time_t from, time_t to, int step, interval_arr_t &ret)
{
	getSatisfiedIntervals (tar, from, to, step, ret);
	reverseInterval (from, to, ret);
}

void Constraint::getAltitudeViolatedIntervals (std::vector <ConstraintDoubleInterval> &ac)
{
	std::vector <ConstraintDoubleInterval> si;
	getAltitudeIntervals (si);
	for (double alt = 90; alt > -90; )
	{
		if (si.size () == 0)
		{
			if (ac.size () > 0 || alt < 90)
				ac.push_back (ConstraintDoubleInterval (alt, -90));
			alt = -90;
		}
		else
		{
			std::vector <ConstraintDoubleInterval>::iterator highest = si.begin ();
			double ah = highest->getUpper ();
			if (isnan (ah))
				ah = -90;
			// find highest altitude..
			for (std::vector <ConstraintDoubleInterval>::iterator iter = highest + 1; iter != si.end (); iter++)
			{
				if (!isnan (iter->getUpper ()) && iter->getUpper () > ah)
				{
					highest = iter;
					ah = iter->getUpper ();
				}
				else if (isnan (iter->getUpper ()) && !isnan (iter->getLower ()) && iter->getLower () > ah)
				{
					highest = iter;
					ah = iter->getLower ();
				}
			}
			// decide which part is violated..
			if (!isnan (highest->getUpper ()) && ah < alt)
			{
				ac.push_back (ConstraintDoubleInterval (alt, ah));
			}
			if (!isnan (highest->getLower ()))
				alt = highest->getLower ();
			else
				alt = -90;
			si.erase (highest);
		}
	}
}

void ConstraintTime::load (xmlNodePtr cons)
{
	clearIntervals ();
	for (xmlNodePtr inter = cons->children; inter != NULL; inter = inter->next)
	{
		if (xmlStrEqual (inter->name, (xmlChar *) "interval"))
		{
			double from = NAN;
			double to = NAN;

			for (xmlNodePtr par = inter->children; par != NULL; par = par->next)
			{
				if (xmlStrEqual (par->name, (xmlChar *) "from"))
					parseDate ((const char *) par->children->content, from);
				else if (xmlStrEqual (par->name, (xmlChar *) "to"))
					parseDate ((const char *) par->children->content, to);
				else if (par->type == XML_COMMENT_NODE)
					continue;
				else
					throw XmlUnexpectedNode (par);
			}
			addInterval (from, to);
		}
		else if (inter->type == XML_COMMENT_NODE)
		{
			continue;
		}
		else
		{
			throw XmlUnexpectedNode (inter);
		}
	}
}

bool ConstraintTime::satisfy (Target *target, double JD)
{
	return isBetween (JD);
}

void ConstraintTime::getSatisfiedIntervals (Target *tar, time_t from, time_t to, int step, interval_arr_t &ret)
{
	// get list of satisfied intervals
	for (std::list <ConstraintDoubleInterval>::iterator iter = intervals.begin (); iter != intervals.end (); iter++)
	{
		double l = iter->getUpper ();
		double u = iter->getLower ();
		if (!isnan (l) && l >= from && !isnan (u) && u <= to)
			ret.push_back (std::pair <time_t, time_t> (l, u));
		if (isnan (l) && !isnan (u) && u <= to)
			ret.push_back (std::pair <time_t, time_t> (from, u));
		if (!isnan (l) && l >= from  && isnan (u))
			ret.push_back (std::pair <time_t, time_t> (l, to));
	}
}

bool ConstraintAirmass::satisfy (Target *tar, double JD)
{
	double am = tar->getAirmass (JD);
	if (isnan (am))
		return true;
	return isBetween (am);
}

void ConstraintAirmass::getAltitudeIntervals (std::vector <ConstraintDoubleInterval> &ac)
{
	for (std::list <ConstraintDoubleInterval>::iterator iter = intervals.begin (); iter != intervals.end (); iter++)
	{
		double l = iter->getUpper ();
		double u = iter->getLower ();
		if (!isnan (l))
			l = ln_get_alt_from_airmass (l, 750.0);
		if (!isnan (u))
			u = ln_get_alt_from_airmass (u, 750.0);
		ac.push_back (ConstraintDoubleInterval (l, u));
	}
}

bool ConstraintZenithDistance::satisfy (Target *tar, double JD)
{
	double zd = tar->getZenitDistance (JD);
	if (isnan (zd))
		return true;
	return isBetween(zd);
}

void ConstraintZenithDistance::getAltitudeIntervals (std::vector <ConstraintDoubleInterval> &ac)
{
	for (std::list <ConstraintDoubleInterval>::iterator iter = intervals.begin (); iter != intervals.end (); iter++)
	{
		double l = iter->getUpper ();
		double u = iter->getLower ();
		if (!isnan (l))
			l = 90 - l;
		if (!isnan (u))
			u = 90 - u;
		ac.push_back (ConstraintDoubleInterval (l, u));
	}
}

bool ConstraintHA::satisfy (Target *tar, double JD)
{
	double ha = tar->getHourAngle (JD);
	if (isnan (ha))
		return true;
	return isBetween (ha);
}

bool ConstraintLunarDistance::satisfy (Target *tar, double JD)
{
	double ld = tar->getLunarDistance (JD);
	if (isnan (ld))
		return true;
	return isBetween (ld);
}

bool ConstraintLunarAltitude::satisfy (Target *tar, double JD)
{
	struct ln_equ_posn eq_lun;
	struct ln_hrz_posn hrz_lun;
	ln_get_lunar_equ_coords (JD, &eq_lun);
	ln_get_hrz_from_equ (&eq_lun, rts2core::Configuration::instance ()->getObserver (), JD, &hrz_lun);
	return isBetween (hrz_lun.alt);
}

bool ConstraintLunarPhase::satisfy (Target *tar, double JD)
{
	return isBetween (ln_get_lunar_phase (JD));
}

bool ConstraintSolarDistance::satisfy (Target *tar, double JD)
{
	double sd = tar->getSolarDistance (JD);
	if (isnan (sd))
		return true;
	return isBetween (sd);
}

bool ConstraintSunAltitude::satisfy (Target *tar, double JD)
{
	struct ln_equ_posn eq_sun;
	struct ln_hrz_posn hrz_sun;
	ln_get_solar_equ_coords (JD, &eq_sun);
	ln_get_hrz_from_equ (&eq_sun, rts2core::Configuration::instance ()->getObserver (), JD, &hrz_sun);
	return isBetween (hrz_sun.alt);
}

void ConstraintMaxRepeat::load (xmlNodePtr cons)
{
	if (!cons->children || !cons->children->content)
		throw XmlUnexpectedNode (cons);
	maxRepeat = atoi ((const char *) cons->children->content);
}

bool ConstraintMaxRepeat::satisfy (Target *tar, double JD)
{
	if (maxRepeat > 0)
		return tar->getTotalNumberOfObservations () < maxRepeat;
	return true;
}

void ConstraintMaxRepeat::parse (const char *arg)
{
	maxRepeat = atoi (arg);
}

void ConstraintMaxRepeat::printXML (std::ostream &os)
{
	if (maxRepeat > 0)
		os << "  <" << getName () << ">" << maxRepeat << "</" << getName () << ">" << std::endl;
}

void ConstraintMaxRepeat::printJSON (std::ostream &os)
{
	os << "\"" << getName () << "\":" << maxRepeat;
}

void ConstraintMaxRepeat::getSatisfiedIntervals (Target *tar, time_t from, time_t to, int step, interval_arr_t &ret)
{
	if (maxRepeat <= 0 || tar->getTotalNumberOfObservations () < maxRepeat)
		ret.push_back ( std::pair <time_t, time_t> (from, to) );
}


Constraints::Constraints (Constraints &cs): std::map <std::string, ConstraintPtr > (cs)
{
	for (Constraints::iterator iter = cs.begin (); iter != cs.end (); iter++)
	{
		Constraint *con = createConstraint (iter->first.c_str ());
		// can "compare" chars, as they are const char*
		if (con->getName () == CONSTRAINT_TIME
			|| con->getName () == CONSTRAINT_AIRMASS
			|| con->getName () == CONSTRAINT_ZENITH_DIST
			|| con->getName () == CONSTRAINT_HA
			|| con->getName () == CONSTRAINT_LDISTANCE
			|| con->getName () == CONSTRAINT_LALTITUDE
			|| con->getName () == CONSTRAINT_LPHASE
			|| con->getName () == CONSTRAINT_SDISTANCE
			|| con->getName () == CONSTRAINT_SALTITUDE)
		{
			((ConstraintInterval *) con)->copyIntervals (((ConstraintInterval *) iter->second->th ()));
		}
		else if (con->getName () == CONSTRAINT_MAXREPEATS)
		{
			((ConstraintMaxRepeat *) con)->copyConstraint (((ConstraintMaxRepeat *) iter->second->th ()));
		}
		else
		{
			std::cerr << "unsuported constraint type in copy constructor " __FILE__ ":" << __LINE__ << std::endl;
			exit (10);
		}

		(*this)[iter->first] = ConstraintPtr (con);
	}
}

Constraints::~Constraints ()
{
	for (Constraints::iterator iter = begin (); iter != end (); iter++)
		iter->second.null ();
	clear ();
}

size_t Constraints::getAltitudeConstraints (std::map <std::string, std::vector <ConstraintDoubleInterval> > &ac)
{
	size_t i = 0;
	for (Constraints::iterator iter = begin (); iter != end (); iter++)
	{
		if (!strcmp (iter->second->getName (), CONSTRAINT_AIRMASS) || !strcmp (iter->second->getName (), CONSTRAINT_ZENITH_DIST))
		{
			std::vector <ConstraintDoubleInterval> vv;

			iter->second->getAltitudeIntervals (vv);
			ac[std::string (iter->second->getName ())] = vv; 
			
			i++;
		}
	}
	return i;
}

size_t Constraints::getAltitudeViolatedConstraints (std::map <std::string, std::vector <ConstraintDoubleInterval> > &ac)
{
	size_t i = 0;
	for (Constraints::iterator iter = begin (); iter != end (); iter++)
	{
		if (!strcmp (iter->second->getName (), CONSTRAINT_AIRMASS) || !strcmp (iter->second->getName (), CONSTRAINT_ZENITH_DIST))
		{
			std::vector <ConstraintDoubleInterval> vv;

			iter->second->getAltitudeViolatedIntervals (vv);
			ac[std::string (iter->second->getName ())] = vv; 
			
			i++;
		}
	}
	return i;
}

size_t Constraints::getTimeConstraints (std::map <std::string, ConstraintPtr> &cons)
{
	size_t i = 0;
	for (Constraints::iterator iter = begin (); iter != end (); iter++)
	{
		if (!strcmp (iter->second->getName (), CONSTRAINT_HA)
			|| !strcmp (iter->second->getName (), CONSTRAINT_TIME)
			|| !strcmp (iter->second->getName (), CONSTRAINT_LDISTANCE)
			|| !strcmp (iter->second->getName (), CONSTRAINT_LALTITUDE)
			|| !strcmp (iter->second->getName (), CONSTRAINT_LPHASE)
			|| !strcmp (iter->second->getName (), CONSTRAINT_SDISTANCE)
			|| !strcmp (iter->second->getName (), CONSTRAINT_SALTITUDE)
			|| !strcmp (iter->second->getName (), CONSTRAINT_MAXREPEATS)
		)
		{
			cons[iter->second->getName ()] = iter->second;
			
			i++;
		}
	}
	return i;
}

bool Constraints::satisfy (Target *tar, double JD)
{
	for (Constraints::iterator iter = begin (); iter != end (); iter++)
	{
		if (!(iter->second->satisfy (tar, JD)))
			return false;
	}
	return true;
}

size_t Constraints::getViolated (Target *tar, double JD, ConstraintsList &violated)
{
	for (Constraints::iterator iter = begin (); iter != end (); iter++)
	{
		if (!(iter->second->satisfy (tar, JD)))
			violated.push_back (iter->second);
	}
	return violated.size ();
}

size_t Constraints::getSatisfied (Target *tar, double JD, ConstraintsList &satisfied)
{
	for (Constraints::iterator iter = begin (); iter != end (); iter++)
	{
		if (iter->second->satisfy (tar, JD))
			satisfied.push_back (iter->second);
	}
	return satisfied.size ();
}

void Constraints::getSatisfiedIntervals (Target *tar, time_t from, time_t to, int length, int step, interval_arr_t &satisfiedIntervals)
{
	satisfiedIntervals.clear ();
	satisfiedIntervals.push_back (std::pair <time_t, time_t> (from, to));
	for (Constraints::iterator iter = begin (); iter != end (); iter++)
	{
		interval_arr_t intervals;
		iter->second->getSatisfiedIntervals (tar, from, to, step, intervals);
		// now look for join with current intervals..
		interval_arr_t ret = satisfiedIntervals;
		satisfiedIntervals.clear ();
		mergeIntervals (ret, intervals, satisfiedIntervals);
	}
}

double Constraints::getSatisfiedDuration (Target *tar, double from, double to, double length, double step)
{
	time_t fti = (time_t) to;

	double to_JD = ln_get_julian_from_timet (&fti);
	fti = (time_t) (from + length);
	
	double t;
	double from_JD = ln_get_julian_from_timet (&fti);
	for (t = from_JD; t < to_JD; t += step / 86400.0)
	{
		if (!satisfy (tar, t))
		{
			if (t == from_JD)
			{
				return NAN;
			}
			time_t ret;
		  	ln_get_timet_from_julian (t, &ret);
			return ret;
		}
	}
	return INFINITY;
}

void Constraints::getViolatedIntervals (Target *tar, time_t from, time_t to, int length, int step, interval_arr_t &violatedIntervals)
{
	getSatisfiedIntervals (tar, from, to, length, step, violatedIntervals);
	reverseInterval (from, to, violatedIntervals);
}

void Constraints::load (xmlNodePtr _node, bool overwrite)
{
	for (xmlNodePtr cons = _node->children; cons != NULL; cons = cons->next)
	{
		if (cons->type == XML_COMMENT_NODE)
			continue;
	  	ConstraintPtr con;
		Constraints::iterator candidate = find (std::string ((const char *) cons->name));
		// found existing constrain - if commanded to not overwrite, do not overwrite it
		if (candidate != end ())
		{
			if (overwrite == false)
				continue;
			con = candidate->second;
		}
		else 
		{
			Constraint *cp = createConstraint ((const char *) cons->name);
			if (cp == NULL)
				throw XmlUnexpectedNode (cons);
			con = ConstraintPtr (cp);
		}
		try
		{
			con->load (cons);
		}
		catch (XmlError er)
		{
			con.null ();
			throw er;
		}
		if (candidate == end ())
		{
			(*this)[std::string (con->getName ())] = con;
		}
	}
}

void Constraints::load (const char *filename, bool overwrite)
{
	LIBXML_TEST_VERSION

	xmlLineNumbersDefault (1);
	xmlDoc *doc = xmlReadFile (filename, NULL, XML_PARSE_NOBLANKS | XML_PARSE_NOWARNING);
	if (doc == NULL)
		throw XmlError ("cannot parse constraint file " + std::string (filename));

	xmlNodePtr root_element = xmlDocGetRootElement (doc);
	if (!xmlStrEqual (root_element->name, (xmlChar *) "constraints"))
		throw XmlUnexpectedNode (root_element);

	load (root_element, overwrite);
	xmlFreeDoc (doc);
	xmlCleanupParser ();
}

void Constraints::parse (const char *name, const char *arg)
{
	Constraints::iterator iter = find(std::string (name));
	if (iter != end ())
	{
		iter->second->parse (arg);
	}
	else
	{
		Constraint *con = createConstraint (name);
		if (con == NULL)
			throw rts2core::Error ((std::string ("cannot allocate constraint with name ") + name).c_str ());
		con->parse (arg);
		(*this)[std::string (con->getName ())] = ConstraintPtr (con);
	}
}

void Constraints::removeInvalid ()
{
	for (Constraints::iterator iter = begin (); iter != end ();)
	{
		if (iter->second->isInvalid ())
		{
			Constraints::iterator toerase = iter;
			iter++;
			erase (toerase);
		}
		else
		{
			iter++;
		}
	}
}

void Constraints::printXML (std::ostream &os)
{
	os << "<constraints>" << std::endl;
	for (Constraints::iterator iter = begin (); iter != end (); iter++)
	{
		iter->second->printXML (os);
	}
	os << "</constraints>" << std::endl;
}

void Constraints::printJSON (std::ostream &os)
{
	for (Constraints::iterator iter = begin (); iter != end (); iter++)
	{
		if (iter != begin ())
			os << ",";
		iter->second->printJSON (os);
	}
}

Constraint *Constraints::createConstraint (const char *name)
{
	if (!strcmp (name, CONSTRAINT_TIME))
		return new ConstraintTime ();
	else if (!strcmp (name, CONSTRAINT_AIRMASS))
		return new ConstraintAirmass ();
	else if (!strcmp (name, CONSTRAINT_ZENITH_DIST))
		return new ConstraintZenithDistance ();
	else if (!strcmp (name, CONSTRAINT_HA))
		return new ConstraintHA ();
	else if (!strcmp (name, CONSTRAINT_LDISTANCE))
		return new ConstraintLunarDistance ();
	else if (!strcmp (name, CONSTRAINT_LALTITUDE))
		return new ConstraintLunarDistance ();
	else if (!strcmp (name, CONSTRAINT_LPHASE))
		return new ConstraintLunarPhase ();
	else if (!strcmp (name, CONSTRAINT_SDISTANCE))
		return new ConstraintSolarDistance ();
	else if (!strcmp (name, CONSTRAINT_SALTITUDE))
		return new ConstraintSunAltitude ();
	else if (!strcmp (name, CONSTRAINT_MAXREPEATS))
		return new ConstraintMaxRepeat ();
	return NULL;
}

static Constraints *masterCons = NULL;

// target->constraint hash
static std::map <int, Constraints *> constraintsCache;

void ConstraintsList::printJSON (std::ostream &os)
{
	os << "[";
	for (ConstraintsList::iterator iter = begin (); iter != end (); iter++)
	{
		if (iter != begin ())
			os << ",";
		os << '"' << (*iter)->getName() << '"';
	}
	os << "]";
}

MasterConstraints::~MasterConstraints ()
{
	for (std::map <int, Constraints *>::iterator ci = constraintsCache.begin (); ci != constraintsCache.end (); ci++)
		delete ci->second;
	constraintsCache.clear ();	
}

Constraints & MasterConstraints::getConstraint ()
{
	if (masterCons)
		return *masterCons;
	masterCons = new Constraints ();
	masterCons->load (rts2core::Configuration::instance ()->getMasterConstraintFile ());
	return *masterCons;
}

Constraints * MasterConstraints::getTargetConstraints (int tar_id)
{
	std::map <int, Constraints *>::iterator ci = constraintsCache.find (tar_id);
	if (ci == constraintsCache.end ())
	{
		return NULL;
	}
	return ci->second;
}

void MasterConstraints::setTargetConstraints (int tar_id, Constraints * _constraints)
{
	Constraints *old = getTargetConstraints (tar_id);
	if (old)
		delete old;
	constraintsCache[tar_id] = _constraints;
}
