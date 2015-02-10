/* 
 * Various API routines, usually XML-RPC API.
 * Copyright (C) 2011 Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
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

#include "xmlapi.h"
#include "httpd.h"

#include "xmlstream.h"

#include "displayvalue.h"

#ifdef RTS2_HAVE_PGSQL
#include "rts2db/recvals.h"
#include "rts2db/records.h"
#include "rts2db/recordsavg.h"
#include "rts2db/devicedb.h"
#include "rts2db/imageset.h"
#include "rts2db/observationset.h"
#include "rts2db/messagedb.h"
#include "rts2db/targetset.h"
#include "rts2db/user.h"
#include "rts2db/sqlerror.h"
#include "rts2scheduler/ticket.h"
#include "rts2fits/imagedb.h"
#else
#include "configuration.h"
#include "device.h"
#endif /* RTS2_HAVE_PGSQL */

using namespace rts2xmlrpc;

void connectionValuesToXmlRpc (rts2core::Connection *conn, XmlRpcValue& result, bool pretty)
{
	int i = 0;
	for (rts2core::ValueVector::iterator variter = conn->valueBegin (); variter != conn->valueEnd (); variter++, i++)
	{
		XmlRpcValue retVar;
		retVar["name"] = (*variter)->getName ();
		retVar["flags"] = (*variter)->getFlags ();

		rts2core::Value *val = *variter;

		if (pretty)
		{
			std::ostringstream _os;
			switch (val->getValueType ())
			{
				case RTS2_VALUE_RADEC:
				{
					if (val->getValueDisplayType () == RTS2_DT_DEGREES)
					{
						LibnovaDeg v_rd (((rts2core::ValueRaDec *) val)->getRa ());
						LibnovaDeg v_dd (((rts2core::ValueRaDec *) val)->getDec ());
						_os << v_rd << " " << v_dd;
					}
					else if (val->getValueDisplayType () == RTS2_DT_DEG_DIST_180)
					{
						LibnovaDeg180 v_rd (((rts2core::ValueRaDec *) val)->getRa ());
						LibnovaDeg90 v_dd (((rts2core::ValueRaDec *) val)->getDec ());
						_os << v_rd << " " << v_dd;
					}
					else 
					{
						LibnovaRaDec v_radec (((rts2core::ValueRaDec *) val)->getRa (), ((rts2core::ValueRaDec *) val)->getDec ());
						_os << v_radec;
					}
					break;
				}	
				case RTS2_VALUE_ALTAZ:
				{
					LibnovaHrz hrz (((rts2core::ValueAltAz *) val)->getAlt (), ((rts2core::ValueAltAz *) val)->getAz ());
					_os << hrz;
					break;
				}
				case RTS2_VALUE_SELECTION:
					_os << ((rts2core::ValueSelection *) val)->getSelName ();
					break;
				default:
					_os << getDisplayValue (val);
			}
			retVar["value"] = _os.str ();
		}
		else
		{
			switch (val->getValueExtType ())
			{
				case RTS2_VALUE_RECTANGLE:
					retVar["value"] = (*variter)->getDisplayValue ();
					break;
				default:
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
							bool_val = ((rts2core::ValueBool*)(*variter))->getValueBool ();
							retVar["value"] = bool_val;
							break;
						case RTS2_VALUE_LONGINT:
							int_val = (*variter)->getValueLong ();
							retVar["value"] = int_val;
							break;
						case RTS2_VALUE_TIME:
							struct tm tm_s;
							long usec;
							((rts2core::ValueTime*) (*variter))->getStructTm (&tm_s, &usec);
							retVar["value"] = XmlRpcValue (&tm_s);
							break;
						default:
							retVar["value"] = (*variter)->getValue ();
							break;
					}
			}
		}
		result[i] = retVar;
	}
}

SessionMethod::SessionMethod (const char *method, XmlRpcServer* s): XmlRpcServerMethod (method, s)
{
}

void SessionMethod::execute (struct sockaddr_in *saddr, XmlRpcValue& params, XmlRpcValue& result)
{
	if (((rts2json::HTTPServer *) getMasterApp ())->authorizeLocalhost () || ntohl (saddr->sin_addr.s_addr) != INADDR_LOOPBACK)
	{
		if (getUsername () == std::string ("session_id"))
		{
			if (((HttpD *) getMasterApp ())->existsSession (getPassword ()) == false)
			{
				throw XmlRpcException ("Invalid session ID");
			}
		}

		if (verifyUser (getUsername (), getPassword ()) == false)
		{
			throw XmlRpcException ("Invalid login or password");
		}
	}

	sessionExecute (params, result);
}

