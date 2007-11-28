/* 
 * XML-RPC daemon.
 * Copyright (C) 2007 Petr Kubanek <petr@kubanek.net>
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

#include "../utilsdb/rts2devicedb.h"
#include "../utilsdb/rts2targetset.h"
#include "../utilsdb/rts2obsset.h"
#include "../utilsdb/rts2imgset.h"
#include "../writers/rts2imagedb.h"
#include "../utils/libnova_cpp.h"
#include "../utils/timestamp.h"
#include "xmlrpc++/XmlRpc.h"
#include "xmlstream.h"

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
	rpcPort = 8889;
	addOption ('p', NULL, 1, "XML-RPC port. Default to 8889");
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

					Rts2Value *val = *variter;

					switch (val->getValueBaseType ())
					{
						case RTS2_VALUE_INTEGER:
							int int_val;
							int_val = (*variter)->getValueInteger ();
							retVar["value"] = int_val;
							break;
						case RTS2_VALUE_DOUBLE:
							double value;
							value = (*variter)->getValueDouble ();
							if (isnan (value))
								retVar["value"] = "NaN";
							else
								retVar["value"] = value;
							break;
						case RTS2_VALUE_FLOAT:
							float float_val;
							float_val = (*variter)->getValueFloat ();
							if (isnan (value))
								retVar["value"] = "NaN";
							else
								retVar["value"] = value;
							break;
						case RTS2_VALUE_LONGINT:
							int_val = (*variter)->getValueInteger ();
							retVar["value"] = int_val;
							break;
						case RTS2_VALUE_TIME:
							int_val = (*variter)->getValueInteger ();
							retVar["value"] = int_val;
							break;
						default:
							retVar["value"] = (*variter)->getValue ();
							break;
					}
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

/*
 *
 */

class ListTargets: public XmlRpcServerMethod
{
	public:
		ListTargets (XmlRpcServer* s) : XmlRpcServerMethod ("system.listTargets", s) {}

		void execute (XmlRpcValue& params, XmlRpcValue& result)
		{
			Rts2TargetSet *tar_set;
			tar_set = new Rts2TargetSet ();
			double value;
			int i = 0;
			XmlRpcValue retVar;

			for (Rts2TargetSet::iterator tar_iter = tar_set->begin(); tar_iter != tar_set->end (); tar_iter++, i++)
			{
				std::cout << "ID " << (*tar_iter)->getTargetID () << std::endl;
				retVar["id"] = (*tar_iter)->getTargetID ();
				retVar["type"] = (*tar_iter)->getTargetType ();
				if ((*tar_iter)->getTargetName ())
					retVar["name"] = (*tar_iter)->getTargetName ();
				else
					retVar["name"] = "NULL";

				retVar["enabled"] = (*tar_iter)->getTargetEnabled ();
				if ((*tar_iter)->getTargetComment ())
					retVar["comment"] = (*tar_iter)->getTargetComment ();
				else
					retVar["comment"] = "";
				value = (*tar_iter)->getLastObs();
				if (value == value) retVar["last"] = value;
				else retVar["last"] = "NaN";

				for (int j = 0; j < params.size(); j++)
				{
					char type = (((std::string)params[j]).c_str())[0];
					//	if (type == ' ')
					//		result[i++] = retVar;	// ask for all targets
					//	else
					//	{
					std::cout << "types: " << (*tar_iter)->getTargetType () << type << std::endl;
					if ((*tar_iter)->getTargetType() == type)
								 // only one type
						result[i++] = retVar;
					//	}
				}

				//else
				//	result[i++] = retVar;	// no parameter, send all targets

			}
		}

		std::string help ()
		{
			return std::string ("Returns all targets");
		}

} listTargets (&xmlrpc_server);

class TargetInfo: public XmlRpcServerMethod
{
	public:
		TargetInfo (XmlRpcServer* s) : XmlRpcServerMethod ("system.targetInfo", s) {}

