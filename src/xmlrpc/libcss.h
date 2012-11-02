/* 
 * CSS for AJAX web access.
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

#ifndef __RTS2_LIBCSS__
#define __RTS2_LIBCSS__

#include "rts2json/httpreq.h"

namespace rts2xmlrpc
{

/**
 * Pages with CSS.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class LibCSS: public rts2json::GetRequestAuthorized
{
	public:
		LibCSS (const char* prefix, rts2json::HTTPServer *_http_server, XmlRpc::XmlRpcServer* s):rts2json::GetRequestAuthorized (prefix, _http_server, NULL, s) {}

		virtual void authorizedExecute (XmlRpc::XmlRpcSource *source, std::string path, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length);
};

}

#endif // !__RTS2_LIBCSS__
