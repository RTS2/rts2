/* 
 * Switch state support.
 * Copyright (C) 2010 Petr Kubanek <kubanek@fzu.cz> Institute of Physics, Czech Republic
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

#include "rts2json/httpreq.h"
#include "xmlrpcd.h"

using namespace rts2xmlrpc;

void SwitchState::authorizedExecute (XmlRpc::XmlRpcSource *source, std::string path, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length)
{
	std::vector <std::string> vals = SplitStr (path, std::string ("/"));
	if (vals.size () == 1)
	{
		if (vals[0] == "off")
			((XmlRpcd *) getMasterApp ())->queAllCentralds ("off");
		else if (vals[0] == "standby")
		  	((XmlRpcd *) getMasterApp ())->queAllCentralds ("standby");
		else if (vals[0] == "on")
		  	((XmlRpcd *) getMasterApp ())->queAllCentralds ("on");
		else
		  	throw XmlRpcException ("Cannot process request");
	}
	std::ostringstream _os;
	printHeader (_os, "State switching");

	_os << "<p>Current state is: " << ((XmlRpcd *) getMasterApp ())->getMasterStateString () << "<p>";

	_os << "<p>Set state to:"
		"<p>"
		  "<form action='off' method='get'><input type='submit' value='OFF'></form>\n"
		  "System is OFF, telescope stowed."
	        "</p><p>"
	          "<form action='standby' method='get'><input type='submit' value='STANDBY'></form>\n"
		  "System is ready, but dome slit is closed. This state is used to stop observations manually."
                "</p><p>"
		  "<form action='on' method='get'><input type='submit' value='ON'></form>"
		  "System is up and running, taking images during night."
		"</p>"
	      "</p>";

	printFooter (_os);

	response_type = "text/html";
	response_length = _os.str ().length ();
	response = new char[response_length];
	memcpy (response, _os.str ().c_str (), response_length);
}
