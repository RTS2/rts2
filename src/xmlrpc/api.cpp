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
	std::vector <std::string> vals = SplitStr (path, std::string ("/"));
  	std::ostringstream os;

	// widgets - divs with informations
	if (vals.size () >= 1 && vals[1] == "w")
	{
		getWidgets (vals, params, response_type, response, response_length);
		return;
	}

	os << "{";
	XmlRpcd * master = (XmlRpcd *) getMasterApp ();
	if (vals.size () == 1)
	{
		if (vals[0] == "executor")
		{
			connections_t::iterator iter = master->getConnections ()->begin ();
			master->getOpenConnectionType (DEVICE_TYPE_EXECUTOR, iter);
			if (iter == master->getConnections ()->end ())
			 	throw XmlRpcException ("executor is not connected");
			sendConnectionValues (os, *iter);
		}
	}
	os << "}";

	response_type = "application/json";
	response_length = os.str ().length ();
	response = new char[response_length];
	memcpy (response, os.str ().c_str (), response_length);
}

void API::sendConnectionValues (std::ostringstream & os, Rts2Conn * conn)
{
	for (Rts2ValueVector::iterator iter = conn->valueBegin (); iter != conn->valueEnd ();)
	{
		switch ((*iter)->getValueType ())
		{
			case RTS2_VALUE_STRING:
				os << "\"" << (*iter)->getName () << "\":\"" << (*iter)->getValue () << "\"";
				break;
			case RTS2_VALUE_DOUBLE:
			case RTS2_VALUE_FLOAT:
			case RTS2_VALUE_INTEGER:
			case RTS2_VALUE_LONGINT:
			case RTS2_VALUE_SELECTION:
				os << "\"" << (*iter)->getName () << "\":" << (*iter)->getValue ();
				break;
			case RTS2_VALUE_RADEC:
				os << "\"" << (*iter)->getName () << "\":{\"ra\":" << ((Rts2ValueRaDec *) (*iter))->getRa () << ",\"dec\":" << ((Rts2ValueRaDec *) (*iter))->getDec () << "}";
				break;
			default:
				iter++;
				continue;
		}
		iter++;
		if (iter != conn->valueEnd ())
			os << ",";
	}
}

void API::getWidgets (const std::vector <std::string> &vals, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length)
{
	std::ostringstream os;

	if (vals[1] == "executor")
	{
		os << "<div id='executor'><table>"
			"<tr><td>Current</td><td><div id='ex:current'></div></td></tr>"
			"<tr><td>Next</td><td><div id='ex:next'></div></td></tr>"
			"</table></div>";
	}

	response_type = "text/html";
	response_length = os.str ().length ();
	response = new char[response_length];
	memcpy (response, os.str ().c_str (), response_length);
}
