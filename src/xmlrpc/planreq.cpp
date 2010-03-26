/* 
 * Classes for generating pages for planning/scheduling.
 * Copyright (C) 2010 Petr Kubanek <petr@kubanek.net>
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

#include "planreq.h"
#include "xmlrpcd.h"

using namespace rts2xmlrpc;

void Plan::authorizedExecute (std::string path, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length)
{
	std::vector <std::string> vals = SplitStr (path.substr (1), std::string ("/"));
	response_type = "text/html";
	switch (vals.size ())
	{
		case 0:
			printScheduling (response, response_length);
			return;
		case 1:
			selectNext (response, response_length);
			return;
	}
	throw rts2core::Error ("Invalid path");
}

void Plan::printScheduling (char* &response, size_t &response_length)
{
	std::ostringstream _os;

	printHeader (_os, "Planning");

	XmlRpcd *serv = (XmlRpcd *) getMasterApp ();
	Rts2Conn * conn = serv->getOpenConnection (DEVICE_TYPE_EXECUTOR);

	if (conn == NULL)
		throw rts2core::Error ("Cannot find executor connection. Please check your setup");

	Rts2Value *val = conn->getValue ("current_id");
	if (val == NULL)
		throw rts2core::Error ("Cannot find current target ID");

	_os << "Current target is " << val->getDisplayValue () << "<br/>";

	_os << "<a href='next'>Select next target</a>";

	printFooter (_os);

	response_length = _os.str ().length ();
	response = new char[response_length];
	memcpy (response, _os.str ().c_str (), response_length);
}

void Plan::selectNext (char* &response, size_t &response_length)
{
	std::ostringstream _os;
	printHeader (_os, "Observing plan");

	XmlRpcd *serv = (XmlRpcd *) getMasterApp ();
	for (connections_t::iterator iter = serv->getConnections ()->begin (); iter != serv->getConnections ()->end (); iter++)
	{
		_os << "<tr><td><a href='" << (*iter)->getName () << "/'>" << (*iter)->getName () << "</a></td></tr>\n";
	}

	_os << "\n</table>";
	
	printFooter (_os);

	response_length = _os.str ().length ();
	response = new char[response_length];
	memcpy (response, _os.str ().c_str (), response_length);
}
