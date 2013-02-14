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

#include "bbserver.h"
#include "xmlrpc++/urlencoding.h"
#include "xmlrpcd.h"

#include "hoststring.h"
#include "daemon.h"

using namespace rts2xmlrpc;

// thread routine
void *updateBB (void *arg)
{
	BBServer *bbserver = (BBServer *) arg;

	while (true)
	{
		switch (bbserver->requests.pop (true))
		{
			case 1:
				bbserver->sendUpdate ();
				break;
		}
	}
	return NULL;
}

void BBServer::postEvent (rts2core::Event *event)
{
	switch (event->getType ())
	{
		case EVENT_XMLRPC_BB:
			sendUpdate ();
			server->addTimer (getCadency (), event);
			return;
	}
	rts2core::Object::postEvent (event);
}

void BBServer::sendUpdate ()
{
	if (client == NULL)
	{
		client = new XmlRpcClient (serverApi.c_str (), &_uri);
		if (password.length ())
		{
			std::ostringstream auth;
			auth << observatoryId << ":" << password;
			client->setAuthorization (auth.str ().c_str ());
		}
	}

	std::ostringstream body;

	bool first = true;

	body << "{";

	for (rts2core::connections_t::iterator iter = server->getConnections ()->begin (); iter != server->getConnections ()->end (); iter++)
	{
		if ((*iter)->getName ()[0] == '\0')
			continue;
		if (first)
			first = false;
		else
			body << ",";
		body << "\"" << (*iter)->getName () << "\":{";
		rts2json::sendConnectionValues (body, *iter, NULL, NAN, true);
		body << "}";
	}

	body << "}";

	char * reply;
	int reply_length;

	std::ostringstream url;
	url << _uri << "/api/observatory?observatory_id=" << observatoryId;

	int ret = client->executePostRequest (url.str ().c_str (), body.str ().c_str (), reply, reply_length);
	if (!ret)
	{
		logStream (MESSAGE_ERROR) << "Error requesting " << serverApi.c_str () << sendLog;
		delete client;
		client = NULL;
		return;
	}

	server->bbSend (atof (reply));

	delete[] reply;
}

void BBServer::queueUpdate ()
{
	if (send_thread == 0)
	{
		pthread_create (&send_thread, NULL, updateBB, (void *) this);
	}
	requests.push (1);
}

void BBServers::sendUpdate ()
{
	for (BBServers::iterator iter = begin (); iter != end (); iter++)
		iter->queueUpdate ();
}

size_t BBServers::queueSize ()
{
	size_t ret = 0;
	for (BBServers::iterator iter = begin (); iter != end (); iter++)
		ret += iter->queueSize ();
	
	return ret;
}
