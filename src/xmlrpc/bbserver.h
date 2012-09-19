/* 
 * Big Brother server connector.
 * Copyright (C) 2010 Petr Kubánek <petr@kubanek.net>
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

#ifndef __RTS2__BBSERVER__
#define __RTS2__BBSERVER__

#include "rts2-config.h"

#ifdef RTS2_JSONSOUP

#include "xmlrpc++/XmlRpcValue.h"
#include "xmlrpc++/XmlRpcClient.h"

#include <vector>

#include <glib-object.h>
#include <json-glib/json-glib.h>
#include <libsoup/soup.h>

using namespace XmlRpc;

namespace rts2xmlrpc
{

class XmlRpcd;

/**
 * Big Brother server.
 *
 * @author Petr Kubánek <petr@kubanek.net>
 */
class BBServer
{
	public:
		BBServer (char *serverApi, const char *observatoryName):_serverApi (serverApi), _observatoryName(observatoryName) {session = NULL; }
		~BBServer () {}

		/**
		 * Sends update message to BB server. Data part is specified in data parameter.
		 *
		 * @param data  data to send to BB server
		 */
		void sendUpdate (XmlRpcd *server);

	private:
		std::string _serverApi;
		std::string _observatoryName;

		SoupSession *session;
		SoupLogger *logger;
};


class BBServers:public std::vector <BBServer>
{
	public:
		BBServers (): std::vector <BBServer> () {}

		void sendUpdate (XmlRpcd *server);
};

}

#endif // RTS2_JSONSOUP

#endif // !__RTS2__BBSERVER__
