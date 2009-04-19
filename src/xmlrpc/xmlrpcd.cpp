/* 
 * XML-RPC daemon.
 * Copyright (C) 2007-2008 Petr Kubanek <petr@kubanek.net>
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

#include "config.h"

#ifdef HAVE_PGSQL
#include "../utilsdb/rts2devicedb.h"
#include "../utilsdb/rts2imgset.h"
#include "../utilsdb/rts2obsset.h"
#include "../utilsdb/rts2messagedb.h"
#include "../utilsdb/rts2targetset.h"
#include "../utilsdb/rts2user.h"
#include "../writers/rts2imagedb.h"
#else
#include "../utils/rts2device.h"
#endif /* HAVE_PGSQL */

#include "../utils/libnova_cpp.h"
#include "../utils/timestamp.h"
#include "xmlrpc++/XmlRpc.h"
#include "xmlstream.h"

#include "r2x.h"

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
#ifdef HAVE_PGSQL
class Rts2XmlRpcd:public Rts2DeviceDb
#else
class Rts2XmlRpcd:public Rts2Device
#endif
{
	private:
		int rpcPort;
	protected:
#ifndef HAVE_PGSQL
		virtual int willConnect (Rts2Address * _addr);
#endif
		virtual int processOption (int in_opt);
		virtual int init ();
		virtual void addSelectSocks ();
		virtual void selectSuccess ();

	public:
		Rts2XmlRpcd (int argc, char **argv);

		virtual void message (Rts2Message & msg);
};

#ifndef HAVE_PGSQL
int
Rts2XmlRpcd::willConnect (Rts2Address *_addr)
{
       if (_addr->getType () < getDeviceType ()
                || (_addr->getType () == getDeviceType ()
                && strcmp (_addr->getName (), getDeviceName ()) < 0))
                return 1;
        return 0;
}
#endif

int
Rts2XmlRpcd::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'p':
			rpcPort = atoi (optarg);
			break;
		default:
#ifdef HAVE_PGSQL
			return Rts2DeviceDb::processOption (in_opt);
#else
			return Rts2Device::processOption (in_opt);
#endif
	}
	return 0;
}


int
Rts2XmlRpcd::init ()
{
	int ret;
#ifdef HAVE_PGSQL
	ret = Rts2DeviceDb::init ();
#else
	ret = Rts2Device::init ();
#endif
	if (ret)
		return ret;

	if (printDebug ())
		XmlRpc::setVerbosity (10);

	xmlrpc_server.bindAndListen (rpcPort);
	xmlrpc_server.enableIntrospection (true);

	return ret;
}


void
Rts2XmlRpcd::addSelectSocks ()
{
#ifdef HAVE_PGSQL
	Rts2DeviceDb::addSelectSocks ();
#else
	Rts2Device::addSelectSocks ();
#endif
	xmlrpc_server.addToFd (&read_set, &write_set, &exp_set);
}


void
Rts2XmlRpcd::selectSuccess ()
{
#ifdef HAVE_PGSQL
	Rts2DeviceDb::selectSuccess ();
#else
	Rts2Device::selectSuccess ();
#endif
	xmlrpc_server.checkFd (&read_set, &write_set, &exp_set);
}

#ifdef HAVE_PGSQL
Rts2XmlRpcd::Rts2XmlRpcd (int argc, char **argv): Rts2DeviceDb (argc, argv, DEVICE_TYPE_SOAP, "XMLRPC")
#else
Rts2XmlRpcd::Rts2XmlRpcd (int argc, char **argv): Rts2Device (argc, argv, DEVICE_TYPE_SOAP, "XMLRPC")
#endif
{
	rpcPort = 8889;
	addOption ('p', NULL, 1, "XML-RPC port. Default to 8889");
	XmlRpc::setVerbosity (0);
}


