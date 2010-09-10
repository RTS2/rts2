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

#include "xmlrpc++/XmlRpc.h"
#include "xmlrpc++/urlencoding.h"
#include "simbadtarget.h"

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
	simbadBMag = nan ("f");
	propMotions.ra = nan ("f");
	propMotions.dec = nan ("f");

	xmlInitParser ();
}

SimbadTarget::~SimbadTarget (void)
{
	xmlCleanupParser ();
}

int SimbadTarget::load ()
{
	#define LINEBUF  200
	char buf[LINEBUF];

	const char *_uri;
	char* reply = NULL;
	int reply_length = 0;

	std::ostringstream os;
	std::string name (getTargetName ());
	urlencode (name);

	os << "http://cdsws.u-strasbg.fr/axis/services/Sesame?method=sesame&name="
		<< name << "&resultType=ui&all=true&service=NS";

	char url[os.str ().length () + 1];
	strcpy (url, os.str ().c_str ());

	XmlRpcClient httpClient (url, &_uri);

	int ret = httpClient.executeGet (_uri, reply, reply_length);
	if (!ret)
	{
		logStream (MESSAGE_ERROR) << "Error requesting " << url << sendLog;
		return -1;
	}

	xmlDocPtr xml = xmlReadMemory (reply, reply_length, NULL, NULL, XML_PARSE_NOBLANKS);
	free (reply);

	if (xml == NULL)
	{
		logStream (MESSAGE_ERROR) << "cannot parse reply from Simbad server" << sendLog;
		return -1;

	}

	xmlXPathContextPtr xpathCtx = xmlXPathNewContext (xml);
	if (xpathCtx == NULL)
	{
		logStream (MESSAGE_ERROR) << "cannot create XPath context for Simbad reply" << sendLog;
		xmlFreeDoc (xml);
		return -1;
	}

	xmlXPathObjectPtr xpathObj = xmlXPathEvalExpression (BAD_CAST "//return", xpathCtx);
	if (xpathObj == NULL || xpathObj->nodesetval->nodeNr < 1)
	{
		logStream (MESSAGE_ERROR) << "Cannot find return value" << sendLog;
		xmlXPathFreeContext (xpathCtx);
		xmlFreeDoc (xml);
		return -1;
	}

	if (!(xpathObj->nodesetval->nodeTab[0]->children))
	{
		logStream (MESSAGE_ERROR) << "Simbad server not reposponding. Is it down? Failed URL is: " << os.str () << sendLog;
		xmlXPathFreeContext (xpathCtx);
		xmlFreeDoc (xml);
		return -1;
	}

	istringstream *iss = new istringstream ();
	iss->str ((char *) xpathObj->nodesetval->nodeTab[0]->children->content);

	//std::cout << (char *) xpathObj->nodesetval->nodeTab[0]->children->content << std::endl;

	xmlXPathFreeObject (xpathObj);
	xmlXPathFreeContext (xpathCtx);
	xmlFreeDoc (xml);

	string str_type;
	while (*iss >> str_type)
	{
		if (str_type == "%J")
		{
			double ra, dec;
			*iss >> ra >> dec;
			iss->getline (buf, LINEBUF);
			setPosition (ra, dec);
		}
		else if (str_type == "#=Simbad:")
		{
			int nobj;
			*iss >> nobj;
			if (nobj != 1)
			{
				cerr << "More then 1 object found!" << endl;
				return -1;
			}
		}
		else if (str_type == "%C")
		{
			iss->getline (buf, LINEBUF);
			simbadType = string (buf);
		}
		else if (str_type == "#B")
		{
			*iss >> simbadBMag;
			simbadBMag /= 100;
		}
		else if (str_type == "%I")
		{
			iss->getline (buf, LINEBUF);
			aliases.push_back (string (buf));
		}
		else if (str_type.substr (0, 3) == "#!E")
		{
			cerr << "Not found" << endl;
			return -1;
		}
		else if (str_type == "%J.E")
		{
			iss->getline (buf, LINEBUF);
			references = string (buf);
		}
		else if (str_type == "%I.0")
		{
			// eat whitespace
			*iss >> std::ws;

			iss->getline (buf, LINEBUF);
			setTargetName (buf);
		}
		else if (str_type == "%@")
		{
			// most probably simbad version, ignore
			iss->getline (buf, LINEBUF);
		}
		else if (str_type == "%P")
		{
			*iss >> propMotions.ra >> propMotions.dec;
			// it's in masec/year
			propMotions.ra /= 360000.0;
			propMotions.dec /= 360000.0;
			iss->getline (buf, LINEBUF);
		}
		else if (str_type == "#!")
		{
			iss->getline (buf, LINEBUF);
			if (strcasestr (buf, "nothing found"))
			{
				// errrors;;
				cerr << "Not found" << endl;
				return -1;
			}
		}
		else if (str_type.c_str ()[0] == '#')
		{
			// ignore comments
			iss->getline (buf, LINEBUF);
		}
		else
		{
			cerr << "Unknow " << str_type << endl;
			iss->getline (buf, LINEBUF);
		}
	}
	#undef LINEBUF
	return 0;
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
