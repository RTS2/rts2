/*
 * Abstract class for executor connection.
 * Copyright (C) 2010 Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
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

#include "connexe.h"
#include "../utils/rts2command.h"
#include "../utils/daemon.h"

using namespace rts2script;

ConnExe::ConnExe (rts2core::Block *_master, const char *_exec, bool fillConnEnv, int timeout):rts2core::ConnFork (_master, _exec, fillConnEnv, true, timeout)
{
}

ConnExe::~ConnExe ()
{
	for (std::vector <std::string>::iterator iter = tempentries.begin (); iter != tempentries.end (); iter++)
	{
		logStream (MESSAGE_DEBUG) << "removing /tmp/" << (*iter) << sendLog;
		rmdir_r (("/tmp/" + (*iter)).c_str ());
	}
}

int32_t typeFromString (const char *dt_string)
{
	if (dt_string == NULL)
		return 0;
	static const char *types [0xb] = {"DT_RA", "DT_DEC", "DT_DEGREES", "DT_DEG_DIST", "DT_PERCENTS", "DT_ROTANG", "DT_HEX", "DT_BYTESIZE", "DT_KMG", "DT_INTERVAL", "DT_ONOFF"};
	const char** vn = types;
	for (int32_t i = 0; i < 0xc; i++, vn++)
	{
		if (!strcasecmp (*vn, dt_string))
			return i << 16;
	}
	throw rts2core::Error (std::string ("invalid data type ") + dt_string);
}

void ConnExe::processCommand (char *cmd)
{
	char *device;
	char *value;
	char *operat;
	char *operand;
	char *comm;

	if (!strcmp (cmd, "C"))
	{
		if (paramNextString (&device) || (comm = paramNextWholeString ()) == NULL)
			return;
		Rts2Conn *conn = getConnectionForScript (device);
		if (conn)
		{
			conn->queCommand (new Rts2Command (getMaster (), comm));
		}
	}
	else if (!strcmp (cmd, "V"))
	{
		if (paramNextString (&device) || paramNextString (&value) || paramNextString (&operat) || (operand = paramNextWholeString ()) == NULL)
			return;
		Rts2Conn *conn = getConnectionForScript (device);
		if (conn)
		{
			conn->queCommand (new Rts2CommandChangeValue (conn->getOtherDevClient (), std::string (value), *operat, std::string (operand), true));
		}
	}
	else if (!strcmp (cmd, "G"))
	{
		if (paramNextString (&device) || paramNextString (&value) || master == NULL)
			return;

		rts2core::Value *val = NULL;

		if (isCentraldName (device))
			val = master->getSingleCentralConn ()->getValue (value);
		else
			val = master->getValue (device, value);

		if (val)
		{
			writeToProcess (val->getValue ());
			return;
		}
		else
		{
			writeToProcess ("ERR");
		}
	}
	else if (!strcmp (cmd, "waitidle"))
	{

	}
	else if (!strcmp (cmd, "log"))
	{
		if (paramNextString (&device) || (value = paramNextWholeString ()) == NULL)
			return;
		messageType_t logLevel;
		switch (toupper (*device))
		{
			case 'E':
				logLevel = MESSAGE_ERROR;
				break;
			case 'W':
				logLevel = MESSAGE_WARNING;
				break;
			case 'I':
				logLevel = MESSAGE_INFO;
				break;
			case 'D':
				logLevel = MESSAGE_DEBUG;
				break;
			default:
				logStream (MESSAGE_ERROR) << "Unknow log level: " << *device << sendLog;
				logLevel = MESSAGE_ERROR;
				break;
		}
		logStream (logLevel) << value << sendLog;
	}
	else if (!strcasecmp (cmd, "tempentry"))
	{
	 	if (paramNextString (&value))
			return;
		tempentries.push_back (std::string (value));
	}
	else if (!strcasecmp (cmd, "double"))
	{
		char *vname, *desc;
		double val;
		char *rts2_type = NULL;
		if (paramNextString (&vname) || paramNextString (&desc) || paramNextDouble (&val) || (!paramEnd () && ( paramNextString (&rts2_type) || !paramEnd () )) )
			throw rts2core::Error ("invalid double string");
		rts2core::Value *v = ((rts2core::Daemon *) master)->getOwnValue (vname);
		// either variable with given name exists..
		if (v)
		{
			if (v->getValueBaseType () == RTS2_VALUE_DOUBLE)
			{
				((rts2core::ValueDouble *) v)->setValueDouble (val);
				((rts2core::Daemon *) master)->sendValueAll (v);
			}
			else
			{
				throw rts2core::Error (std::string ("value is not double ") + vname);
			}
		}
		// or create it and distribute it..
		else
		{
			rts2core::ValueDouble *vd;
			((rts2core::Daemon *) master)->createValue (vd, vname, desc, false, typeFromString (rts2_type));
			vd->setValueDouble (val);
			master->updateMetaInformations (vd);
		}
	}
	else if (!strcasecmp (cmd, "integer"))
	{
		char *vname, *desc;
		int val;
		if (paramNextString (&vname) || paramNextString (&desc) || paramNextInteger (&val) || !paramEnd ())
			throw rts2core::Error ("invalid integer string");
		rts2core::Value *v = ((rts2core::Daemon *) master)->getOwnValue (vname);
		// either variable with given name exists..
		if (v)
		{
			if (v->getValueBaseType () == RTS2_VALUE_INTEGER)
			{
				((rts2core::ValueInteger *) v)->setValueInteger (val);
				((rts2core::Daemon *) master)->sendValueAll (v);
			}
			else
			{
				throw rts2core::Error (std::string ("value is not double ") + vname);
			}
		}
		// or create it and distribute it..
		else
		{
			rts2core::ValueInteger *vi;
			((rts2core::Daemon *) master)->createValue (vi, vname, desc, false);
			vi->setValueInteger (val);
			master->updateMetaInformations (vi);
		}
	}
	else if (!strcasecmp (cmd, "string"))
	{
		char *vname, *desc, *val;
		if (paramNextString (&vname) || paramNextString (&desc) || paramNextString (&val) || !paramEnd ())
			throw rts2core::Error ("invalid string");
		rts2core::Value *v = ((rts2core::Daemon *) master)->getOwnValue (vname);
		// either variable with given name exists..
		if (v)
		{
			if (v->getValueBaseType () == RTS2_VALUE_STRING)
			{
				((rts2core::ValueInteger *) v)->setValueCharArr (val);
				((rts2core::Daemon *) master)->sendValueAll (v);
			}
			else
			{
				throw rts2core::Error (std::string ("value is not double ") + vname);
			}
		}
		// or create it and distribute it..
		else
		{
			rts2core::ValueString *vi;
			((rts2core::Daemon *) master)->createValue (vi, vname, desc, false);
			vi->setValueCharArr (val);
			master->updateMetaInformations (vi);
		}
	}
	else
	{
		throw rts2core::Error (std::string ("unknow command ") + cmd);
	}
}

void ConnExe::processLine ()
{
	char *cmd;

	if (paramNextString (&cmd))
		return;

	try
	{
		processCommand (cmd);
		return;
	} catch (rts2core::Error &er)
	{
		logStream (MESSAGE_ERROR) << "while processing " << cmd << " : " << er << sendLog;
	}
}

void ConnExe::processErrorLine (char *errbuf)
{
	logStream (MESSAGE_ERROR) << "from script " << getExePath () << " received " << errbuf << sendLog;
}

Rts2Conn *ConnExe::getConnectionForScript (const char *_name)
{
	if (isCentraldName (_name))
		return getMaster ()->getSingleCentralConn ();
	return getMaster ()->getOpenConnection (_name);
}

int ConnExe::getDeviceType (const char *_name)
{
	if (!strcasecmp (_name, "TELESCOPE"))
		return DEVICE_TYPE_MOUNT;
	else if (!strcasecmp (_name, "CCD"))
	  	return DEVICE_TYPE_CCD;
	else if (!strcasecmp (_name, "DOME"))
	  	return DEVICE_TYPE_DOME;
	else if (!strcasecmp (_name, "WEATHER"))
	  	return DEVICE_TYPE_WEATHER;
	else if (!strcasecmp (_name, "PHOT"))
	  	return DEVICE_TYPE_PHOT;
	else if (!strcasecmp (_name, "PLAN"))
	  	return DEVICE_TYPE_PLAN;
	else if (!strcasecmp (_name, "FOCUS"))
	  	return DEVICE_TYPE_FOCUS;
	else if (!strcasecmp (_name, "CUPOLA"))
	  	return DEVICE_TYPE_CUPOLA;
	else if (!strcasecmp (_name, "FW"))
	  	return DEVICE_TYPE_FW;
	else if (!strcasecmp (_name, "SENSOR"))
	  	return DEVICE_TYPE_SENSOR;
	throw rts2core::Error (std::string ("unknow device type ") + _name);
}