void
Rts2XmlRpcd::message (Rts2Message & msg)
{
// log message to DB, if database is present
#ifdef HAVE_PGSQL
	if (msg.isNotDebug ())
	{
		Rts2MessageDB msgDB (msg);
		msgDB.insertDB ();
	}
#endif
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
		ListDevices (XmlRpcServer* s) : XmlRpcServerMethod (R2X_DEVICES_LIST, s)
		{
		}

		void execute (XmlRpcValue& params, XmlRpcValue& result)
		{
			Rts2XmlRpcd *serv = (Rts2XmlRpcd *) getMasterApp ();
			connections_t::iterator iter;
			int i = 0;
			for (iter = serv->getConnections ()->begin (); iter != serv->getConnections ()->end (); iter++, i++)
			{
				result[i] = (*iter)->getName ();
			}
			for (iter = serv->getConnections ()->begin (); iter != serv->getConnections ()->end (); iter++, i++)
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
		ListValues (XmlRpcServer* s): XmlRpcServerMethod (R2X_VALUES_LIST, s)
		{
		}

		void execute (XmlRpcValue& params, XmlRpcValue& result)
		{
			Rts2XmlRpcd *serv = (Rts2XmlRpcd *) getMasterApp ();
			Rts2Conn *conn;
			connections_t::iterator iter;
			Rts2ValueVector::iterator variter;
			int i = 0;
			// print results for single device..
			for (iter = serv->getCentraldConns ()->begin (); iter != serv->getCentraldConns ()->end (); iter++)
			{
				conn = *iter;
				std::string deviceName = (*iter)->getName ();
				// filter device list
				for (variter = conn->valueBegin (); variter != conn->valueEnd (); variter++, i++)
				{
					result[i] = deviceName + "." + (*variter)->getName ();
				}
			}
			for (iter = serv->getConnections ()->begin (); iter != serv->getConnections ()->end (); iter++)
			{
				conn = *iter;
				std::string deviceName = (*iter)->getName ();
				// filter device list
				for (variter = conn->valueBegin (); variter != conn->valueEnd (); variter++, i++)
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
		ListValuesDevice (XmlRpcServer* s): ListValues (R2X_DEVICES_VALUES_LIST, s)
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
					retVar["flags"] = (*variter)->getFlags ();

					Rts2Value *val = *variter;

					switch (val->getValueBaseType ())
					{
						case RTS2_VALUE_INTEGER:
							int int_val;
							int_val = (*variter)->getValueInteger ();
							retVar["value"] = int_val;
							break;
						case RTS2_VALUE_DOUBLE:
							double dbl_value;
							dbl_value = (*variter)->getValueDouble ();
							retVar["value"] = dbl_value;
							break;
						case RTS2_VALUE_FLOAT:
							float float_val;
							float_val = (*variter)->getValueFloat ();
							retVar["value"] = float_val;
							break;
						case RTS2_VALUE_BOOL:
							bool bool_val;
							bool_val = ((Rts2ValueBool*)(*variter))->getValueBool ();
							retVar["value"] = bool_val;
							break;
						case RTS2_VALUE_LONGINT:
							int_val = (*variter)->getValueInteger ();
							retVar["value"] = int_val;
							break;
						case RTS2_VALUE_TIME:
							struct tm tm_s;
							long usec;
							((Rts2ValueTime*) (*variter))->getStructTm (&tm_s, &usec);
							retVar["value"] = XmlRpcValue (&tm_s);
							break;
						default:
							retVar["value"] = (*variter)->getValue ();
							break;
					}
					result[i] = retVar;
				}
			}
			// print from all
			else
			{
				connections_t::iterator iter;
				Rts2ValueVector::iterator variter;
				for (iter = serv->getCentraldConns ()->begin (); iter != serv->getCentraldConns ()->end (); iter++)
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
					for (variter = conn->valueBegin (); variter != conn->valueEnd (); variter++, i++)
					{
						result[i] = deviceName + "." + (*variter)->getName ();
					}
				}
				for (iter = serv->getConnections ()->begin (); iter != serv->getConnections ()->end (); iter++)
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
					for (variter = conn->valueBegin (); variter != conn->valueEnd (); variter++, i++)
					{
						result[i] = deviceName + "." + (*variter)->getName ();
					}
				}
			}
		}

		std::string help ()
		{
			return std::string ("Returns name of devices conencted to the system for given device(s)");
		}
} listValuesDevice (&xmlrpc_server);

class SetValue: public XmlRpcServerMethod
{
	public:
		SetValue (XmlRpcServer* s) : XmlRpcServerMethod (R2X_VALUE_SET, s) {}

