/**
 * RTS2 Big Brother server.
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

#ifndef __RTS2_BB__
#define __RTS2_BB__

#include "bbapi.h"
#include "bbtasks.h"
#include "schedreq.h"
#include "rts2db/devicedb.h"
#include "rts2db/user.h"
#include "xmlrpc++/XmlRpc.h"

#include "rts2json/images.h"
#include "rts2json/libjavascript.h"
#include "rts2json/libcss.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>

#define EVENT_TASK_SCHEDULE       1500
#define EVENT_SCHEDULING_DONE     1501

using namespace XmlRpc;

namespace rts2bb
{

/**
 * Big Brother ("Bridgite Bordoux" - pick what you like)
 * master class.
 *
 * @author Petr Kubánek <petr@kubanek.net>
 */
class BB:public rts2db::DeviceDb, XmlRpc::XmlRpcServer, rts2json::HTTPServer
{
	public:
		BB (int argc, char **argv);

		virtual void postEvent (rts2core::Event *event);

		void update (XmlRpcValue &value);

		virtual bool isPublic (struct sockaddr_in *saddr, const std::string &path) { return false; }
		virtual bool existsSession (std::string sessionId) { return false; }
		virtual void addExecutedPage () {}
		virtual const char* getPagePrefix () { return ""; }

		virtual bool getDebug () { return rts2db::DeviceDb::getDebug (); }

		virtual void sendValueAll (rts2core::Value * value) { rts2db::DeviceDb::sendValueAll (value); }

		virtual rts2db::CamList *getCameras () { return &(rts2db::DeviceDb::cameras); }

		virtual rts2core::connections_t *getConnections () { return rts2db::DeviceDb::getConnections (); }

		virtual void getOpenConnectionType (int deviceType, rts2core::connections_t::iterator &current) { return getOpenConnectionType (deviceType, current); }

		virtual bool verifyDBUser (std::string username, std::string pass, bool &executePermission);

		bool getDebugConn () { return debugConn->getValueBool (); }

	protected:
		virtual int processOption (int opt);

		virtual int init ();
		virtual int info ();

		virtual int idle ();

		virtual void addSelectSocks (fd_set &read_set, fd_set &write_set, fd_set &exp_set);
		virtual void selectSuccess (fd_set &read_set, fd_set &write_set, fd_set &exp_set);

	private:
		int rpcPort;
		BBAPI bbApi;
		SchedReq sched;

		rts2json::ImageReq imageReq;
		rts2json::LibJavaScript javaScriptRequests;
		rts2json::LibCSS cssRequests;

		rts2core::ValueBool *debugConn;
		rts2core::ValueInteger *queueSize;

		BBTasks task_queue;

		void processSchedule (ObservatorySchedule *obs_sched);
};

}

#endif // __RTS2_BB__
