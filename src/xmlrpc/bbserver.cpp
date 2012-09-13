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

#include "hoststring.h"
#include "daemon.h"

using namespace rts2xmlrpc;

void BBServer::sendUpdate (XmlRpcd *server)
{
	if (session == NULL)
	{
		session = soup_session_sync_new_with_options (
			SOUP_SESSION_ADD_FEATURE_BY_TYPE, SOUP_TYPE_CONTENT_DECODER,
			SOUP_SESSION_ADD_FEATURE_BY_TYPE, SOUP_TYPE_COOKIE_JAR,
			SOUP_SESSION_USER_AGENT, "rts2bb ",
			NULL);

		logger = soup_logger_new (SOUP_LOGGER_LOG_BODY, -1);
		soup_logger_attach (logger, session);
	}

	SoupMessage *msg;
	msg = soup_message_new (SOUP_METHOD_GET, _serverApi.c_str ());
}

void BBServers::sendUpdate (XmlRpcd *server)
{
	for (BBServers::iterator iter = begin (); iter != end (); iter++)
		iter->sendUpdate (server);
}