		void execute (XmlRpcValue& params, XmlRpcValue& result)
		{
			std::string devName = params[0];
			std::string valueName = params[1];
			Rts2XmlRpcd *serv = (Rts2XmlRpcd *) getMasterApp ();
			Rts2Conn *conn = serv->getOpenConnection (devName.c_str ());
			if (!conn)
			{
				throw XmlRpcException ("Cannot find connection '" + std::string (devName) + "'.");
			}
			Rts2Value *val = conn->getValue (valueName.c_str ());
			if (!val)
			{
				throw XmlRpcException ("Cannot find value '" + std::string (valueName) + "' on device '" + std::string (devName) + "'.");
			}

			int i_val;
			double d_val;
			std::string s_val;
			XmlRpcValue x_val = params[2];
			switch (val->getValueBaseType ())
			{
				case RTS2_VALUE_INTEGER:
				case RTS2_VALUE_LONGINT:
					if (x_val.getType () == XmlRpcValue::TypeInt)
					{
						i_val = (int) (x_val);
					}
					else
					{
						s_val = (std::string) (x_val);
						i_val = atoi (s_val.c_str ());
					}
					conn->queCommand (new Rts2CommandChangeValue (conn->getOtherDevClient (), valueName, '=', i_val));
					break;
				case RTS2_VALUE_DOUBLE:
					if (x_val.getType () == XmlRpcValue::TypeDouble)
					{
						d_val = (double) (x_val);
					}
					else
					{
						s_val = (std::string) (x_val);
						d_val = atof (s_val.c_str ());
					}
					conn->queCommand (new Rts2CommandChangeValue (conn->getOtherDevClient (), valueName, '=', d_val));
					break;

				case RTS2_VALUE_FLOAT:
					if (x_val.getType () == XmlRpcValue::TypeDouble)
					{
						d_val = (double) (x_val);
					}
					else
					{
						s_val = (std::string) (x_val);
						d_val = atof (s_val.c_str ());
					}
					conn->queCommand (new Rts2CommandChangeValue (conn->getOtherDevClient (), valueName, '=', (float) d_val));
					break;
				case RTS2_VALUE_STRING:
					conn->queCommand (new Rts2CommandChangeValue (conn->getOtherDevClient (), valueName, '=', (std::string) (params[2])));
					break;
				default:
					conn->queCommand (new Rts2CommandChangeValue (conn->getOtherDevClient (), valueName, '=', (std::string) (params[2]), true));
					break;
			}
		}

		std::string help ()
		{
			return std::string ("Set RTS2 value");
		}

} setValue (&xmlrpc_server);


#ifdef HAVE_PGSQL
/*
 *
 */
class ListTargets: public XmlRpcServerMethod
{
	public:
		ListTargets (XmlRpcServer* s) : XmlRpcServerMethod (R2X_TARGETS_LIST, s) {}

		void execute (XmlRpcValue& params, XmlRpcValue& result)
		{
			Rts2TargetSet *tar_set = new Rts2TargetSet ();
			double value;
			int i = 0;
			XmlRpcValue retVar;

			for (Rts2TargetSet::iterator tar_iter = tar_set->begin(); tar_iter != tar_set->end (); tar_iter++, i++)
			{
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
				retVar["last_obs"] = value;
				struct ln_equ_posn pos;
				(*tar_iter)->getPosition (&pos, ln_get_julian_from_sys ());
				retVar["ra"] = pos.ra;
				retVar["dec"] = pos.dec;
				result[i++] = retVar;
			}
		}

		std::string help ()
		{
			return std::string ("Returns all targets");
		}

} listTargets (&xmlrpc_server);


class ListTargetsByType: public XmlRpcServerMethod
{
	public:
		ListTargetsByType (XmlRpcServer* s) : XmlRpcServerMethod (R2X_TARGETS_TYPE_LIST, s) {}

		void execute (XmlRpcValue& params, XmlRpcValue& result)
		{
			char target_types[params.size ()+1];
			int j;
			for (j = 0; j < params.size (); j++)
				target_types[j] = *(((std::string)params[j]).c_str());
			target_types[j] = '\0';

			Rts2TargetSet *tar_set = new Rts2TargetSet (target_types);
			double value;
			int i = 0;
			XmlRpcValue retVar;

			for (Rts2TargetSet::iterator tar_iter = tar_set->begin(); tar_iter != tar_set->end (); tar_iter++, i++)
			{
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
				retVar["last_obs"] = value;
				struct ln_equ_posn pos;
				(*tar_iter)->getPosition (&pos, ln_get_julian_from_sys ());
				retVar["ra"] = pos.ra;
				retVar["dec"] = pos.dec;
				result[i++] = retVar;
			}
		}