		void execute (XmlRpcValue& params, XmlRpcValue& result)
		{
			std::list < int >targets;
			XmlRpcValue retVar;
			int i;
			double JD;
			time_t now;

			for (i = 0; i < params.size(); i++)
				targets.push_back (params[i]);

			Rts2TargetSet *tar_set;
			tar_set = new Rts2TargetSet (targets);

			i = 0;

			for (Rts2TargetSet::iterator tar_iter = tar_set->begin(); tar_iter != tar_set->end (); tar_iter++)
			{
				JD = ln_get_julian_from_sys ();

				ln_get_timet_from_julian (JD, &now);

				XmlStream xs (&retVar);
				xs << *(*tar_iter);

				// observations
				Rts2ObsSet *obs_set;
				obs_set = new Rts2ObsSet ((*tar_iter)->getTargetID ());
				int j = 0;
				for (Rts2ObsSet::iterator obs_iter = obs_set->begin(); obs_iter != obs_set->end(); obs_iter++, j++)
				{
					retVar["observation"][j] =  (*obs_iter).getObsId();
					// retVar["observation"][j]["images"] = (*obs_iter).getNumberOfImages();
				}

				/*
					XmlStream xs (&retVar);

					// xs << InfoVal<XmlRpcValue> ("HOUR", (*tar_iter)->getHourAngle (JD));
					Target *tar = *tar_iter;
					tar->sendInfo (xs, JD);
				*/
				result[i++] = retVar;
			}
		}

} targetInfo (&xmlrpc_server);

class ListObservations: public XmlRpcServerMethod
{
	public:
		ListObservations (XmlRpcServer* s) : XmlRpcServerMethod ("system.listObservations", s) {}

		void execute (XmlRpcValue& params, XmlRpcValue& result)
		{
			XmlRpcValue retVar;
			Rts2ObsSet *obs_set;
			obs_set = new Rts2ObsSet ((int)params[0]);
			int i = 0;
			double value;
			for (Rts2ObsSet::iterator obs_iter = obs_set->begin(); obs_iter != obs_set->end(); obs_iter++, i++)
			{
				retVar["id"] = (*obs_iter).getObsId();
				/**/
				value  = (*obs_iter).getObsRa();
				if (value == value) retVar["obs_ra"] = value;
				else retVar["obs_ra"] = "NaN";
				/**/
				value  = (*obs_iter).getObsDec();
				if (value == value) retVar["obs_dec"] = value;
				else retVar["obs_dec"] = "NaN";
				/**/
				value = (*obs_iter).getObsStart();
				if (isnan (value))
					retVar["obs_start"] = "NaN";
				else
					retVar["obs_start"] = value;
				/**/
				value = (*obs_iter).getObsEnd();
				if (value == value) retVar["obs_end"] = value;
				else retVar["obs_end"] = "NaN";
				/**/
				// value = (*obs_iter).getNumberOfImages();
				// if (value == value) retVar["obs_images"] = value;
				// else retVar["obs_images"] = "NaN";

				result[i] = retVar;
			}
		}

} listObservations (&xmlrpc_server);

class ListImages: public XmlRpcServerMethod
{
	public:
		ListImages (XmlRpcServer* s) : XmlRpcServerMethod ("system.listImages", s) {}

		void execute (XmlRpcValue& params, XmlRpcValue& result)
		{
			XmlRpcValue retVar;
			Rts2Obs *obs;
			obs = new Rts2Obs ((int)params[0]);
			if (obs->loadImages ())
				return;
			Rts2ImgSet *img_set = obs->getImageSet();
			int i = 0;
			for (Rts2ImgSet::iterator img_iter = img_set->begin(); img_iter != img_set->end(); img_iter++)
			{
				Rts2Image *image = *img_iter;
				retVar["filename"] = image->getImageName ();
				result[i++] = retVar;
			}
		}
} listImages (&xmlrpc_server);

class GetMessages: public XmlRpcServerMethod
{
	public:
		GetMessages (XmlRpcServer* s) : XmlRpcServerMethod ("system.getMessages", s) {}

		void execute (XmlRpcValue& params, XmlRpcValue& result)
		{
			int i = 0;
			XmlRpcValue retVar;
			std::list < Rts2Message > messages;
			// setMessageMask (MESSAGE_MASK_ALL);

			for (std::list < Rts2Message >::iterator iter = messages.begin (); iter != messages.end (); iter++, i++)
			{
				Rts2Message msg = *iter;
				retVar[i] = msg.toString ().c_str ();
				result[i] = retVar;
			}

		}
} getMessages (&xmlrpc_server);

int
main (int argc, char **argv)
{
	Rts2XmlRpcd device = Rts2XmlRpcd (argc, argv);
	return device.run ();
}
