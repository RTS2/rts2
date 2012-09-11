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

#ifndef __RTS2_HTTPREQ__
#define __RTS2_HTTPREQ__

#include "config.h"

#ifdef HAVE_PGSQL
#ifdef HAVE_LIBJPEG
#include <Magick++.h>
#endif // HAVE_LIBJPEG
#else

#endif // HAVE_PGSQL

#include "xmlrpc++/XmlRpc.h"
#include "bb.h"

namespace rts2bb
{

/**
 * Abstract class for authorized requests. Acts similarly
 * to servelets under JSP container. The class have method
 * to reply to requests arriving on the incoming connection.
 *
 * The most important is GetRequestAuthorized::authorizedExecute method.  This
 * is called after user properly authorised agains setup authorization
 * mechanism.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class GetRequestAuthorized: public XmlRpc::XmlRpcServerGetRequest
{


	public:
		GetRequestAuthorized (const char* prefix, const char *description = NULL, XmlRpc::XmlRpcServer* s = 0):XmlRpcServerGetRequest (prefix, description, s) {}

		virtual void execute (XmlRpc::XmlRpcSource *source, struct sockaddr_in *saddr, std::string path, XmlRpc::HttpParams *params, int &http_code, const char* &response_type, char* &response, size_t &response_length);

	protected:
		/**
		 * Received exact path and HTTP params. Returns response - MIME
		 * type, its data and length. This request is password
		 * protected - pasword protection can be removed by listing
		 * path in public section of XML-RPC config file.
		 *
		 * @param path            exact path of the request, excluding prefix part
		 * @param params          HTTP parameters. Please see xmlrpc++/XmlRpcServerGetRequest.h for allowed methods.
		 * @param response_type   MIME type of response. Ussually you will put there something like "text/html" or "image/jpeg".
		 * @param response        response data. Must be allocated, preferably by new char[]. They will be deleted by calling code
		 * @param response_length response lenght in bytes
		 *
		 * @see GetRequestAuthorized::execute
		 */
		virtual void authorizedExecute (std::string path, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length) = 0;
};

/**
 * Retrieve status of all observatories logged to system.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Status:public GetRequestAuthorized
{
	public:
		Status (const char *description, XmlRpc::XmlRpcServer* s, BB* bb):GetRequestAuthorized ("/status", description, s) { _bb = bb; }

	protected:
		virtual void authorizedExecute (std::string path, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length);
	
	private:
		BB *_bb;

		void listObservatories (std::ostringstream &_os);
		void printObservatory (const std::string &name, std::ostringstream &_os);
};

}

#endif /* !__RTS2_HTTPREQ__ */