		std::string help ()
		{
			return std::string ("Returns all targets");
		}

} listTargetsByType (&xmlrpc_server);


class TargetInfo: public XmlRpcServerMethod
{
	public:
		TargetInfo (XmlRpcServer* s) : XmlRpcServerMethod (R2X_TARGETS_INFO, s) {}

		void execute (XmlRpcValue& params, XmlRpcValue& result)
		{
			std::list < int >targets;
			XmlRpcValue retVar;
			int i;
			double JD;

			for (i = 0; i < params.size(); i++)
				targets.push_back (params[i]);

			Rts2TargetSet *tar_set;
			tar_set = new Rts2TargetSet (targets);

			i = 0;

			for (Rts2TargetSet::iterator tar_iter = tar_set->begin(); tar_iter != tar_set->end (); tar_iter++)
			{
				JD = ln_get_julian_from_sys ();

				XmlStream xs (&retVar);
				xs << *(*tar_iter);

				// observations
				Rts2ObsSet *obs_set;
				obs_set = new Rts2ObsSet ((*tar_iter)->getTargetID ());
				int j = 0;
				for (Rts2ObsSet::iterator obs_iter = obs_set->begin(); obs_iter != obs_set->end(); obs_iter++, j++)
				{
					retVar["observation"][j]["obsid"] =  (*obs_iter).getObsId();
					retVar["observation"][j]["images"] = (*obs_iter).getNumberOfImages();
				}

				retVar["HOUR"] = (*tar_iter)->getHourAngle (JD);
				Target *tar = *tar_iter;
				tar->sendInfo (xs, JD);
				result[i++] = retVar;
			}
		}

} targetInfo (&xmlrpc_server);

class ListObservations: public XmlRpcServerMethod
{
	public:
		ListObservations (XmlRpcServer* s) : XmlRpcServerMethod (R2X_TARGET_OBSERVATIONS_LIST, s) {}

		void execute (XmlRpcValue& params, XmlRpcValue& result)
		{
			XmlRpcValue retVar;
			Rts2ObsSet *obs_set;
			obs_set = new Rts2ObsSet ((int)params[0]);
			int i = 0;
			for (Rts2ObsSet::iterator obs_iter = obs_set->begin(); obs_iter != obs_set->end(); obs_iter++, i++)
			{
				retVar["id"] = (*obs_iter).getObsId();
				retVar["obs_ra"] = (*obs_iter).getObsRa();
				retVar["obs_dec"] = (*obs_iter).getObsDec();
				retVar["obs_start"] = (*obs_iter).getObsStart();
				retVar["obs_end"] = (*obs_iter).getObsEnd();
				retVar["obs_images"] = (*obs_iter).getNumberOfImages();

				result[i] = retVar;
			}
		}

} listObservations (&xmlrpc_server);

class ListImages: public XmlRpcServerMethod
{
	public:
		ListImages (XmlRpcServer* s) : XmlRpcServerMethod (R2X_OBSERVATION_IMAGES_LIST, s) {}

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
				double eRa, eDec, eRad;
				eRa = eDec = eRad = nan ("f");
				Rts2Image *image = *img_iter;
				retVar["filename"] = image->getImageName ();
				retVar["start"] = image->getExposureStart ();
				retVar["exposure"] = image->getExposureLength ();
				retVar["filter"] = image->getFilter ();
				image->getError (eRa, eDec, eRad);
				retVar["error_ra"] = eRa;
				retVar["error_dec"] = eDec;
				retVar["error_pos"] = eRad;
				result[i++] = retVar;
			}
		}
} listImages (&xmlrpc_server);

class GetMessages: public XmlRpcServerMethod
{
	public:
		GetMessages (XmlRpcServer* s) : XmlRpcServerMethod (R2X_MESSAGES_GET, s) {}

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

class UserLogin: public XmlRpcServerMethod
{
	public:
		UserLogin (XmlRpcServer* s) : XmlRpcServerMethod (R2X_USER_LOGIN, s) {}

		void execute (XmlRpcValue& params, XmlRpcValue& result)
		{
			if (params.size() != 2)
			{
				throw XmlRpcException ("Invalid number of parameters");
			}
			result = verifyUser (params[0], params[1]);
		}
} userLogin (&xmlrpc_server);

#endif /* HAVE_PGSQL */

int
main (int argc, char **argv)
{
	Rts2XmlRpcd device = Rts2XmlRpcd (argc, argv);
	return device.run ();
}
