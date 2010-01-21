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
 * Abstract class for authorized requests.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class GetRequestAuthorized: public XmlRpc::XmlRpcServerGetRequest
{
	public:
		GetRequestAuthorized (const char* prefix, XmlRpc::XmlRpcServer* s):XmlRpcServerGetRequest (prefix, s) {}

		virtual void execute (std::string path, XmlRpc::HttpParams *params, int &http_code, const char* &response_type, char* &response, size_t &response_length);

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
 * Draw graph of variables.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Graph: public GetRequestAuthorized
{
	public:
		Graph (const char *prefix, XmlRpc::XmlRpcServer *s):GetRequestAuthorized (prefix, s) {};

		virtual void authorizedExecute (std::string path, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length);

	private:
		void printDevices (const char* &response_type, char* &response, size_t &response_length);

		void plotValue (const char *device, const char *value, double from, double to, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length);
};

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

class Targets: public GetRequestAuthorized
{
	public:
		Targets (const char *prefix, XmlRpc::XmlRpcServer *s):GetRequestAuthorized (prefix, s) {};

		virtual void authorizedExecute (std::string path, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length);
	
	private:
		void listTargets (const char* &response_type, char* &response, size_t &response_length);
		void printTarget (Target *tar, const char* &response_type, char* &response, size_t &response_length);
		void printTargetImages (Target *tar, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length);
		void printTargetObservations (Target *tar, const char* &response_type, char* &response, size_t &response_length);
#ifdef HAVE_LIBJPEG
		void printTargetPlot (Target *tar, const char* &response_type, char* &response, size_t &response_length);
#endif /* HAVE_LIBJPEG */
};

/**
 * Add target to the database. Consists of those steps:
 *

 * @author Petr Kubanek <petr@kubanek.net>
 */
class AddTarget: public GetRequestAuthorized
{
	public:
		AddTarget (const char *prefix, XmlRpc::XmlRpcServer *s):GetRequestAuthorized (prefix, s) {};

		virtual void authorizedExecute (std::string path, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length);

	private:
		void askForTarget (const char* &response_type, char* &response, size_t &response_length);
		void confimTarget (const char *tar, const char* &response_type, char* &response, size_t &response_length);
		void newTarget (const char *oriname, const char *name, int tarid, double ra, double dec, const char* &response_type, char* &response, size_t &response_length);
		void schedule (int tarid, const char* &response_type, char* &response, size_t &response_length);
};

#endif /* HAVE_PGSQL */

}

#endif /* !__RTS2_HTTPREQ__ */
