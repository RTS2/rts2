/* 
 * XML-RPC daemon.
 * Copyright (C) 2007-2009 Petr Kubanek <petr@kubanek.net>
 * Copyright (C) 2007 Stanislav Vitek <standa@iaa.es>
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

#ifndef __RTS2_XMLRPCD__
#define __RTS2_XMLRPCD__

#include "config.h"
#include <deque>

#ifdef HAVE_PGSQL
#include "../utilsdb/rts2devicedb.h"
#else
#include "../utils/rts2config.h"
#include "../utils/rts2device.h"
#endif /* HAVE_PGSQL */

#include "events.h"
#include "httpreq.h"
#include "session.h"
#include "xmlrpc++/XmlRpc.h"

#define OPT_STATE_CHANGE            OPT_LOCAL + 76


#define EVENT_XMLRPC_VALUE_TIMER    RTS2_LOCAL_EVENT + 850

using namespace XmlRpc;

/**
 * @file
 * XML-RPC access to RTS2.
 *
 * @defgroup XMLRPC XML-RPC
 */

namespace rts2xmlrpc
{

/**
 * XML-RPC client class. Provides functions for XML-RPCd to react on state
 * and value changes.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 *
 * @addgroup XMLRPC
 */
class XmlDevClient:public Rts2DevClient
{
	public:
		XmlDevClient (Rts2Conn *conn):Rts2DevClient (conn)
		{

		}

		virtual void stateChanged (Rts2ServerState * state);

		virtual void valueChanged (Rts2Value * value);
};

/**
 * XML-RPC daemon class.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 *
 * @addgroup XMLRPC
 */
#ifdef HAVE_PGSQL
class XmlRpcd:public Rts2DeviceDb
#else
class XmlRpcd:public Rts2Device
#endif
{
	public:
		XmlRpcd (int argc, char **argv);
		virtual ~XmlRpcd ();

		virtual Rts2DevClient *createOtherType (Rts2Conn * conn, int other_device_type);

		void stateChangedEvent (Rts2Conn *conn, Rts2ServerState *new_state);

		void valueChangedEvent (Rts2Conn *conn, Rts2Value *new_value);

		virtual void message (Rts2Message & msg);

		/**
		 * Create new session for given user.
		 *
		 * @param _username  Name of the user.
		 * @param _timeout   Timeout in seconds for session validity.
		 *
		 * @return String with session ID.
		 */
		std::string addSession (std::string _username, time_t _timeout);


		/**
		 * Returns true if session with a given sessionId exists.
		 *
		 * @param sessionId  Session ID.
		 *
		 * @return True if session with a given session ID exists.
		 */
		bool existsSession (std::string sessionId);

		/**
		 * If XmlRpcd is allowed to send emails.
		 */
		bool sendEmails () { return send_emails->getValueBool (); }

		/**
		 * Returns messages buffer.
		 */
		std::deque <Rts2Message> & getMessages () { return messages; }

		/**
		 * Return prefix for generated pages - usefull for pages behind proxy.
		 */
		const char* getPagePrefix () { return page_prefix.c_str (); }

		/**
		 * Returns true, if given path is marked as being public - accessible to all.
		 */
		bool isPublic (const std::string &path) { return events.isPublic (path); }

	protected:
#ifndef HAVE_PGSQL
		virtual int willConnect (Rts2Address * _addr);
#endif
		virtual int processOption (int in_opt);
		virtual int init ();
		virtual void addSelectSocks ();
		virtual void selectSuccess ();

		virtual void signaledHUP ();

	private:
		int rpcPort;
		const char *stateChangeFile;
		std::map <std::string, Session*> sessions;

		std::deque <Rts2Message> messages;

		std::vector <Directory *> directories;

		Events events;

		Rts2ValueBool *send_emails;

#ifndef HAVE_PGSQL
		const char *config_file;
#endif

		std::string page_prefix;
};

};

#endif /* __RTS2_XMLRPCD__ */
