/* 
 * Variant of SCPI connection used by ThorLabs.
 * Copyright (C) 2012 Petr Kubanek <petr@kubanek.net>
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

#include "connection/thorlabs.h"

using namespace rts2core;

ConnThorLabs::ConnThorLabs (const char *device_file, Block * _master, int _thorlabsType):ConnSerial (device_file, _master, rts2core::BS115200, rts2core::C8, rts2core::NONE, 10)
{
	thorlabsType = _thorlabsType;
}

int ConnThorLabs::getValue (const char *vname, rts2core::Value *value)
{
	int ret = sprintf (buf, "%s?\r", vname);

	ret = writeRead (buf, ret, buf, 50, '\r');
	if (ret < 0)
		return ret;
	switch (thorlabsType)
	{
		case LASER:
			if ((buf[0] != '<' && buf[0] != '>') || strncmp (buf + 2, vname, strlen (vname)))
			{
				buf[ret] = '\0';
				logStream (MESSAGE_ERROR) << "invalid reply while quering for value " << vname << " : " << buf << sendLog;
				return -1;
			}
			ret = readPort (buf, 50, '\r');
			if (ret < 0)
				return ret;
			if (ret == 0)
				throw rts2core::Error ("empty reply");
			buf[ret - 1] = '\0';
			if (value->getValueBaseType () == RTS2_VALUE_BOOL)
				((rts2core::ValueBool *) value)->setValueBool (buf[0] == '1');
			else	
				value->setValueCharArr (buf);
			return 0;
		case FW:
			if (strncmp (buf, vname, strlen (vname)))
			{
				buf[ret] = '\0';
				logStream (MESSAGE_ERROR) << "invalid reply while quering for value " << vname << " : " << buf << sendLog;
				return -1;
			}
			ret = readPort (buf, 50, '\r');
			if (ret < 0)
				return ret;
			if (ret == 0)
				throw rts2core::Error ("empty reply");
			buf[ret - 1] = '\0';
			if (value->getValueBaseType () == RTS2_VALUE_BOOL)
				((rts2core::ValueBool *) value)->setValueBool (buf[0] == '1');
			else	
				value->setValueCharArr (buf);
			ret = readPort (buf, 50, ' ');
			if (ret < 0)
				return ret;
			return 0;
	}
	return -1;
}

int ConnThorLabs::setValue (const char *vname, rts2core::Value *value)
{
	int ret = sprintf (buf, "%s=%s\r", vname, value->getValue ());

	ret = writeRead (buf, strlen (buf), buf, 50, '\r');
	if (ret < 0)
		return ret;
	return -1;
}

int ConnThorLabs::getInt (const char *vname, int &value)
{
	int ret = sprintf (buf, "%s?\r", vname);

	ret = writeRead (buf, ret, buf, 50, '\r');
	if (ret < 0)
		return ret;
	switch (thorlabsType)
	{
		case LASER:
			if ((buf[0] != '<' && buf[0] != '>') || strncmp (buf + 2, vname, strlen (vname)))
			{
				buf[ret] = '\0';
				logStream (MESSAGE_ERROR) << "invalid reply while quering for value " << vname << " : " << buf << sendLog;
				return -1;
			}
			ret = readPort (buf, 50, '\r');
			if (ret < 0)
				return ret;
			if (ret == 0)
				throw rts2core::Error ("empty reply");
			buf[ret - 1] = '\0';
			value = atoi (buf);
			return 0;
		case FW:
			if (strncmp (buf, vname, strlen (vname)))
			{
				buf[ret] = '\0';
				logStream (MESSAGE_ERROR) << "invalid reply while quering for value " << vname << " : " << buf << sendLog;
				return -1;
			}
			ret = readPort (buf, 50, '\r');
			if (ret < 0)
				return ret;
			if (ret == 0)
				throw rts2core::Error ("empty reply");
			buf[ret - 1] = '\0';
			value = atoi (buf);
			ret = readPort (buf, 50, ' ');
			if (ret < 0)
				return ret;
			return 0;
	}
	return 0;
}

int ConnThorLabs::setInt (const char *vname, int value)
{
	int ret = sprintf (buf, "%s=%d\r", vname, value);

	ret = writeRead (buf, strlen (buf), buf, 50, '\r');
	if (ret < 0)
		return ret;
	return -1;
}
