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

#include "observatory.h"
#include "rts2db/devicedb.h"
#include "xmlrpc++/XmlRpc.h"

using namespace XmlRpc;

namespace rts2bb
{

/**
 * Big Brother ("Bridgite Bordoux" - pick what you like)
 * master class.
 *
 * @author Petr Kubánek <petr@kubanek.net>
 */
class BB:public rts2db::DeviceDb
{
	public:
		BB (int argc, char **argv);

		void update (XmlRpcValue &value);

		XmlRpcValue *findValues (const std::string &name);

		std::map <std::string, Observatory>::iterator omBegin () { return om.begin (); }
		std::map <std::string, Observatory>::iterator omEnd () { return om.end (); }

	protected:
		virtual int processOption (int opt);

		virtual int init ();

		virtual void addSelectSocks (fd_set &read_set, fd_set &write_set, fd_set &exp_set);
		virtual void selectSuccess (fd_set &read_set, fd_set &write_set, fd_set &exp_set);

	private:
		int rpcPort;
		rts2bb::ObservatoryMap om;
};

/**
 * Represents session methods. Those must be executed either with user name and
 * password, or with "session_id" passed as username and session ID passed in password.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 *
 * @addgroup XMLRPC
 */
class SessionMethod: public XmlRpcServerMethod
{
	public:
		SessionMethod (const char *method, XmlRpcServer* s): XmlRpcServerMethod (method, s)
		{
		}

		void execute (XmlRpcValue& params, XmlRpcValue& result)
		{
			if (! (getUsername() ==  std::string ("rts2") && getPassword() == std::string ("rts2")))
				throw XmlRpcException ("Login not supported");

			sessionExecute (params, result);
		}

		virtual void sessionExecute (XmlRpcValue& params, XmlRpcValue& result) = 0;
};

}

#endif // __RTS2_BB__
