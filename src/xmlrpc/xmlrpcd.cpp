/* 
 * XML-RPC daemon.
 * Copyright (C) 2007 Petr Kubanek <petr@kubanek.net>
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

#include "../utilsdb/rts2devicedb.h"
#include "xmlrpc++/XmlRpc.h"

using namespace XmlRpc;

/**
 * @file
 * XML-RPC access to RTS2.
 *
 * @defgroup XMLRPC XML-RPC
 */

/**
 * XML-RPC server.
 */
XmlRpcServer xmlrpc_server;

/**
 * XML-RPC
 *
 * @author Petr Kubanek <petr@kubanek.net>
 *
 * @addgroup XMLRPC
 */
class Rts2XmlRpcd:public Rts2DeviceDb
{
	private:
		int rpcPort;
	protected:
		virtual int processOption (int in_opt);
		virtual int init ();
		virtual void addSelectSocks ();
		virtual void selectSuccess ();

	public:
		Rts2XmlRpcd (int in_argc, char **in_argv);
};

int
Rts2XmlRpcd::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'p':
			rpcPort = atoi (optarg);
			break;
		default:
			return Rts2DeviceDb::processOption (in_opt);
	}
	return 0;
}


int
Rts2XmlRpcd::init ()
{
	int ret;
	ret = Rts2DeviceDb::init ();
	if (ret)
		return ret;

	if (printDebug ())
		XmlRpc::setVerbosity (5);

	xmlrpc_server.bindAndListen (rpcPort);
	xmlrpc_server.enableIntrospection (true);

	return ret;
}


void
Rts2XmlRpcd::addSelectSocks ()
{
	Rts2DeviceDb::addSelectSocks ();
	xmlrpc_server.addToFd (&read_set, &write_set, &exp_set);
}


void
Rts2XmlRpcd::selectSuccess ()
{
	Rts2DeviceDb::selectSuccess ();
	xmlrpc_server.checkFd (&read_set, &write_set, &exp_set);
}


Rts2XmlRpcd::Rts2XmlRpcd (int in_argc, char **in_argv): Rts2DeviceDb (in_argc, in_argv, DEVICE_TYPE_SOAP, "XMLRPC")
{
	rpcPort = 8888;
	addOption ('p', NULL, 1, "XML-RPC port. Default to 8888");
	XmlRpc::setVerbosity (0);
}


/**
 * Returns number of devices connected to the system.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 *
 * @addgroup XMLRPC
 */
class DeviceCount: public XmlRpcServerMethod
{
	public:
		DeviceCount (XmlRpcServer* s) : XmlRpcServerMethod ("DeviceCount", s)
		{
		}

		void execute (XmlRpcValue& params, XmlRpcValue& result)
		{
			result = ((Rts2XmlRpcd *) getMasterApp ())->connectionSize ();
		}

		std::string help ()
		{
			return std::string ("Returns number of devices connected to XMLRPC");
		}
} deviceSum (&xmlrpc_server);

/**
 * List name of devices connected to the server.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 *
 * @addgroup XMLRPC
 */
class ListDevices: public XmlRpcServerMethod
{
	public:
		ListDevices (XmlRpcServer* s) : XmlRpcServerMethod ("system.listDevices", s)
		{
		}

		void execute (XmlRpcValue& params, XmlRpcValue& result)
		{
			Rts2XmlRpcd *serv = (Rts2XmlRpcd *) getMasterApp ();
			int i = 0;
			for (connections_t::iterator iter = serv->connectionBegin (); iter != serv->connectionEnd (); iter++, i++)
			{
				result[i] = (*iter)->getName ();
			}
		}

		std::string help ()
		{
			return std::string ("Returns name of devices conencted to the system");
		}
} listDevices (&xmlrpc_server);

/**
 * List name of all values accessible from server.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 *
 * @addgroup XMLRPC
 */
class ListValues: public XmlRpcServerMethod
{
	protected:
		ListValues (const char *in_name, XmlRpcServer* s): XmlRpcServerMethod (in_name, s)
		{
		}

	public:
		ListValues (XmlRpcServer* s): XmlRpcServerMethod ("system.listValues", s)
		{
		}

		void execute (XmlRpcValue& params, XmlRpcValue& result)
		{
			Rts2XmlRpcd *serv = (Rts2XmlRpcd *) getMasterApp ();
			Rts2Conn *conn;
			int i = 0;
			// print results for single device..
			for (connections_t::iterator iter = serv->connectionBegin (); iter != serv->connectionEnd (); iter++)
			{
				conn = *iter;
				std::string deviceName = (*iter)->getName ();
				// filter device list
				for (Rts2ValueVector::iterator variter = conn->valueBegin (); variter != conn->valueEnd (); variter++, i++)
				{
					result[i] = deviceName + "." + (*variter)->getName ();
				}
			}
		}

		std::string help ()
		{
			return std::string ("Returns name of devices conencted to the system");
		}
} listValues (&xmlrpc_server);

/**
 * List name of all values accessible from server.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 *
 * @addgroup XMLRPC
 */
class ListValuesDevice: public ListValues
{
	public:
		ListValuesDevice (XmlRpcServer* s): ListValues ("system.listValuesDevice", s)
		{
		}

		void execute (XmlRpcValue& params, XmlRpcValue& result)
		{
			Rts2XmlRpcd *serv = (Rts2XmlRpcd *) getMasterApp ();
			Rts2Conn *conn;
			int i = 0;
			// print results for single device..
			if (params.size() == 1)
			{
				conn = serv->getOpenConnection (((std::string)params[0]).c_str());
				if (!conn)
				{
					throw XmlRpcException ("Cannot get device " + (std::string) params[0]);
				}
				for (Rts2ValueVector::iterator variter = conn->valueBegin (); variter != conn->valueEnd (); variter++, i++)
				{
					XmlRpcValue retVar;
					retVar["name"] = (*variter)->getName ();
					retVar["value"] = (*variter)->getValue ();
					result[i] = retVar;
				}
			}
			// print from all
			else for (connections_t::iterator iter = serv->connectionBegin (); iter != serv->connectionEnd (); iter++)
			{
				conn = *iter;
				std::string deviceName = (*iter)->getName ();
				// filter device list
				int v;
				for (v = 0; v < params.size (); v++)
				{
					if (((std::string) params[v]) == deviceName)
						break;
				}
				if (v == params.size ())
					continue;
				for (Rts2ValueVector::iterator variter = conn->valueBegin (); variter != conn->valueEnd (); variter++, i++)
				{
					result[i] = deviceName + "." + (*variter)->getName ();
				}
			}
		}

		std::string help ()
		{
			return std::string ("Returns name of devices conencted to the system for given device(s)");
		}
} listValuesDevice (&xmlrpc_server);

int
main (int argc, char **argv)
{
	Rts2XmlRpcd device = Rts2XmlRpcd (argc, argv);
	return device.run ();
}
