/* 
 * Classes answering to HTTP requests.
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

#include <sstream>

#include "httpreq.h"
#include "libnova_cpp.h"

using namespace XmlRpc;
using namespace rts2bb;

void GetRequestAuthorized::execute (XmlRpc::XmlRpcSource *source, struct sockaddr_in *saddr, std::string path, HttpParams *params, int &http_code, const char* &response_type, char* &response, size_t &response_length)
{
	if (! (getUsername() ==  std::string ("rts2") && getPassword() == std::string ("rts2")))
	{
		authorizePage (http_code, response_type, response, response_length);
		return;
	}
	http_code = HTTP_OK;

	authorizedExecute (path, params, response_type, response, response_length);
}

void Status::authorizedExecute (std::string path, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length)
{
	std::ostringstream _os;

	std::vector <std::string> vals = SplitStr (path, std::string ("/"));

	switch (vals.size ())
	{
		case 0:
			listObservatories (_os);
			break;
		case 1:
			printObservatory (vals[0], _os);		
			break;
		default:
			throw XmlRpcException ("Invalid path");
	}

	response_type = "text/html";
	response_length = _os.str ().length ();
	response = new char[response_length];
	memcpy (response, _os.str ().c_str (), response_length);
}

void Status::listObservatories (std::ostringstream &_os)
{
	_os << "<html><head><title>Observatory status</title></head><body>\n"
		<< "<ul>\n";
	for (std::map <std::string,Observatory>::iterator iter = _bb->omBegin (); iter != _bb->omEnd (); iter++)
	{
		time_t lastUpdate = iter->second.getUpdateTime ();
		_os << "<li><a href='" << iter->first << "'>" << iter->first << "</a> " << LibnovaDate (&lastUpdate) << "</li>\n";
	}
	_os << "</ul>\n</body>";
}

void Status::printObservatory (const std::string &name, std::ostringstream &_os)
{
	_os << "<html><head><title>Observatory " << name << "</title></head><body>\n";
	XmlRpcValue *v = _bb->findValues (name);
	_os << (*v);
	_os << "</body>";
}