void Login::execute (struct sockaddr_in *saddr, XmlRpcValue& params, XmlRpcValue& result)
{
	if (params.size () != 2)
	{
		throw XmlRpcException ("Invalid number of parameters");
	}

	if (verifyUser (params[0], params[1]) == false)
	{
		throw XmlRpcException ("Invalid login or password");
	}

	result = ((HttpD *) getMasterApp ())->addSession (params[0], 3600);
}

void DeviceCount::sessionExecute (XmlRpcValue& params, XmlRpcValue& result)
{
	result = ((HttpD *) getMasterApp ())->connectionSize ();
}

void ListDevices::sessionExecute (XmlRpcValue& params, XmlRpcValue& result)
{
	HttpD *serv = (HttpD *) getMasterApp ();
	connections_t::iterator iter;
	int i = 0;
	for (iter = serv->getConnections ()->begin (); iter != serv->getConnections ()->end (); iter++, i++)
	{
		result[i] = (*iter)->getName ();
	}
}

void DeviceByType::sessionExecute (XmlRpcValue& params, XmlRpcValue& result)
{
	HttpD *serv = (HttpD *) getMasterApp ();
	connections_t::iterator iter = serv->getConnections ()->begin ();
	int i = 0;
	int type = params[0];
	while (true)
	{
		serv->getOpenConnectionType (type, iter);
		if (iter == serv->getConnections ()->end ())
			break;
		result[i] = (*iter)->getName ();
		iter++;
		i++;
	}

}



void DeviceType::sessionExecute (XmlRpcValue& params, XmlRpcValue &result)
{
	if (params.size () != 1)
		throw XmlRpcException ("Single device name expected");
	HttpD *serv = (HttpD *) getMasterApp ();
	rts2core::Connection *conn = serv->getOpenConnection (((std::string)params[0]).c_str());
	if (conn == NULL)
		throw XmlRpcException ("Cannot get device with name " + (std::string)params[0]);
	result = conn->getOtherType ();
}

void MasterState::sessionExecute (XmlRpcValue& params, XmlRpcValue& result)
{
	result = int (((HttpD *) getMasterApp ())->getMasterStateFull ());
}

void MasterStateIs::sessionExecute (XmlRpcValue& params, XmlRpcValue& result)
{
	int ms = ((HttpD *) getMasterApp ())->getMasterStateFull ();
	if (params.size () == 1)
	{
		if (params[0] == "on")
		{
			result = !(ms & SERVERD_ONOFF_MASK);
		}
		else if (params[0] == "standby")
		{
			result = (ms & SERVERD_ONOFF_MASK) == SERVERD_STANDBY;
		}
		else if (params[0] == "off")
		{
			result = (ms & SERVERD_ONOFF_MASK) == SERVERD_HARD_OFF
				|| (ms & SERVERD_ONOFF_MASK) == SERVERD_SOFT_OFF;
		}
		else if (params[0] == "rnight")
		{
			result = (ms & SERVERD_STATUS_MASK) == SERVERD_NIGHT && !(ms & SERVERD_ONOFF_MASK);
		}
		else
		{
			throw XmlRpcException ("Invalid status name parameter");
		}
	}
	else
	{
		throw XmlRpcException ("Invalid number of parameters");
	}
}

void DeviceCommand::sessionExecute (XmlRpcValue& params, XmlRpcValue &result)
{
	if (params.size () != 2)
		throw XmlRpcException ("Device name and command (as single parameter) expected");
	HttpD *serv = (HttpD *) getMasterApp ();
	rts2core::Connection *conn = serv->getOpenConnection (((std::string)params[0]).c_str());
	if (conn == NULL)
		throw XmlRpcException ("Device named " + (std::string)params[0] + " does not exists");
	conn->queCommand (new rts2core::Command (serv, ((std::string)params[1]).c_str()));
}

