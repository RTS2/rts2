/* 
 * Classes for generating pages with Auger triggers.
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

#include "rts2-config.h"
#include "httpreq.h"

#ifdef RTS2_HAVE_PGSQL
#include "xmlrpc++/XmlRpc.h"

namespace rts2xmlrpc
{

/**
 * Browse and prints informations about auger showers.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */ 
class Auger: public GetRequestAuthorized
{
	public:
		Auger (const char *prefix, XmlRpc::XmlRpcServer *s):GetRequestAuthorized (prefix, "access to auger shower data", s) {};

		virtual void authorizedExecute (std::string path, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length);
	private:
		void printTarget (int auger_id, const char* &response_type, char* &response, size_t &response_length);

		void listAuger (int year, int month, int day, std::ostringstream &_os);

		void printTable (int year, int month, int day, char* &response, size_t &response_length);

};

}

#endif /* RTS2_HAVE_PGSQL */
