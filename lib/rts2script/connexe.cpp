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

#include "rts2script/connexe.h"
#include "command.h"
#include "daemon.h"
#include "device.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

using namespace rts2script;

ConnExe::ConnExe (rts2core::Block *_master, const char *_exec, bool fillConnEnv, int timeout):rts2core::ConnFork (_master, _exec, fillConnEnv, true, timeout)
{
	active = true;
}

ConnExe::~ConnExe ()
{
	// as safety
	for (std::vector <std::string>::iterator iter = tempentries.begin (); iter != tempentries.end (); iter++)
	{
		int ret = rmdir_r (("/tmp/" + (*iter)).c_str ());
		if (ret && errno != ENOENT)
			logStream (MESSAGE_ERROR) << "cannot remove /tmp/" << (*iter) << ", error " << strerror (errno) << sendLog;
		else
			logStream (MESSAGE_DEBUG) << "removed /tmp/" << (*iter) << sendLog;
	}
}

void ConnExe::testWritableVariable (const char *cmd, int32_t vflags, rts2core::Value *v)
{
	if (vflags & RTS2_VALUE_WRITABLE)
	{
		if (!v->isWritable ())
			logStream (MESSAGE_WARNING) << "updating constant variable " << v->getName () << " with " << cmd << " command" << sendLog;
	}
	else if (v->isWritable ())
	{
		logStream (MESSAGE_WARNING) << "updating writable variable " << v->getName () << " with " << cmd << " command" << sendLog;
	}
}

bool isValueCommand (const char *cmd, const char *expected, int32_t &flags)
{
	flags = 0;
	int el = strlen (expected);
	if (!strncasecmp (cmd, expected, el))
	{
		const char *pos = cmd + el;

		if (*pos == '\0')
			return true;
		else if (*pos == '_')
		{
			do
			{
				pos ++;

				if (*pos == 'w')
					flags |= RTS2_VALUE_WRITABLE;
				if (*pos == 'f')
					flags |= RTS2_VALUE_FITS;
				if (*pos == 't')
					flags |= RTS2_VALUE_SCRIPTTEMPORARY;
			}
			while (*pos != '\0');
		}
		return true;
	}
	return false;
}

