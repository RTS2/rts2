/* 
 * Classes for answers to HTTP requests.
 * Copyright (C) 2009 Petr Kubanek <petr@kubanek.net>
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

#include "augerreq.h"

#ifdef HAVE_PGSQL
#include "../utilsdb/augerset.h"
#if defined(HAVE_LIBJPEG) && HAVE_LIBJPEG == 1
#include <Magick++.h>
#endif // HAVE_LIBJPEG

using namespace XmlRpc;
using namespace rts2xmlrpc;

void Auger::authorizedExecute (std::string path, XmlRpc::HttpParams *params, const char* &response_type, char* &response, int &response_length)
{
	response_type = "text/html";

	// get path and possibly date range
	std::vector <std::string> vals = SplitStr (path.substr (1), std::string ("/"));
	int year = -1;
	int month = -1;
	int day = -1;

	switch (vals.size ())
	{
		case 3:
			day = atoi (vals[2].c_str ());
		case 2:
			month = atoi (vals[1].c_str ());
		case 1:
			year = atoi (vals[0].c_str ());
		case 0:
			printTable (year, month, day, response, response_length);
			break;
		default:
			throw rts2core::Error ("Invalid path for graph!");
	}
}

void Auger::printTable (int year, int month, int day, char* &response, int &response_length)
{
	std::ostringstream _os;

	_os << "<html><head><title>Auger showers";

	if (year > 0)
	{
		_os << " for " << year;
		if (month > 0)
		{
			_os << "-" << month;
			if (day > 0)
			{
				_os << "-" << day;
			}
		}
	}

	_os << "</title></head><body><p><table>";

	rts2db::AugerSetDate as = rts2db::AugerSetDate ();
	as.load (year, month, day);

	for (rts2db::AugerSetDate::iterator iter = as.begin (); iter != as.end (); iter++)
	{
		_os << "<tr><td><a href='" << iter->first << "'>" << iter->first << "</a></td><td>" << iter->second << "</td></tr>";
	}

	_os << "</table><p></body></html>";

	response_length = _os.str ().length ();
	response = new char[response_length];
	memcpy (response, _os.str ().c_str (), response_length);
}

#endif /* HAVE_PGSQL */
