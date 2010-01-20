/* 
 * Classes for generating pages with observations by nights.
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

#include "obsreq.h"
#include "../utilsdb/observation.h"

#ifdef HAVE_PGSQL

using namespace rts2xmlrpc;

void Observation::authorizedExecute (std::string path, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length)
{
	// get path and possibly date range
	std::vector <std::string> vals = SplitStr (path.substr (1), std::string ("/"));

	switch (vals.size ())
	{
		case 0:
			printQuery (response_type, response, response_length);
			return;
		case 1:
			printObs (atoi (vals[0].c_str ()), response_type, response, response_length);
			return;
	}
}

void Observation::printQuery (const char* &response_type, char* &response, size_t &response_length)
{

}

void Observation::printObs (int obs_id, const char* &response_type, char* &response, size_t &response_length)
{
	
}

#endif /* HAVE_PGSQL */
