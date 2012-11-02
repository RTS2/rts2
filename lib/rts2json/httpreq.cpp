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

#include "rts2db/user.h"
#include "rts2json/httpreq.h"

using namespace XmlRpc;
using namespace rts2json;

void GetRequestAuthorized::execute (XmlRpc::XmlRpcSource *source, struct sockaddr_in *saddr, std::string path, HttpParams *params, int &http_code, const char* &response_type, char* &response, size_t &response_length)
{
	// if it is public page..
	if (getServer ()->isPublic (saddr, getPrefix () + path))
	{
		http_code = HTTP_OK;
		authorizedExecute (source, path, params, response_type, response, response_length);
		return;
	}

	if (getUsername () == std::string ("session_id"))
	{
		if (getServer ()->existsSession (getPassword ()) == false)
		{
			authorizePage (http_code, response_type, response, response_length);
			return;
		}
	}

	if (getServer ()->verifyDBUser (getUsername (), getPassword (), executePermission) == false)
	{
		authorizePage (http_code, response_type, response, response_length);
		return;
	}
	http_code = HTTP_OK;

	authorizedExecute (source, path, params, response_type, response, response_length);

	getServer ()->addExecutedPage ();
}

void GetRequestAuthorized::printHeader (std::ostream &os, const char *title, const char *css, const char *cssLink, const char *onLoad)
{
	os << "<html><head><title>" << title << "</title>";
	
	if (css)
		os << "<style type='text/css'>" << css << "</style>";

	if (cssLink)
		os << "<link href='" << getServer ()->getPagePrefix () << cssLink << "' rel='stylesheet' type='text/css'/>";

	os << "</head><body";
	
	if (onLoad)
		os << " onload='" << onLoad << "'";
	
	os << ">";
}

void GetRequestAuthorized::printSubMenus (std::ostream &os, const char *prefix, const char *current, const char *submenus[][2])
{
	const char **p = submenus[0];  
	const char *a = p[0];
	const char *n = p[1];
	while (true)
	{
		if (strcmp (current, a))
		{
			os << "<a href='" << prefix << a << "/'>" << n << "</a>";
		}
		else
		{
			os << n;
		}
		p += 2;
		a = p[0];
		n = p[1];
		if (a == NULL)
			break;
		os << "&nbsp;";
	}
}

void GetRequestAuthorized::printFooter (std::ostream &os)
{
	os << "</body></html>";
}

void GetRequestAuthorized::includeJavaScript (std::ostream &os, const char *name)
{
	os << "<script type='text/javascript' src='" << getServer ()->getPagePrefix () << "/js/" << name << "'></script>\n";
}

void GetRequestAuthorized::includeJavaScriptWithPrefix (std::ostream &os, const char *name)
{
	os << "<script type='text/javascript'>pagePrefix = '" << getServer ()->getPagePrefix () << "';</script>\n";
	includeJavaScript (os, name);
}


