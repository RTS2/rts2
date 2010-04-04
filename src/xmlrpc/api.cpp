/* 
 * API access for RTS2.
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

#include "api.h"
#include "xmlrpcd.h"

using namespace rts2xmlrpc;

void API::authorizedExecute (std::string path, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length)
{
  	std::ostringstream os;
	XmlRpcd * master = (XmlRpcd *) getMasterApp ();
	os << "{";
	for (connections_t::iterator iter = master->getConnections ()->begin (); iter != master->getConnections ()->end (); iter++)
	{
		if (iter != master->getConnections ()->begin ())
			os << ", ";

		os << "\"" << (*iter)->getName () << "\" : \"" << (*iter)->getStateString () << "\"";
	}
	os << "}";

	response_type = "application/json";
	response_length = os.str ().length ();
	response = new char[response_length];
	memcpy (response, os.str ().c_str (), response_length);
}
