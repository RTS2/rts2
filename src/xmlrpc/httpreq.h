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

#ifndef __RTS2_HTTPREQ__
#define __RTS2_HTTPREQ__

#include "config.h"

#ifdef HAVE_PGSQL
#include "../utilsdb/recvals.h"
#include "../utilsdb/records.h"
#include "../utilsdb/rts2devicedb.h"
#include "../utilsdb/targetset.h"
#if defined(HAVE_LIBJPEG) && HAVE_LIBJPEG == 1
#include <Magick++.h>
#include "valueplot.h"
#endif // HAVE_LIBJPEG
#include "../utils/rts2config.h"
#include "../utils/rts2device.h"
#else

#endif // HAVE_PGSQL

#include "xmlrpc++/XmlRpc.h"

#include "../utils/libnova_cpp.h"

namespace rts2xmlrpc
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
		GetRequestAuthorized (const char* prefix, XmlRpc::XmlRpcServer* s):XmlRpcServerGetRequest (prefix, s) {}

		virtual void execute (std::string path, XmlRpc::HttpParams *params, int &http_code, const char* &response_type, char* &response, size_t &response_length);

		/** 
		 * Executed after user is properly authorized.
		 *
		 * @brief 
		 * 
		 * @param path called path on server (stripped from prefix, which resulted in this object called)
		 * @param params HTTP parameters received from client
		 * @param response_type MIME type of the response
		 * @param response response buffer. It must be allocated (XmlRpcServerGetRequest will free it)
		 * @param response_length length of response buffer
		 *
		 * @throw rts2core::Error and its descendands
		 *
		 * @see GetRequestAuthorized::execute
		 */
		virtual void authorizedExecute (std::string path, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length) = 0;
};

/**
 * Maps directory to request space.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Directory: public GetRequestAuthorized
{
	public:
		Directory (const char* prefix, const char *_dirPath, XmlRpc::XmlRpcServer* s):GetRequestAuthorized (prefix, s) { dirPath = _dirPath; }

		virtual void authorizedExecute (std::string path, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length);

	private:
		const char *dirPath;
};

#ifdef HAVE_LIBJPEG

/**
 * Plot current position of telescope, target and next target position.
 */
class CurrentPosition:public XmlRpc::XmlRpcServerGetRequest
{
	public:
		CurrentPosition (const char *prefix, XmlRpc::XmlRpcServer *s):XmlRpc::XmlRpcServerGetRequest (prefix, s) {};

		virtual void execute (std::string path, XmlRpc::HttpParams *params, int &http_code, const char* &response_type, char* &response, size_t &response_length);
};

#endif /* HAVE_LIBJPEG */

#ifdef HAVE_PGSQL

#ifdef HAVE_LIBJPEG

/**
 * Plot targets on the alt-az graph.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class AltAzTarget: public GetRequestAuthorized
{
	public:
		AltAzTarget (const char *prefix, XmlRpc::XmlRpcServer *s):GetRequestAuthorized (prefix, s) {};

		/**
		 * Received exact path and HTTP params. Returns response - MIME
		 * type, its data and length. This request is password
		 * protected - pasword protection can be removed by listing
		 * path in public section of XML-RPC config file.
		 *
		 * @param path            Exact path of the request, excluding prefix part.
		 * @param params          HTTP parameters. Please see xmlrpc++/XmlRpcServerGetRequest.h for allowed methods.
		 * @param response_type   MIME type of response. Ussually you will put there something like "text/html" or "image/jpeg".
		 * @param response        Response data. Must be allocated, preferably by new char[]. They will be deleted by calling code.
		 * @param response_length Response lenght in bytes. 
		 */
		virtual void authorizedExecute (std::string path, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length);
};

#endif /* HAVE_LIBJPEG */ 

#endif /* HAVE_PGSQL */

}

#endif /* !__RTS2_HTTPREQ__ */
