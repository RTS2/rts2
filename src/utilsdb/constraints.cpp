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
#include <xmlerror.h>

bool between (double val, double low, double upper)
{
	if (isnan (low) && isnan (upper))
		return true;
	if (isnan (low))
		return val < upper;
	if (isnan (upper))
		return val >= low;
	return val >= low && val < upper;
}

using namespace rts2db;

bool ConstraintTimeInterval::satisfy (Target *target, double JD)
{
	return between (JD, from, to);
}

bool ConstraintAirmass::satisfy (Target *tar, double JD)
{
	return between (tar->getAirmass (JD), getLower (), getUpper ());
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
			double from = rts2_nan ("f");
			double to = rts2_nan ("f");
			for (xmlNodePtr par = cons->children; par != NULL; par = par->next)
			{
				if (xmlStrEqual (par->name, (xmlChar *) "from"))
					parseDate ((const char *) par->children->content, from);
				else if (xmlStrEqual (par->name, (xmlChar *) "to"))
					parseDate ((const char *) par->children->content, to);
				else
					throw XmlUnexpectedNode (par);
			}
			push_back (new ConstraintTimeInterval (from, to));
		}
		else // assume it is DoubleInterval node
		{
			double lower = rts2_nan ("f");
			double upper = rts2_nan ("f");
			for (xmlNodePtr par = cons->children; par != NULL; par = par->next)
			{
				if (xmlStrEqual (par->name, (xmlChar *) "lower"))
					lower = atof ((const char *) par->children->content);
				else if (xmlStrEqual (par->name, (xmlChar *) "upper"))
					upper = atof ((const char *) par->children->content);
				else
					throw XmlUnexpectedNode (par);
			}
			if (xmlStrEqual (cons->name, (xmlChar *) "airmass"))
				push_back (new ConstraintAirmass (lower, upper));
			else
				throw XmlUnexpectedNode (cons);
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
