/* 
 * Images for web interface.
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

#include "rts2json/httpreq.h"

class ImageReq: public XmlRpc::XmlRpcServerGetRequest
{
	public:
		ImageReq (const char* prefix, XmlRpc::XmlRpcServer* s = 0):XmlRpcServerGetRequest (prefix, NULL, s) {}

		virtual void execute (XmlRpc::XmlRpcSource *source, struct sockaddr_in *saddr, std::string path, XmlRpc::HttpParams *params, int &http_code, const char* &response_type, char* &response, size_t &response_length);
};
