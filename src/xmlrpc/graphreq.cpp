/* 
 * Classes to plot graph interface.
 * Copyright (C) 2009-2010 Petr Kubanek <petr@kubanek.net>
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

#include "xmlrpcd.h"

#ifdef RTS2_HAVE_LIBJPEG

using namespace rts2xmlrpc;

void Graph::authorizedExecute (std::string path, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length)
{
	response_type = "image/jpeg";

	// get path and possibly date range
	std::vector <std::string> vals = SplitStr (path, std::string ("/"));

	int valId = 1;
	time_t to = 0;
	time_t from = 0;

	switch (vals.size ())
	{
		case 0:
			// list values;
			printDevices (response_type, response, response_length);
			break;
		case 1:
			if (isdigit (vals[0][0]))
				valId = atoi (vals[0].c_str ());
			else
				valId = 1;
			plotValue (valId, from, to, params, response_type, response, response_length);
			break;
		case 3:
			// from - to date
			if (vals[2][0] == '-')
			{
				to = time (NULL);
				from = to + strtod (vals[2].c_str (), NULL);
			}

		case 2:
			plotValue (vals[0].c_str (), vals[1].c_str (), from, to, params, response_type, response, response_length);
			break;
		default:
			throw rts2core::Error ("Invalid path for graph!");
	}
}

void Graph::printDevices (const char* &response_type, char* &response, size_t &response_length)
{
	rts2db::RecvalsSet rs = rts2db::RecvalsSet ();
	rs.load ();
	std::ostringstream _os;
	printHeader (_os, "Record value list", NULL, "/css/calendar.css");
	_os << "<script type='text/javascript' src='"
		<< ((XmlRpcd *)getMasterApp ())->getPagePrefix () << "/js/date.js'></script>\n<table>";
	int i = 1;
	for (rts2db::RecvalsSet::iterator iter = rs.begin (); iter != rs.end (); iter++, i++)
	{
		_os << std::endl << "<tr><form name='fg" << i << "' action='"
			<< iter->second.getDevice () << "/" << iter->second.getValueName () << "'><td>" << iter->second.getDevice () << "</td><td>"
			<< iter->second.getValueName () << "</td><td><select name='t'><option value='A'>Auto</option><option value='c'>Cross</option><option value='l'>Lines</option><option value='L'>Sharp Lines</option></select></td><td><input type='text' name='from' onfocus='showCalendarControl(this);'/></td><td><input type='text' name='to' onfocus='showCalendarControl(this);'/></td><td><input type='submit' value='Plot'/></td><td>"
			<< LibnovaDateDouble (iter->second.getFrom ()) << "</td><td>"
			<< LibnovaDateDouble (iter->second.getTo ()) << "</td></form></tr>";
	}
	_os << std::endl << "</table>";
	printFooter (_os);

	response_type = "text/html";
	response_length = _os.str ().length ();
	response = new char[response_length];
	memcpy (response, _os.str ().c_str (), response_length);
}

void Graph::plotValue (const char *device, const char *value, double from, double to, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length)
{
	rts2db::RecvalsSet rs = rts2db::RecvalsSet ();
	rs.load ();
	rts2db::Recval *rv = rs.searchByName (device, value);
	if (rv == NULL)
		throw rts2core::Error ("Cannot find device/value pair with given name");

	plotValue (rv, from, to, params, response_type, response, response_length);
}

void Graph::plotValue (int valId, double from, double to, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length)
{
	rts2db::RecvalsSet rs = rts2db::RecvalsSet ();
	rs.load ();
	rts2db::RecvalsSet::iterator iter = rs.find(valId);
	if (iter == rs.end ())
		throw rts2core::Error ("Cannot find device/value pair with given name");

	plotValue (&(iter->second), from, to, params, response_type, response, response_length);
}

void Graph::plotValue (rts2db::Recval *rv, double from, double to, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length)
{
	ValuePlot vp (rv->getId (), rv->getType ());

	const char *type = params->getString ("t", "A");

	PlotType pt;

	switch (*type)
	{
		case 'l':
			pt = PLOTTYPE_LINE;
			break;
		case 'L':
			pt = PLOTTYPE_LINE_SHARP;
			break;
		case 'c':
			pt = PLOTTYPE_CROSS;
			break;
		case 'C':
			pt = PLOTTYPE_CIRCLES;
			break;
		case 's':
			pt = PLOTTYPE_SQUARES;
			break;
		case 'f':
			pt = PLOTTYPE_FILL;
			break;
		case 'F':
			pt = PLOTTYPE_FILL_SHARP;
			break;
		case 'A':
		default:
			pt = PLOTTYPE_AUTO;
	}
	
	Magick::Geometry size (params->getInteger ("w", 800), params->getInteger ("h", 600));

	from = params->getDate ("from", from);
	to = params->getDate ("to", to, true);

	if (from < 0 && to == 0)
	{
		// just fr specified - from
		to = time (NULL);
		from += to;
	}
	else if (from == 0 && to == 0)
	{
		// default - one hour
		to = time (NULL);
		from = to - 86400;
	}

	Magick::Image mimage (size, "white");
	vp.getPlot (from, to, &mimage, pt, params->getInteger ("lw", 3), params->getInteger ("sh", 3), params->getBoolean ("pn", true), params->getBoolean ("ps", true), params->getBoolean ("l", true));

	Magick::Blob blob;
	mimage.write (&blob, "jpeg");

	response_length = blob.length();
	response = new char[response_length];
	memcpy (response, blob.data(), response_length);
}

#endif // RTS2_HAVE_LIBJPEG
