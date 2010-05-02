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
#include "r2x.h"

#include "../utils/hoststring.h"
#include "../utils/rts2daemon.h"

using namespace rts2xmlrpc;

void BBServer::sendUpdate (XmlRpcValue *data)
{
	if (xmlClient == NULL)
	{
		HostString serverUrl (_serverName.c_str ());
		xmlClient = new XmlRpcClient (serverUrl.getHostname (), serverUrl.getPort (), "rts2:rts2");
	}

	XmlRpcValue send;
	send["observatory"] = "BOOTES";
	send["data"] = *data;

	XmlRpcValue result;

	int ret = xmlClient->execute (R2X_BB_UPDATE_OBSERVATORY, send, result);
	if (!ret)
		logStream (MESSAGE_ERROR) << "error calling " R2X_BB_UPDATE_OBSERVATORY << sendLog;
}

void BBServers::sendUpdate (XmlRpcValue *values)
{
	for (BBServers::iterator iter = begin (); iter != end (); iter++)
		iter->sendUpdate (values);
}
