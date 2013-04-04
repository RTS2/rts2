/* 
 * Abstract class for server implementation.
 * Copyright (C) 2013 Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
 * Copyright (C) 2012 Petr Kubanek <petr@kubanek.net>
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

#include "rts2json/asyncapi.h"
#include "rts2json/httpserver.h"

using namespace rts2json;

void HTTPServer::registerAPI (AsyncAPI *a)
{
	asyncAPIs.push_back (a);
	if (sumAsync)
	{
		sumAsync->inc ();
		sendValueAll (sumAsync);
	}
	if (numberAsyncAPIs)
	{
		numberAsyncAPIs->setValueInteger (asyncAPIs.size ());
		sendValueAll (numberAsyncAPIs);
	}
}

void HTTPServer::asyncIdle ()
{
	// delete freed async, check for shared memory data
	for (std::list <rts2json::AsyncAPI *>::iterator iter = asyncAPIs.begin (); iter != asyncAPIs.end ();)
	{
		if ((*iter)->idle ())
		{
			delete *iter;
			iter = asyncAPIs.erase (iter);
			numberAsyncAPIs->setValueInteger (asyncAPIs.size ());
			sendValueAll (numberAsyncAPIs);
		}
		else
		{
			iter++;
		}
	}
}
