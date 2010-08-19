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
#include <xmlerror.h>

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

bool ConstraintTimeInterval::satisfy (double JD)
{
	return between (JD, from, to);
}

void ConstraintTime::load (xmlNodePtr cons)
{
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
				else
					throw XmlUnexpectedNode (par);
			}
			addInterval (from, to);
		}
		else
		{
			throw XmlUnexpectedNode (inter);
		}
	}
}

bool ConstraintTime::satisfy (Target *target, double JD)
{
	for (std::list <ConstraintTimeInterval>::iterator iter = intervals.begin (); iter != intervals.end (); iter++)
	{
		if (iter->satisfy (JD))
			return true;
	}
	return false;
}

bool ConstraintDoubleInterval::satisfy (double val)
{
	return between (val, lower, upper);
}

void ConstraintDouble::load (xmlNodePtr cons)
{
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
				else
					throw XmlUnexpectedNode (par);
			}
			addInterval (lower, upper);
		}
		else
		{
			throw XmlUnexpectedNode (inter);
		}
	}
}

bool ConstraintDouble::isBetween (double val)
{
	for (std::list <ConstraintDoubleInterval>::iterator iter = intervals.begin (); iter != intervals.end (); iter++)
	{
		if (iter->satisfy (val))
			return true;
	}
	return false;
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

Constraints::~Constraints ()
{
	for (Constraints::iterator iter = begin (); iter != end (); iter++)
		delete *iter;
	clear ();
}

bool Constraints::satisfy (Target *tar, double JD)
{
	for (Constraints::iterator iter = begin (); iter != end (); iter++)
	{
		if (!((*iter)->satisfy (tar, JD)))
			return false;
	}
	return true;
}

size_t Constraints::violated (Target *tar, double JD)
{
	size_t vn = 0;
	for (Constraints::iterator iter = begin (); iter != end (); iter++)
	{
		if (!((*iter)->satisfy (tar, JD)))
			vn++;
	}
	return vn;
}

void Constraints::load (xmlNodePtr _node)
{
	for (xmlNodePtr cons = _node->children; cons != NULL; cons = cons->next)
	{
		if (xmlStrEqual (cons->name, (xmlChar *) "time"))
		{
			ConstraintTime *tc = new ConstraintTime ();
			try
			{
				tc->load (cons);
			}
			catch (XmlError er)
			{
				delete tc;
				throw er;
			}
			push_back (tc);
		}
		else // assume it is DoubleInterval node
		{
			ConstraintDouble *cdi;
			if (xmlStrEqual (cons->name, (xmlChar *) "airmass"))
				cdi = new ConstraintAirmass ();
			else if (xmlStrEqual (cons->name, (xmlChar *) "HA"))
				cdi = new ConstraintHA ();
			else if (xmlStrEqual (cons->name, (xmlChar *) "lunarDistance"))
				cdi = new ConstraintLunarDistance ();
			else if (xmlStrEqual (cons->name, (xmlChar *) "lunarPhase"))
				cdi = new ConstraintLunarPhase ();
			else if (xmlStrEqual (cons->name, (xmlChar *) "solarDistance"))
				cdi = new ConstraintSolarDistance ();
			else if (xmlStrEqual (cons->name, (xmlChar *) "sunAltitude"))
				cdi = new ConstraintSunAltitude ();
			else
			{
				delete cdi;
				throw XmlUnexpectedNode (cons);
			}
			try
			{
				cdi->load (cons);
			}
			catch (XmlError er)
			{
				delete cdi;
				throw er;
			}
			push_back (cdi);
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
