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

#include "constraints.h"
#include "../utils/utilsfunc.h"
#include "../utils/rts2config.h"

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

Constraint::Constraint (Constraint &cons)
{
	for (std::list <ConstraintDoubleInterval>::iterator iter = cons.intervals.begin (); iter != cons.intervals.end (); iter++)
		add (*iter);
}

void Constraint::load (xmlNodePtr cons)
{
	intervals.clear ();
	for (xmlNodePtr inter = cons->children; inter != NULL; inter = inter->next)
	{
		if (xmlStrEqual (inter->name, (xmlChar *) "interval"))
		{
			double lower = rts2_nan ("f");
			double upper = rts2_nan ("f");

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

bool Constraint::isBetween (double val)
{
	for (std::list <ConstraintDoubleInterval>::iterator iter = intervals.begin (); iter != intervals.end (); iter++)
	{
		if (iter->satisfy (val))
			return true;
	}
	return false;
}

void ConstraintTime::load (xmlNodePtr cons)
{
	clearIntervals ();
	for (xmlNodePtr inter = cons->children; inter != NULL; inter = inter->next)
	{
		if (xmlStrEqual (inter->name, (xmlChar *) "interval"))
		{
			double from = rts2_nan ("f");
			double to = rts2_nan ("f");

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

bool ConstraintAirmass::satisfy (Target *tar, double JD)
{
	return isBetween (tar->getAirmass (JD));
}

bool ConstraintHA::satisfy (Target *tar, double JD)
{
	return isBetween (tar->getHourAngle (JD));
}

bool ConstraintLunarDistance::satisfy (Target *tar, double JD)
{
	return isBetween (tar->getLunarDistance (JD));
}

bool ConstraintLunarPhase::satisfy (Target *tar, double JD)
{
	return isBetween (ln_get_lunar_phase (JD));
}

bool ConstraintSolarDistance::satisfy (Target *tar, double JD)
{
	return isBetween (tar->getSolarDistance (JD));
}

bool ConstraintSunAltitude::satisfy (Target *tar, double JD)
{
	struct ln_equ_posn eq_sun;
	struct ln_hrz_posn hrz_sun;
	ln_get_solar_equ_coords (JD, &eq_sun);
	ln_get_hrz_from_equ (&eq_sun, Rts2Config::instance ()->getObserver (), JD, &hrz_sun);
	return isBetween (hrz_sun.alt);
}

Constraints::Constraints (Constraints &cs): std::map <std::string, Constraint *> (cs)
{
}

Constraints::~Constraints ()
{
	for (Constraints::iterator iter = begin (); iter != end (); iter++)
		delete iter->second;
	clear ();
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

size_t Constraints::violated (Target *tar, double JD, std::list <std::string> &names)
{
	names.clear ();
	for (Constraints::iterator iter = begin (); iter != end (); iter++)
	{
		if (!(iter->second->satisfy (tar, JD)))
			names.push_back (iter->first);
	}
	return names.size ();
}

void Constraints::load (xmlNodePtr _node)
{
	for (xmlNodePtr cons = _node->children; cons != NULL; cons = cons->next)
	{
		if (cons->type == XML_COMMENT_NODE)
			continue;
	  	Constraint *con;
		Constraints::iterator candidate = find (std::string ((const char *) cons->name));
		if (candidate != end ())
		{
			con = candidate->second;
		}
		else 
		{
			con = createConstraint ((const char *) cons->name);
			if (con == NULL)
				throw XmlUnexpectedNode (cons);
		}
		try
		{
			con->load (cons);
		}
		catch (XmlError er)
		{
			delete con;
			throw er;
		}
		if (candidate == end ())
		{
			(*this)[std::string ((const char *) cons->name)] = con;
		}
	}
}

void Constraints::load (const char *filename)
{
	LIBXML_TEST_VERSION

	xmlLineNumbersDefault (1);
	xmlDoc *doc = xmlReadFile (filename, NULL, XML_PARSE_NOBLANKS);
	if (doc == NULL)
		throw XmlError ("cannot parse constraint file " + std::string (filename));

	xmlNodePtr root_element = xmlDocGetRootElement (doc);
	if (!xmlStrEqual (root_element->name, (xmlChar *) "constraints"))
		throw XmlUnexpectedNode (root_element);

	load (root_element);
	xmlFreeDoc (doc);
	xmlCleanupParser ();
}

Constraint *Constraints::createConstraint (const char *name)
{
	if (!strcmp (name, "time"))
		return new ConstraintTime ();
	else if (!strcmp (name, "airmass"))
		return new ConstraintAirmass ();
	else if (!strcmp (name, "HA"))
		return new ConstraintHA ();
	else if (!strcmp (name, "lunarDistance"))
		return new ConstraintLunarDistance ();
	else if (!strcmp (name, "lunarPhase"))
		return new ConstraintLunarPhase ();
	else if (!strcmp (name, "solarDistance"))
		return new ConstraintSolarDistance ();
	else if (!strcmp (name, "sunAltitude"))
		return new ConstraintSunAltitude ();
	return NULL;
}

Constraints *MasterConstraints::cons = NULL;

Constraints & MasterConstraints::getConstraint ()
{
	if (cons)
		return *cons;
	cons = new Constraints ();
	cons->load (Rts2Config::instance ()->getMasterConstraintFile ());
	return *cons;
}