void ConnExe::processCommand (char *cmd)
{
	char *device;
	char *value;
	char *operat;
	char *operand;
	char *comm;

	char *vname, *desc;
	rts2core::Value *v;

	int32_t vflags;

	if (!strcmp (cmd, "C"))
	{
		if (!checkActive ())
			return;
		if (paramNextString (&device) || (comm = paramNextWholeString ()) == NULL)
			return;
		rts2core::Connection *conn = getConnectionForScript (device);
		if (conn)
		{
			conn->queCommand (new rts2core::Command (getMaster (), comm));
		}
	}
	else if (!strcmp (cmd, "CT"))
	{
		if (!checkActive ())
			return;
		if (paramNextString (&device) || (comm = paramNextWholeString ()) == NULL)
			return;
		int deviceTypeNum = getDeviceType (device);
		rts2core::Command comd (getMaster (), comm);
		getMaster ()->queueCommandForType (deviceTypeNum, comd);
	}
	else if (!strcmp (cmd, "V"))
	{
		if (!checkActive ())
			return;
		if (paramNextString (&device) || paramNextString (&value) || paramNextString (&operat) || (operand = paramNextWholeString ()) == NULL)
			return;
		rts2core::Connection *conn = getConnectionForScript (device);
		if (conn)
		{
			conn->queCommand (new rts2core::CommandChangeValue (conn->getOtherDevClient ()->getMaster (), std::string (value), *operat, std::string (operand), true));
		}
	}
	else if (!strcmp (cmd, "set_own"))
	{
		if (!checkActive ())
			return;
		if (paramNextString (&value) || (operand = paramNextWholeString ()) == NULL || master == NULL)
			return;
		rts2core::Value *val = ((rts2core::Daemon *) master)->getOwnValue (value);
		if (val)
		{
			val->setValueCharArr (operand);
			((rts2core::Daemon *) master)->sendValueAll (val);
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
	else if (!strcmp (cmd, "get_own"))
	{
		if (paramNextString (&value) || master == NULL)
			return;

		rts2core::Value *val = ((rts2core::Daemon *) master)->getOwnValue (value);

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
	else if (!strcmp (cmd, "S"))
	{
		if (paramNextString (&device))
			return;

		rts2core::Connection *conn = getConnectionForScript (device);
		if (conn)
			writeToProcessInt (conn->getState ());
		else
			writeToProcess ("ERR");
	}
	else if (!strcmp (cmd, "waitidle"))
	{
		int tom;
		if (paramNextString (&device) || paramNextInteger (&tom))
			return;
		rts2core::Connection *conn = getConnectionForScript (device);
		if (conn)
		{
			tom += time (NULL);
			while (time (NULL) < tom)
			{
				if ((conn->getOtherType () == DEVICE_TYPE_MOUNT && (conn->getState () & TEL_MASK_MOVING) != TEL_MOVING) || conn->isIdle ())
				{
					writeToProcess ("1");
					break;
				}
				getMaster ()->oneRunLoop ();
			}
			if (time (NULL) >= tom)
				writeToProcess ("0");
		}
		else
		{
			writeToProcess ("ERR");
		}
	}
	else if (!strcmp (cmd, "waitmask"))
	{
		int tom, mask;
		if (paramNextString (&device) || paramNextInteger (&mask) || paramNextInteger (&tom))
			return;
		rts2core::Connection *conn = getConnectionForScript (device);
		if (conn)
		{
			tom += time (NULL);
			while (time (NULL) < tom)
			{
				if ((int) (conn->getState () & mask) == mask)
				{
					writeToProcess ("1");
					break;
				}
				getMaster ()->oneRunLoop ();
			}
			if (time (NULL) >= tom)
				writeToProcess ("0");
		}
		else
		{
			writeToProcess ("ERR");
		}
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
	else if (!strcmp (cmd, "run_device"))
	{
		writeToProcess ("processing");
	}
	else if (!strcmp (cmd, "exec_device"))
	{
		writeToProcess (((rts2core::Device *) master)->getDeviceName () ? ((rts2core::Device *) master)->getDeviceName () : "None");
	}
	// register temporar entry directory. Path without /tmp should be specified.
	else if (!strcasecmp (cmd, "tempentry"))
	{
	 	if (paramNextString (&value))
			return;
		// refuse tempentry containing /../
		if (strstr (value, "/../"))
		{
			logStream (MESSAGE_ERROR) << "ignore tempentry with /../, as this might escape from /tmp directory" << sendLog;
		}
		else
		{
			// check that the file exist and is writable
			int fd = open ((std::string("/tmp/") + value).c_str (), O_RDWR);
			if (fd < 0 && errno != EISDIR)
				logStream (MESSAGE_ERROR) << "entry /tmp/" << value << " does not exists: " << strerror (errno) << sendLog;
			else
				tempentries.push_back (std::string (value));
		}
	}
	else if (isValueCommand (cmd, "double", vflags))
	{
		double val;
		char *rts2_type = NULL;
		if (paramNextString (&vname) || paramNextString (&desc) || paramNextDouble (&val) || (!paramEnd () && ( paramNextString (&rts2_type) || !paramEnd () )) )
			throw rts2core::Error ("invalid double string");
		v = ((rts2core::Daemon *) master)->getOwnValue (vname);
		// either variable with given name exists..
		if (v)
		{
			if (v->getValueBaseType () != RTS2_VALUE_DOUBLE)
				throw rts2core::Error (std::string ("value is not double ") + vname);
			testWritableVariable (cmd, vflags, v);
			((rts2core::ValueDouble *) v)->setValueDouble (val);
			((rts2core::Daemon *) master)->sendValueAll (v);
		}
		// or create it and distribute it..
		else
		{
			rts2core::ValueDouble *vd;
			((rts2core::Daemon *) master)->createValue (vd, vname, desc, vflags & RTS2_VALUE_FITS, vflags | rts2core::typeFromString (rts2_type));
			vd->setValueDouble (val);
			master->updateMetaInformations (vd);
		}
	}
	else if (isValueCommand (cmd, "timevalue", vflags))
	{
		double val;
		char *rts2_type = NULL;
		if (paramNextString (&vname) || paramNextString (&desc) || paramNextDouble (&val) || (!paramEnd () && ( paramNextString (&rts2_type) || !paramEnd () )) )
			throw rts2core::Error ("invalid double string");
		v = ((rts2core::Daemon *) master)->getOwnValue (vname);
		// either variable with given name exists..
		if (v)
		{
			if (v->getValueBaseType () != RTS2_VALUE_TIME)
				throw rts2core::Error (std::string ("value is not time ") + vname);
			testWritableVariable (cmd, vflags, v);
			((rts2core::ValueTime *) v)->setValueDouble (val);
			((rts2core::Daemon *) master)->sendValueAll (v);
		}
		// or create it and distribute it..
		else
		{
			rts2core::ValueTime *vd;
			((rts2core::Daemon *) master)->createValue (vd, vname, desc, vflags & RTS2_VALUE_FITS, vflags | rts2core::typeFromString (rts2_type));
			vd->setValueDouble (val);
			master->updateMetaInformations (vd);
		}
	}
	else if (isValueCommand (cmd, "integer", vflags))
	{
		int val;
		if (paramNextString (&vname) || paramNextString (&desc) || paramNextInteger (&val) || !paramEnd ())
			throw rts2core::Error ("invalid integer string");
		v = ((rts2core::Daemon *) master)->getOwnValue (vname);
		// either variable with given name exists..
		if (v)
		{
			if (v->getValueBaseType () != RTS2_VALUE_INTEGER)
				throw rts2core::Error (std::string ("value is not double ") + vname);
			testWritableVariable (cmd, vflags, v);
			((rts2core::ValueInteger *) v)->setValueInteger (val);
			((rts2core::Daemon *) master)->sendValueAll (v);
		}
		// or create it and distribute it..
		else
		{
			rts2core::ValueInteger *vi;
			((rts2core::Daemon *) master)->createValue (vi, vname, desc, vflags & RTS2_VALUE_FITS, vflags);
			vi->setValueInteger (val);
			master->updateMetaInformations (vi);
		}
	}
	else if (isValueCommand (cmd, "string", vflags))
	{
		char *val;
		if (paramNextString (&vname) || paramNextString (&desc) || ((val = paramNextWholeString ()) == NULL))
			throw rts2core::Error ("invalid string");
		v = ((rts2core::Daemon *) master)->getOwnValue (vname);
		// either variable with given name exists..
		if (v)
		{
			if (v->getValueBaseType () != RTS2_VALUE_STRING)
				throw rts2core::Error (std::string ("value is not string ") + vname);
			testWritableVariable (cmd, vflags, v);
			((rts2core::ValueInteger *) v)->setValueCharArr (val);
			((rts2core::Daemon *) master)->sendValueAll (v);
		}
		// or create it and distribute it..
		else
		{
			rts2core::ValueString *vi;
			((rts2core::Daemon *) master)->createValue (vi, vname, desc, vflags & RTS2_VALUE_FITS, vflags);
			vi->setValueCharArr (val);
			master->updateMetaInformations (vi);
		}
	}
	else if (isValueCommand (cmd, "bool", vflags) || isValueCommand (cmd, "onoff", vflags))
	{
		char *val;
		if (paramNextString (&vname) || paramNextString (&desc) || paramNextString (&val) || !paramEnd ())
			throw rts2core::Error ("missing variable name or description");
		v = ((rts2core::Daemon *) master)->getOwnValue (vname);
		// either variable with given name exists..
		if (v)
		{
			if (v->getValueBaseType () != RTS2_VALUE_BOOL)
				throw rts2core::Error (std::string ("value is not boolean ") + vname);
			testWritableVariable (cmd, vflags, v);
			((rts2core::ValueInteger *) v)->setValueCharArr (val);
			((rts2core::Daemon *) master)->sendValueAll (v);
		}
		// or create it and distribute it..
		else
		{
			rts2core::ValueBool *vi;
			if (!strcasecmp (cmd, "bool"))
				((rts2core::Daemon *) master)->createValue (vi, vname, desc, vflags & RTS2_VALUE_FITS, vflags);
			else
				((rts2core::Daemon *) master)->createValue (vi, vname, desc, vflags & RTS2_VALUE_FITS, vflags | RTS2_DT_ONOFF);
			vi->setValueCharArr (val);
			master->updateMetaInformations (vi);
		}
	}
	else if (isValueCommand (cmd, "double_array", vflags))
	{
		if (paramNextString (&vname) || paramNextString (&desc))
			throw rts2core::Error ("missing variable name or description");
		v = ((rts2core::Daemon *) master)->getOwnValue (vname);
		rts2core::DoubleArray *vad;
		if (v)
		{
			if (v->getValueType () != (RTS2_VALUE_ARRAY | RTS2_VALUE_DOUBLE))
				throw rts2core::Error (std::string ("value is not double array") + vname);
			testWritableVariable (cmd, vflags, v);
			vad = (rts2core::DoubleArray *) v;
			vad->clear ();
		}
		else
		{
			((rts2core::Daemon *) master)->createValue (vad, vname, desc, vflags & RTS2_VALUE_FITS, vflags);
			master->updateMetaInformations (vad);
		}
		while (!paramEnd ())
		{
			double vd;
			if (paramNextDouble (&vd))
				throw rts2core::Error ("invalid double value");
			vad->addValue (vd);
		}
		((rts2core::Daemon *) master)->sendValueAll (vad);
	}
	else if (isValueCommand (cmd, "stat", vflags))
	{
		if (paramNextString (&vname) || paramNextString (&desc))
			throw rts2core::Error ("missing variable name or description");
		v = ((rts2core::Daemon *) master)->getOwnValue (vname);
		rts2core::ValueDoubleStat *vds;
		if (v)
		{
			if (v->getValueType () != (RTS2_VALUE_STAT | RTS2_VALUE_DOUBLE))
				throw rts2core::Error (std::string ("value is not double stat") + vname);
			testWritableVariable (cmd, vflags, v);
			vds = (rts2core::ValueDoubleStat *) v;
			vds->clearStat ();
		}
		else
		{
			((rts2core::Daemon *) master)->createValue (vds, vname, desc, vflags & RTS2_VALUE_FITS, vflags);
			master->updateMetaInformations (vds);
		}
		int num;
		if (paramNextInteger (&num))
			throw rts2core::Error ("missing integer count");
		while (!paramEnd ())
		{
			double vd;
			if (paramNextDouble (&vd))
				throw rts2core::Error ("invalid double value");
			vds->addValue (vd, num);
		}
		vds->calculate ();
		((rts2core::Daemon *) master)->sendValueAll (vds);
	}
	else if (!strcasecmp (cmd, "register_randevous"))
	{
		if (paramNextString (&vname))
			throw rts2core::Error ("missing randevous variable name");
		v = ((rts2core::Daemon *) master)->getOwnValue (vname);
		if (v == NULL)
		{
			rts2core::ValueString *vs;
			((rts2core::Daemon *) master)->createValue (vs, vname, "randevous variable", false, RTS2_VALUE_WRITABLE);
			v = vs;
		}
		v->setRandevous ();
		master->updateMetaInformations (v);
		// create synronization value
		if (paramNextString (&value))
			throw rts2core::Error ("missing lock variable name");
		rts2core::Value *vsynch = ((rts2core::Daemon *) master)->getOwnValue (value);
		if (vsynch == NULL)
		{
			rts2core::ValueBool *vb;
			((rts2core::Daemon *) master)->createValue (vb, value, "lock variable", false, RTS2_VALUE_WRITABLE);
			v = vb;
			master->updateMetaInformations (v);
		}
		v->setValueInteger (0);
	}
	else if (!strcasecmp (cmd, "wait_randevous"))
	{

	}
	else if (!strcasecmp (cmd, "value_create") || !strcasecmp (cmd, "value_delete") || !strcasecmp(cmd, "value_add") || !strcasecmp (cmd, "double_array_add") || !strcasecmp (cmd, "stat_add"))
	{
		// Forward the commands directly to the device running the script
		((rts2core::Device *) master)->commandAuthorized (this);
		return;
	}
	else
	{
		throw rts2core::Error (std::string ("unknow command ") + cmd);
	}
}

void ConnExe::processLine ()
{
	rts2core::ConnFork::processLine ();

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

rts2core::Connection *ConnExe::getConnectionForScript (const char *_name)
{
	if (isCentraldName (_name))
		return getMaster ()->getSingleCentralConn ();
	return getMaster ()->getOpenConnection (_name);
}

int ConnExe::getDeviceType (const char *_name)
{
	#define NUMTYPES   13
	struct { const char *dev; int type; } types[NUMTYPES] =
	{
		{"TELESCOPE", DEVICE_TYPE_MOUNT},
		{"CCD",       DEVICE_TYPE_CCD},
		{"DOME",      DEVICE_TYPE_DOME},
		{"WEATHER",   DEVICE_TYPE_WEATHER},
		{"PHOT",      DEVICE_TYPE_PHOT},
		{"PLAN",      DEVICE_TYPE_PLAN},
		{"FOCUS",     DEVICE_TYPE_FOCUS},
		{"COPULA",    DEVICE_TYPE_CUPOLA},
		{"FW",        DEVICE_TYPE_FW},
		{"SENSOR",    DEVICE_TYPE_SENSOR},
		{"EXEC",      DEVICE_TYPE_EXECUTOR},
		{"SELECTOR",  DEVICE_TYPE_SELECTOR},
		{"IMGP",      DEVICE_TYPE_IMGPROC}
	};

	for (int i = 0; i < NUMTYPES; i++)
		if (!strcasecmp (_name, types[i].dev))
			return types[i].type;
	#undef NUMTYPES
	throw rts2core::Error (std::string ("unknow device type ") + _name);
}

bool ConnExe::checkActive (bool report)
{
	if (active)
		return true;
	if (report)
		writeToProcess ("& script is not active");
	return false;
}
