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

#include "tsqueue.h"

#include "xmlrpc++/XmlRpcValue.h"
#include "xmlrpc++/XmlRpcClient.h"

#include <vector>
#include <pthread.h>

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
		BBServer (XmlRpcd *_server, char *serverApi, int observatoryId):_serverApi (serverApi), _observatoryId(observatoryId)
		{
			server = _server;
			client = NULL;
			_uri = NULL;
			send_thread = 0;
		}

		~BBServer ()
		{
			pthread_cancel (send_thread);
			delete client;
		}

		/**
		 * Sends update message to BB server. Data part is specified in data parameter.
		 *
		 * @param data  data to send to BB server
		 */
		void sendUpdate ();

		void queueUpdate ();

		size_t queueSize () { return requests.size (); }

		TSQueue <int> requests;

	private:
		std::string _serverApi;
		int _observatoryId;
		const char *_uri;

		XmlRpcClient *client;
		XmlRpcd *server;

		pthread_t send_thread;
};


class BBServers:public std::vector <BBServer>
{
	public:
		BBServers (): std::vector <BBServer> () {}

		void sendUpdate ();

		size_t queueSize ();
};

}

#endif // !__RTS2__BBSERVER__
