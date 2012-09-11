/* 
 * Classes to plot graph interface.
 * Copyright (C) 2009-2010 Petr Kubanek <petr@kubanek.net>
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

#include "httpreq.h"

#ifdef RTS2_HAVE_LIBJPEG

namespace rts2xmlrpc
{

/**
 * Draw graph of variables.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Graph: public GetRequestAuthorized
{
	public:
		Graph (const char *prefix, XmlRpc::XmlRpcServer *s):GetRequestAuthorized (prefix, "plot graphs of recorded system values", s) {};

		virtual void authorizedExecute (std::string path, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length);

	private:
		void printDevices (const char* &response_type, char* &response, size_t &response_length);

		void plotValue (const char *device, const char *value, double from, double to, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length);
		void plotValue (int valId, double from, double to, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length);
		void plotValue (rts2db::Recval *rv, double from, double to, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length);
};

}

#endif // RTS2_HAVE_LIBJPEG
