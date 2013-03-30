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

#include "object.h"
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
 * Big Brother server communication.
 *
 * @author Petr Kubánek <petr@kubanek.net>
 */
class BBServer:public rts2core::Object
{
	public:
		BBServer (XmlRpcd *_server, char *_serverApi, int _observatoryId, char *_password, int _cadency);
		~BBServer ();

		virtual void postEvent (rts2core::Event *event);

		XmlRpc::XmlRpcClient *createClient ();

		/**
		 * Sends update message to BB server. Data part is specified in data parameter.
		 *
		 * @param data  data to send to BB server
		 */
		void sendUpdate ();

		void sendObservationUpdate (int observationId);

		/**
		 * Add observatory update to the queue. Positive IDs are for observation update (representing obs_id), 
		 * negative IDs are as follow:
		 *  - -1 to send BB observatory update (as JSON)
		 */
		void queueUpdate (int reqId);

		size_t queueSize () { return requests.size (); }

		TSQueue <int> requests;

		int getCadency () { return cadency; }

		void setCadency (int _cadency) { cadency = _cadency; }

		bool isObservatory (int _observatoryId) { return observatoryId == _observatoryId; }

		int getObservatoryId () { return observatoryId; }

	private:
		std::string serverApi;
		int observatoryId;
		std::string password;
		const char *_uri;

		XmlRpcClient *client;
		XmlRpcd *server;

		pthread_t send_thread;
		pthread_t push_thread;

		int cadency;
};

class BBServers:public std::vector <BBServer>
{
	public:
		BBServers (): std::vector <BBServer> () {}

		void sendUpdate ();
		void sendObservatoryUpdate (int observatoryId, int request);

		size_t queueSize ();
};

}

#endif // !__RTS2__BBSERVER__
