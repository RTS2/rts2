/* 
 * Classes to plot graph interface.
 * Copyright (C) 2009-2010,2012 Petr Kubanek <petr@kubanek.net>
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

#ifdef RTS2_HAVE_PGSQL
#include "rts2db/recvals.h"
#endif // RTS2_HAVE_LIBJPEG

namespace rts2xmlrpc
{
#ifdef RTS2_HAVE_LIBJPEG

/**
 * Plot current position of telescope, target and next target position.
 */
class CurrentPosition:public rts2json::GetRequestAuthorized
{
	public:
		CurrentPosition (const char *prefix, rts2json::HTTPServer *_http_server, XmlRpc::XmlRpcServer *s):rts2json::GetRequestAuthorized (prefix, _http_server, "current telescope position", s) {};

		virtual void authorizedExecute (std::string path, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length);
};

#endif /* RTS2_HAVE_LIBJPEG */

#ifdef RTS2_HAVE_PGSQL

#ifdef RTS2_HAVE_LIBJPEG

/**
 * Plot targets on the alt-az graph.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class AltAzTarget: public rts2json::GetRequestAuthorized
{
	public:
		AltAzTarget (const char *prefix, rts2json::HTTPServer *_http_server, XmlRpc::XmlRpcServer *s):rts2json::GetRequestAuthorized (prefix, _http_server, "altitude target graph", s) {};

		virtual void authorizedExecute (std::string path, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length);
};

#endif /* RTS2_HAVE_LIBJPEG */ 

/**
 * Draw graph of variables.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Graph: public rts2json::GetRequestAuthorized
{
	public:
		Graph (const char *prefix, rts2json::HTTPServer *_http_server, XmlRpc::XmlRpcServer *s):rts2json::GetRequestAuthorized (prefix, _http_server, "plot graphs of recorded system values", s) {};

		virtual void authorizedExecute (std::string path, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length);

	private:
		void printDevices (const char* &response_type, char* &response, size_t &response_length);

		void plotValue (const char *device, const char *value, double from, double to, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length);
		void plotValue (int valId, double from, double to, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length);
		void plotValue (rts2db::Recval *rv, double from, double to, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length);
};

#endif /* RTS2_HAVE_PGSQL */

}