void DeviceState::sessionExecute (XmlRpcValue& params, XmlRpcValue& result)
{
	if (params.size () != 1)
	{
		throw XmlRpcException ("Invalid number of parameters");
	}
	HttpD *serv = (HttpD *) getMasterApp ();
	std::string p1 = std::string (params[0]);

	rts2core::Connection *conn;

	if (p1 == "centrald" || p1 == "")
		conn = *(serv->getCentraldConns ()->begin ());
	else
		conn = serv->getOpenConnection (p1.c_str ());

	if (conn == NULL)
	{
		throw XmlRpcException ("Cannot find specified device");
	}
	result[0] = conn->getStateString ();
	result[1] = (int) conn->getRealState ();
}

void ListValues::sessionExecute (XmlRpcValue& params, XmlRpcValue& result)
{
	HttpD *serv = (HttpD *) getMasterApp ();
	rts2core::Connection *conn;
	connections_t::iterator iter;
	rts2core::ValueVector::iterator variter;
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

void ListValuesDevice::sessionExecute (XmlRpcValue& params, XmlRpcValue& result)
{
	HttpD *serv = (HttpD *) getMasterApp ();
	rts2core::Connection *conn;
	int i = 0;
	// print results for a single device..
	if (params.size() == 1)
	{
		if (((std::string) params[0]).length () == 0)
			conn = serv->getSingleCentralConn ();
		else
			conn = serv->getOpenConnection (((std::string)params[0]).c_str());
		if (!conn)
		{
			throw XmlRpcException ("Cannot get device " + (std::string) params[0]);
		}
		connectionValuesToXmlRpc (conn, result, false);
	}
	// print from all
	else
	{
		connections_t::iterator iter;
		rts2core::ValueVector::iterator variter;
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

void ListPrettyValuesDevice::sessionExecute (XmlRpcValue& params, XmlRpcValue& result)
{
	HttpD *serv = (HttpD *) getMasterApp ();
	rts2core::Connection *conn;
	int i = 0;
	// print results for a single device..
	if (params.size() == 1)
	{
		if (((std::string) params[0]).length () == 0)
			conn = serv->getSingleCentralConn ();
		else
			conn = serv->getOpenConnection (((std::string)params[0]).c_str());
		if (!conn)
		{
			throw XmlRpcException ("Cannot get device " + (std::string) params[0]);
		}
		connectionValuesToXmlRpc (conn, result, true);
	}
	// print from all
	else
	{
		connections_t::iterator iter;
		rts2core::ValueVector::iterator variter;
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

void GetValue::sessionExecute (XmlRpcValue& params, XmlRpcValue& result)
{
	std::string devName = params[0];
	std::string valueName = params[1];
	HttpD *serv = (HttpD *) getMasterApp ();
	rts2core::Connection *conn;
	if (devName.length () == 0)
	{
		conn = serv->getSingleCentralConn ();
	}
	else
	{
		conn = serv->getOpenConnection (devName.c_str ());
	}
	if (!conn)
	{
		throw XmlRpcException ("Cannot find connection '" + std::string (devName) + "'.");
	}
	rts2core::Value *val = conn->getValue (valueName.c_str ());
	if (!val)
	{
		throw XmlRpcException ("Cannot find value '" + std::string (valueName) + "' on device '" + std::string (devName) + "'.");
	}
	switch (val->getValueBaseType ())
	{
		case RTS2_VALUE_INTEGER:
		case RTS2_VALUE_LONGINT:
			result = val->getValueInteger ();
			break;
		case RTS2_VALUE_FLOAT:
			result = val->getValueFloat ();
			break;
		case RTS2_VALUE_DOUBLE:
			result = val->getValueDouble ();
			break;
		case RTS2_VALUE_BOOL:
			result = ((rts2core::ValueBool *)val)->getValueBool ();
			break;
		case RTS2_VALUE_TIME:
			struct tm tm_s;
			long usec;
			((rts2core::ValueTime*)val)->getStructTm (&tm_s, &usec);
			result = XmlRpcValue (&tm_s);
			break;
		default:
			result = val->getValue ();
			break;
	}
}

void GetPrettyValue::sessionExecute (XmlRpcValue& params, XmlRpcValue& result)
{
	std::string devName = params[0];
	std::string valueName = params[1];
	HttpD *serv = (HttpD *) getMasterApp ();
	rts2core::Connection *conn;
	if (devName.length () == 0)
	{
		conn = serv->getSingleCentralConn ();
	}
	else
	{
		conn = serv->getOpenConnection (devName.c_str ());
	}
	if (!conn)
	{
		throw XmlRpcException ("Cannot find connection '" + std::string (devName) + "'.");
	}
	rts2core::Value *val = conn->getValue (valueName.c_str ());
	if (!val)
	{
		throw XmlRpcException ("Cannot find value '" + std::string (valueName) + "' on device '" + std::string (devName) + "'.");
	}
	result = getDisplayValue (val);
}

void SessionMethodValue::setXmlValutRts2 (rts2core::Connection *conn, std::string valueName, XmlRpcValue &x_val)
{
	int i_val;
	double d_val;
	std::string s_val;

	rts2core::Value *val = conn->getValue (valueName.c_str ());
	if (!val)
	{
		throw XmlRpcException ("Cannot find value '" + std::string (valueName) + "' on device '" + std::string (conn->getName ()) + "'.");
	}

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
			conn->queCommand (new rts2core::CommandChangeValue (conn->getOtherDevClient (), valueName, '=', i_val));
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
			conn->queCommand (new rts2core::CommandChangeValue (conn->getOtherDevClient (), valueName, '=', d_val));
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
			conn->queCommand (new rts2core::CommandChangeValue (conn->getOtherDevClient (), valueName, '=', (float) d_val));
			break;
		case RTS2_VALUE_STRING:
			conn->queCommand (new rts2core::CommandChangeValue (conn->getOtherDevClient (), valueName, '=', (std::string) (x_val)));
			break;
		default:
			conn->queCommand (new rts2core::CommandChangeValue (conn->getOtherDevClient (), valueName, '=', (std::string) (x_val), true));
			break;
	}
}


void SetValue::sessionExecute (XmlRpcValue& params, XmlRpcValue& result)
{
	std::string devName = params[0];
	std::string valueName = params[1];
	HttpD *serv = (HttpD *) getMasterApp ();
	rts2core::Connection *conn = serv->getOpenConnection (devName.c_str ());
	if (!conn)
	{
		throw XmlRpcException ("Cannot find connection '" + std::string (devName) + "'.");
	}
	setXmlValutRts2 (conn, valueName, params[2]);
}

void SetValueByType::sessionExecute (XmlRpcValue& params, XmlRpcValue& result)
{
	int devType = params[0];
	std::string valueName = params[1];
	HttpD *serv = (HttpD *) getMasterApp ();
	connections_t::iterator iter = serv->getConnections ()->begin ();
	serv->getOpenConnectionType (devType, iter);
	if (iter == serv->getConnections ()->end ())
	{
		throw XmlRpcException ("Cannot find connection of given type");
	}
	for (; iter != serv->getConnections ()->end (); serv->getOpenConnectionType (devType, ++iter))
	{
		setXmlValutRts2 (*iter, valueName, params[2]);
	}
}

void IncValue::sessionExecute (XmlRpcValue& params, XmlRpcValue& result)
{
	std::string devName = params[0];
	std::string valueName = params[1];
	HttpD *serv = (HttpD *) getMasterApp ();
	rts2core::Connection *conn = serv->getOpenConnection (devName.c_str ());
	if (!conn)
	{
		throw XmlRpcException ("Cannot find connection '" + std::string (devName) + "'.");
	}
	rts2core::Value *val = conn->getValue (valueName.c_str ());
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
			conn->queCommand (new rts2core::CommandChangeValue (conn->getOtherDevClient (), valueName, '+', i_val));
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
			conn->queCommand (new rts2core::CommandChangeValue (conn->getOtherDevClient (), valueName, '+', d_val));
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
			conn->queCommand (new rts2core::CommandChangeValue (conn->getOtherDevClient (), valueName, '+', (float) d_val));
			break;
		case RTS2_VALUE_STRING:
			conn->queCommand (new rts2core::CommandChangeValue (conn->getOtherDevClient (), valueName, '+', (std::string) (params[2])));
			break;
		default:
			conn->queCommand (new rts2core::CommandChangeValue (conn->getOtherDevClient (), valueName, '+', (std::string) (params[2]), true));
			break;
	}
}

void GetMessages::sessionExecute (XmlRpcValue& params, XmlRpcValue& result)
{
	int i = 0;
	XmlRpcValue retVar;
	for (std::deque <Message>::iterator iter = ((HttpD *) getMasterApp ())->getMessages ().begin ();
		iter != ((HttpD *) getMasterApp ())->getMessages ().end (); iter++, i++)
	{
		XmlRpcValue val;
		Message msg = *iter;
		time_t t;
		val["type"] = msg.getTypeString ();
		val["origin"] = msg.getMessageOName ();
		t = msg.getMessageTime ();
		val["timestamp"] = XmlRpcValue (gmtime (&t));
		val["message"] = msg.getMessageString ();
		result[i] = val;
	}

}

#ifdef RTS2_HAVE_PGSQL

void ListTargets::sessionExecute (XmlRpcValue& params, XmlRpcValue& result)
{
	rts2db::TargetSet tar_set;

	if (params.size () == 1)
	{
		tar_set.loadByName (((std::string) params[0]).c_str ());
	}
	else
	{
		tar_set.load ();
	}

	double value;
	int i = 0;
	XmlRpcValue retVar;

	for (rts2db::TargetSet::iterator tar_iter = tar_set.begin(); tar_iter != tar_set.end (); tar_iter++, i++)
	{
		rts2db::Target *tar = (*tar_iter).second;
		retVar["id"] = tar->getTargetID ();
		retVar["type"] = tar->getTargetType ();
		if (tar->getTargetName ())
			retVar["name"] = tar->getTargetName ();
		else
			retVar["name"] = "NULL";

		retVar["enabled"] = tar->getTargetEnabled ();
		if (tar->getTargetComment ())
			retVar["comment"] = tar->getTargetComment ();
		else
			retVar["comment"] = "";
		value = tar->getLastObs();
		retVar["last_obs"] = value;
		struct ln_equ_posn pos;
		tar->getPosition (&pos, ln_get_julian_from_sys ());
		retVar["ra"] = pos.ra;
		retVar["dec"] = pos.dec;
		result[i++] = retVar;
	}
}

void ListTargetsByType::sessionExecute (XmlRpcValue& params, XmlRpcValue& result)
{
	char target_types[params.size ()+1];
	int j;
	for (j = 0; j < params.size (); j++)
		target_types[j] = *(((std::string)params[j]).c_str());
	target_types[j] = '\0';

	rts2db::TargetSet *tar_set = new rts2db::TargetSet (target_types);
	tar_set->load ();

	double value;
	int i = 0;
	XmlRpcValue retVar;

	double JD = ln_get_julian_from_sys ();

	for (rts2db::TargetSet::iterator tar_iter = tar_set->begin(); tar_iter != tar_set->end (); tar_iter++, i++)
	{
		rts2db::Target *tar = (*tar_iter).second;
		retVar["id"] = tar->getTargetID ();
		retVar["type"] = tar->getTargetType ();
		if (tar->getTargetName ())
			retVar["name"] = tar->getTargetName ();
		else
			retVar["name"] = "NULL";

		retVar["enabled"] = tar->getTargetEnabled ();
		if (tar->getTargetComment ())
			retVar["comment"] = tar->getTargetComment ();
		else
			retVar["comment"] = "";
		value = tar->getLastObs();
		retVar["last_obs"] = value;
		struct ln_equ_posn pos;
		tar->getPosition (&pos, JD);
		retVar["ra"] = pos.ra;
		retVar["dec"] = pos.dec;
		struct ln_hrz_posn hrz;
		tar->getAltAz (&hrz, JD);
		retVar["alt"] = hrz.alt;
		retVar["az"] = hrz.az;
		result[i++] = retVar;
	}
}

void TargetInfo::sessionExecute (XmlRpcValue& params, XmlRpcValue& result)
{
	std::list < int >targets;
	XmlRpcValue retVar;
	int i;
	double JD;

	for (i = 0; i < params.size(); i++)
		targets.push_back (params[i]);

	rts2db::TargetSet *tar_set = new rts2db::TargetSet ();
	try
	{
		tar_set->load (targets);
	}
	catch (...)
	{
		logStream (MESSAGE_ERROR) << "Sql Error" << sendLog;
		return;
	}

	i = 0;

	for (rts2db::TargetSet::iterator tar_iter = tar_set->begin(); tar_iter != tar_set->end (); tar_iter++)
	{
		JD = ln_get_julian_from_sys ();

		rts2db::Target *tar = (*tar_iter).second;

		XmlStream xs (&retVar);
		xs << *tar;

		// observations
		rts2db::ObservationSet obs_set;
		obs_set.loadTarget (tar->getTargetID ());
		int j = 0;
		for (rts2db::ObservationSet::iterator obs_iter = obs_set.begin(); obs_iter != obs_set.end(); obs_iter++, j++)
		{
			retVar["observation"][j]["obsid"] =  (*obs_iter).getObsId();
			retVar["observation"][j]["images"] = (*obs_iter).getNumberOfImages();
		}

		retVar["HOUR"] = tar->getHourAngle (JD);
		tar->sendInfo (xs, JD, 0);
		result[i++] = retVar;
	}
}

void TargetAltitude::sessionExecute (XmlRpcValue& params, XmlRpcValue& result)
{
	if (params.size () != 4)
		throw XmlRpcException ("Invalid number of parameters");
	if (((int) params[0]) < 0)
		throw XmlRpcException ("Target id < 0");
	rts2db::Target *tar = createTarget ((int) params[0], Configuration::instance()->getObserver ());
	if (tar == NULL)
	{
		throw XmlRpcException ("Cannot create target");
	}
	time_t tfrom = (int)params[1];
	double jd_from = ln_get_julian_from_timet (&tfrom);
	time_t tto = (int)params[2];
	double jd_to = ln_get_julian_from_timet (&tto);
	double stepsize = ((double)params[3]) / 86400;
	// test for insane request
	if ((jd_to - jd_from) / stepsize > 10000)
	{
		throw XmlRpcException ("Too many points");
	}
	int i;
	double j;
	for (i = 0, j = jd_from; j < jd_to; i++, j += stepsize)
	{
		result[i][0] = j;
		ln_hrz_posn hrz;
		tar->getAltAz (&hrz, j);
		result[i][1] = hrz.alt;
	}
}

void ListTargetObservations::sessionExecute (XmlRpcValue& params, XmlRpcValue& result)
{
	XmlRpcValue retVar;
	rts2db::ObservationSet obs_set;
	obs_set.loadTarget ((int)params[0]);
	int i = 0;
	time_t t;
	for (rts2db::ObservationSet::iterator obs_iter = obs_set.begin(); obs_iter != obs_set.end(); obs_iter++, i++)
	{
		retVar["id"] = (*obs_iter).getObsId();
		retVar["obs_ra"] = (*obs_iter).getObsRa();
		retVar["obs_dec"] = (*obs_iter).getObsDec();
		t = (*obs_iter).getObsStart();
		retVar["obs_start"] = XmlRpcValue (gmtime (&t));
		t = (*obs_iter).getObsEnd();
		retVar["obs_end"] = XmlRpcValue (gmtime(&t));
		retVar["obs_images"] = (*obs_iter).getNumberOfImages();

		result[i] = retVar;
	}
}

void ListMonthObservations::sessionExecute (XmlRpcValue& params, XmlRpcValue& result)
{
	int y = params[0];
	int m = params[1];

	XmlRpcValue retVar;
	rts2db::ObservationSet obs_set;

	time_t from = Configuration::instance ()->getNight (y, m, 1);
	time_t to = Configuration::instance ()->getNight (y, m + 1, 1);
	obs_set.loadTime (&from, &to);

	int i = 0;
	time_t t;
	for (rts2db::ObservationSet::iterator obs_iter = obs_set.begin(); obs_iter != obs_set.end(); obs_iter++, i++)
	{
		retVar["id"] = (*obs_iter).getObsId();
		retVar["tar_id"] = (*obs_iter).getTargetId ();
		retVar["tar_name"] = (*obs_iter).getTargetName ();
		retVar["obs_ra"] = (*obs_iter).getObsRa();
		retVar["obs_dec"] = (*obs_iter).getObsDec();
		t = (time_t) ((*obs_iter).getObsStart());
		if (t < 0)
			t = 0;
		retVar["obs_start"] = XmlRpcValue (gmtime (&t));
		t = (time_t) ((*obs_iter).getObsEnd());
		if (t < 0)
			t = 0;
		retVar["obs_end"] = XmlRpcValue (gmtime (&t));
		retVar["obs_images"] = (*obs_iter).getNumberOfImages();

		result[i] = retVar;
	}
}

void ListImages::sessionExecute (XmlRpcValue& params, XmlRpcValue& result)
{
	XmlRpcValue retVar;
	rts2db::Observation *obs;
	obs = new rts2db::Observation ((int)params[0]);
	if (obs->loadImages ())
		return;
	rts2db::ImageSet *img_set = obs->getImageSet();
	int i = 0;
	for (rts2db::ImageSet::iterator img_iter = img_set->begin(); img_iter != img_set->end(); img_iter++)
	{
		double eRa, eDec, eRad;
		eRa = eDec = eRad = NAN;
		rts2image::Image *image = *img_iter;
		retVar["filename"] = image->getFileName ();
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

void TicketInfo::sessionExecute (XmlRpcValue& params, XmlRpcValue& result)
{
	if (params.size () != 1)
	{
		throw XmlRpcException ("Invalid number of parameters");
	}

	try
	{
		rts2sched::Ticket t = rts2sched::Ticket (params[0]);
		t.load ();
		XmlStream xs (&result);

		xs << t;
	}
	catch (rts2db::SqlError e)
	{
		throw XmlRpcException (e.what ());
	}
}

void RecordsValues::sessionExecute (XmlRpcValue& params, XmlRpcValue& result)
{
	if (params.getType () != XmlRpcValue::TypeInvalid)
		throw XmlRpcException ("Invalid number of parameters");
	try
	{
		rts2db::RecvalsSet recvals = rts2db::RecvalsSet ();
		int i = 0;
		time_t t;
		recvals.load ();
		for (rts2db::RecvalsSet::iterator iter = recvals.begin (); iter != recvals.end (); iter++)
		{
			rts2db::Recval rv = iter->second;
			XmlRpcValue res;
			res["id"] = rv.getId ();
			res["device"] = rv.getDevice ();
			res["value_name"] = rv.getValueName ();
			res["value_type"] = rv.getType ();
			t = rv.getFrom ();
			res["from"] = XmlRpcValue (gmtime (&t));
			t = rv.getTo ();
			res["to"] = XmlRpcValue (gmtime (&t));
			result[i++] = res;
		}
	}
	catch (rts2db::SqlError err)
	{
		throw XmlRpcException (err.what ());
	}
}

void Records::sessionExecute (XmlRpcValue& params, XmlRpcValue& result)
{
	if (params.size () != 3)
		throw XmlRpcException ("Invalid number of parameters");

	try
	{
		rts2db::RecordsSet recset = rts2db::RecordsSet (params[0]);
		int i = 0;
		time_t t;
		recset.load (params[1], params[2]);
		for (rts2db::RecordsSet::iterator iter = recset.begin (); iter != recset.end (); iter++)
		{
			rts2db::Record rv = (*iter);
			XmlRpcValue res;
			t = rv.getRecTime ();
			res[0] = XmlRpcValue (gmtime (&t));
			res[1] = rv.getValue ();
			result[i++] = res;
		}
	}
	catch (rts2db::SqlError err)
	{
		throw XmlRpcException (err.what ());
	}
}

void RecordsAverage::sessionExecute (XmlRpcValue& params, XmlRpcValue& result)
{
	if (params.size () != 3)
		throw XmlRpcException ("Invalid number of parameters");

	try
	{
		rts2db::RecordAvgSet recset = rts2db::RecordAvgSet (params[0], rts2db::HOUR);
		int i = 0;
		time_t t;
		recset.load (params[1], params[2]);
		for (rts2db::RecordAvgSet::iterator iter = recset.begin (); iter != recset.end (); iter++)
		{
			rts2db::RecordAvg rv = (*iter);
			XmlRpcValue res;
			t = rv.getRecTime ();
			res[0] = XmlRpcValue (gmtime (&t));
			res[1] = rv.getAverage ();
			res[2] = rv.getMinimum ();
			res[3] = rv.getMaximum ();
			res[4] = rv.getRecCout ();
			result[i++] = res;
		}
	}
	catch (rts2db::SqlError err)
	{
		throw XmlRpcException (err.what ());
	}
}

void UserLogin::execute (struct sockaddr_in *saddr, XmlRpcValue& params, XmlRpcValue& result)
{
	if (params.size() != 2)
		throw XmlRpcException ("Invalid number of parameters");

	result = verifyUser (params[0], params[1]);
}

#endif /* RTS2_HAVE_PGSQL */
