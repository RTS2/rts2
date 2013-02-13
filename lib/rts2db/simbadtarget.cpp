/*
 * Target from SIMBAD database.
 * Copyright (C) 2005-2010 Petr Kubanek <petr@kubanek.net>
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

#include "rts2db/simbadtarget.h"
#include "rts2db/mpectarget.h"

#include "xmlrpc++/XmlRpc.h"
#include "xmlrpc++/urlencoding.h"

#include "configuration.h"

#include <sstream>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>

using namespace std;
using namespace rts2db;
using namespace XmlRpc;

SimbadTarget::SimbadTarget (const char *in_name):ConstTarget ()
{
	setTargetName (in_name);
	simbadBMag = NAN;
	propMotions.ra = NAN;
	propMotions.dec = NAN;

	xmlInitParser ();
}

SimbadTarget::~SimbadTarget (void)
{
	xmlCleanupParser ();
}

void SimbadTarget::load ()
{
	#define LINEBUF  200
	char buf[LINEBUF];

	const char *_uri;
	char* reply = NULL;
	int reply_length = 0;

	std::ostringstream os;
	std::string name (getTargetName ());
	urlencode (name);

	std::string simbadurl;

	rts2core::Configuration::instance ()->getString ("database", "simbadurl", simbadurl, "http://cdsws.u-strasbg.fr/axis/services/Sesame?method=sesame&resultType=ui&all=true&service=NS&name=");

	os << simbadurl << name;

	char url[os.str ().length () + 1];
	strcpy (url, os.str ().c_str ());

	XmlRpcClient httpClient (url, &_uri);

	std::ostringstream err;

	int ret = httpClient.executeGet (_uri, reply, reply_length);
	if (!ret)
	{
	  	err << "error requesting " << url;
		throw rts2core::Error (err.str ());
	}

	xmlDocPtr xml = xmlReadMemory (reply, reply_length, NULL, NULL, XML_PARSE_NOBLANKS);
	free (reply);

	if (xml == NULL)
	{
	  	throw rts2core::Error ("cannot parse reply from Simbad server");
	}

	xmlXPathContextPtr xpathCtx = xmlXPathNewContext (xml);
	if (xpathCtx == NULL)
	{
		xmlFreeDoc (xml);
		throw rts2core::Error ("cannot create XPath context for Simbad reply");
	}

	xmlXPathObjectPtr xpathObj = xmlXPathEvalExpression (BAD_CAST "//return", xpathCtx);
	if (xpathObj == NULL || xpathObj->nodesetval->nodeNr < 1)
	{
		xmlXPathFreeContext (xpathCtx);
		xmlFreeDoc (xml);
		throw rts2core::Error ("Cannot find return value");
	}

	if (!(xpathObj->nodesetval->nodeTab[0]->children))
	{
		xmlXPathFreeContext (xpathCtx);
		xmlFreeDoc (xml);
		err << "Simbad server not reposponding. Is it down? Failed URL is: " << os.str ();
		throw rts2core::Error (err.str ());
	}

	istringstream iss;
	iss.str ((char *) xpathObj->nodesetval->nodeTab[0]->children->content);

	//std::cout << (char *) xpathObj->nodesetval->nodeTab[0]->children->content << std::endl;

	xmlXPathFreeObject (xpathObj);
	xmlXPathFreeContext (xpathCtx);
	xmlFreeDoc (xml);

	string str_type;
	while (iss >> str_type)
	{
		if (str_type == "%J")
		{
			double ra, dec;
			iss >> ra >> dec;
			iss.getline (buf, LINEBUF);
			setPosition (ra, dec);
		}
		else if (str_type == "#=Simbad:")
		{
			int nobj;
			iss >> nobj;
			if (nobj != 1)
			  	throw rts2core::Error ("More then 1 object found!");
		}
		else if (str_type == "%C")
		{
			iss.getline (buf, LINEBUF);
			simbadType = string (buf);
		}
		else if (str_type == "#B")
		{
			iss >> simbadBMag;
			simbadBMag /= 100;
		}
		else if (str_type == "%I")
		{
			iss.getline (buf, LINEBUF);
			aliases.push_back (string (buf));
		}
		else if (str_type.substr (0, 3) == "#!E")
		{
			err << "object with name " << getTargetName () << " was not resolved by Simbad";
			throw rts2core::Error (err.str ());
		}
		else if (str_type == "%J.E")
		{
			iss.getline (buf, LINEBUF);
			references = string (buf);
		}
		else if (str_type == "%I.0")
		{
			// eat whitespace
			iss >> std::ws;

			iss.getline (buf, LINEBUF);
			setTargetName (buf);
		}
		else if (str_type == "%@")
		{
			// most probably simbad version, ignore
			iss.getline (buf, LINEBUF);
		}
		else if (str_type == "%P")
		{
			iss >> propMotions.ra >> propMotions.dec;
			// it's in masec/year
			propMotions.ra /= 360000.0;
			propMotions.dec /= 360000.0;
			iss.getline (buf, LINEBUF);
		}
		else if (str_type == "#!")
		{
			iss.getline (buf, LINEBUF);
			if (strcasestr (buf, "nothing found"))
			{
				// errrors;;
				err << "object with name " << getTargetName () << " was not resolved by Simbad";
				throw rts2core::Error (err.str ());
			}
		}
		else if (str_type.c_str ()[0] == '#')
		{
			// ignore comments
			iss.getline (buf, LINEBUF);
		}
		else
		{
			cerr << "Unknow " << str_type << endl;
			iss.getline (buf, LINEBUF);
		}
	}
	#undef LINEBUF
}

void SimbadTarget::printExtra (Rts2InfoValStream & _ivs)
{
	ConstTarget::printExtra (_ivs, ln_get_julian_from_sys ());

	if (_ivs.getStream ())
	{
		std::ostream* _os = _ivs.getStream ();
		*_os << "REFERENCED " << references << std::endl;
		int old_prec = _os->precision (2);
		*_os << "PROPER MOTION RA " <<
			(propMotions.ra * 360000.0)
			<< " DEC " << (propMotions.dec * 360000.0) << " (mas/y)";
		*_os << "TYPE " << simbadType << std::endl;
		*_os << "B MAG " << simbadBMag << std::endl;
		_os->precision (old_prec);

		for (std::list < std::string >::iterator alias = aliases.begin ();
			alias != aliases.end (); alias++)
		{
			*_os << "ALIAS " << (*alias) << std::endl;
		}
	}
}

Target *createTargetByString (const char *tar_string, bool debug)
{
	Target *rtar = NULL;

	LibnovaRaDec raDec;

	int ret = raDec.parseString (tar_string);
	if (ret == 0)
	{
		std::string new_prefix;

		// default prefix for new RTS2 sources
		new_prefix = std::string ("RTS2_");

		// set name..
		ConstTarget *constTarget = new ConstTarget ();
		constTarget->setPosition (raDec.getRa (), raDec.getDec ());
		std::ostringstream os;

		rts2core::Configuration::instance ()->getString ("newtarget", "prefix", new_prefix, "RTS2");
		os << new_prefix << LibnovaRaComp (raDec.getRa ()) << LibnovaDeg90Comp (raDec.getDec ());
		constTarget->setTargetName (os.str ().c_str ());
		constTarget->setTargetType (TYPE_OPORTUNITY);
		return constTarget;
	}
	// if it's MPC ephemeris..
	rtar = new EllTarget ();
	ret = ((EllTarget *) rtar)->orbitFromMPC (tar_string, debug);
	if (ret == 0)
	{
		return rtar;
	}

	delete rtar;

	XmlRpcLogHandler::setVerbosity (debug ? 5 : 0);

	// try to get target from SIMBAD
	try
	{
		rtar = new rts2db::SimbadTarget (tar_string);
		rtar->load ();
		rtar->setTargetType (TYPE_OPORTUNITY);
		return rtar;
	}
	catch (rts2core::Error &er)
	{
		// MPEC fallback
		rtar = new rts2db::MPECTarget (tar_string);
		rtar->load ();
		return rtar;
	}
}
