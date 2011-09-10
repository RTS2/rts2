/* 
 * EPICS interface.
 * Copyright (C) 2009 Petr Kubanek <petr@kubanek.net>
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

#include "connepics.h"

using namespace rts2core;


EpicsVal * ConnEpics::findValue (rts2core::Value *value)
{
	for (std::list <EpicsVal>::iterator iter = channels.begin (); iter != channels.end (); iter++)
	{
		if ((*iter).value == value)
			return &(*iter);
	}
	throw ConnEpicsError ("value not found", ECA_NORMAL);
}

ConnEpics::ConnEpics (rts2core::Block *_master):Rts2ConnNoSend (_master)
{
	setConnTimeout (5);
}

ConnEpics::~ConnEpics ()
{
	ca_context_destroy ();
}

int ConnEpics::init ()
{
	int res;
	res = ca_context_create (ca_disable_preemptive_callback);
	if (res != ECA_NORMAL)
	{
		throw ConnEpicsError ("CA error occured while trying to start channel access, error: ", res);
	}
	return 0;
}

void ConnEpics::addValue (rts2core::Value *value, const char *pvname)
{
  	chid pchid = createChannel (pvname);
	channels.push_back (EpicsVal (value, pchid));
}

chid ConnEpics::createChannel (const char *pvname)
{
 	chid pchid;
	int result;
	result = ca_create_channel (pvname, NULL, NULL, CA_PRIORITY_DEFAULT, &pchid);
	if (result != ECA_NORMAL)
	{
		throw ConnEpicsErrorChannel ("error while creating channel", pvname, result);
	}
	return pchid;
}

void ConnEpics::deleteChannel (chid _ch)
{
	int result = ca_clear_channel (_ch);
	if (result != ECA_NORMAL)
	{
		throw ConnEpicsError ("cannot delete channel", result);
	}
}

void ConnEpics::get (chid _ch, double *val)
{
	int result = ca_array_get (DBR_DOUBLE, 1, _ch, val);
	if (result != ECA_NORMAL)
	{
		throw ConnEpicsErrorChannel ("cannot get value from channel", ca_name (_ch), result);
	}
}

void ConnEpics::queueGetValue (rts2core::Value *value)
{
	int result;
	chtype pchtype;
	EpicsVal *val = findValue (value);
	if (val->storage != NULL)
		return;
	switch (value->getValueBaseType ())
	{
		case RTS2_VALUE_DOUBLE:
			pchtype = DBR_DOUBLE;
			val->storage = malloc (sizeof (double));
			break;
		case RTS2_VALUE_SELECTION:
			pchtype = DBR_DOUBLE;
			val->storage = malloc (sizeof (double));
			break;
		case RTS2_VALUE_INTEGER:
			pchtype = DBR_INT;
			val->storage = malloc (sizeof (int));
			break;
		case RTS2_VALUE_LONGINT:
			pchtype = DBR_LONG;
			val->storage = malloc (sizeof (long));
			break;
		case RTS2_VALUE_BOOL:
			pchtype = DBR_INT;
			val->storage = malloc (sizeof (int));
			break;
		default:
			throw ConnEpicsErrorChannel ("queueGetValue unsupported value type ", ca_name (val->vchid), ECA_NORMAL);
	}
	result = ca_array_get (pchtype, 1, val->vchid, val->storage);
	if (result != ECA_NORMAL)
	{
		free (val->storage);
		val->storage = NULL;
		throw ConnEpicsErrorChannel ("errro calling ca_array_get for ", ca_name (val->vchid), result);
	}
}

void ConnEpics::queueSetValue (rts2core::Value *value)
{
	int result;
	chtype pchtype;
	EpicsVal *val = findValue (value);
	void *data = NULL;
	switch (value->getValueBaseType ())
	{
		case RTS2_VALUE_DOUBLE:
			pchtype = DBR_DOUBLE;
			data = malloc (sizeof (double));
			*((double *)data) = ((rts2core::ValueDouble *) value)->getValueDouble ();
			break;
		case RTS2_VALUE_SELECTION:
			pchtype = DBR_DOUBLE;
			data = malloc (sizeof (double));
			*((double *)data) = ((rts2core::ValueDouble *) value)->getValueInteger ();
			break;
		case RTS2_VALUE_INTEGER:
			pchtype = DBR_INT;
			data = malloc (sizeof (int));
			*((int *)data) = value->getValueInteger ();
			break;
		case RTS2_VALUE_LONGINT:
			pchtype = DBR_LONG;
			data = malloc (sizeof (long));
			*((long *)data) = value->getValueLong ();
			break;
		default:
			throw ConnEpicsErrorChannel ("queueSetValue unsupported value type ", ca_name (val->vchid), ECA_NORMAL);
	}
	result = ca_array_put (pchtype, 1, val->vchid, data);
	free (data);
	if (result != ECA_NORMAL)
	{
		throw ConnEpicsErrorChannel ("error calling ca_array_put for ", ca_name (val->vchid), result);
	}
}

void ConnEpics::queueSet (chid _vchid, int value)
{
	int result;
	result = ca_array_put (DBR_INT, 1, _vchid, &value);
	if (result != ECA_NORMAL)
	{
		throw ConnEpicsErrorChannel ("error calling ca_array_put for ", ca_name (_vchid), result);
	}
}

void ConnEpics::queueSet (chid _vchid, double value)
{
	int result;
	result = ca_array_put (DBR_DOUBLE, 1, _vchid, &value);
	if (result != ECA_NORMAL)
	{
		throw ConnEpicsErrorChannel ("error calling ca_array_put for ", ca_name (_vchid), result);
	}
}

void ConnEpics::queueSetEnum (chid _vchid, int value)
{
	int result;
	double val = value;
	result = ca_array_put (DBR_DOUBLE, 1, _vchid, &val);
	if (result != ECA_NORMAL)
	{
		throw ConnEpicsErrorChannel ("error calling ca_array_put for ", ca_name (_vchid), result);
	}
}

void ConnEpics::callPendIO ()
{
	int result;
	result = ca_pend_io (getConnTimeout ());
	if (result != ECA_NORMAL)
	{
		throw ConnEpicsError ("error while calling pend_io", result);
	}
	for (std::list <EpicsVal>::iterator iter = channels.begin (); iter != channels.end (); iter++)
	{
		if ((*iter).storage != NULL)
		{
			switch ((*iter).value->getValueBaseType ())
			{
				case RTS2_VALUE_DOUBLE:
					((rts2core::ValueDouble *) (*iter).value)->setValueDouble (*((double *) (*iter).storage));
					break;
				case RTS2_VALUE_SELECTION:
					((rts2core::ValueSelection *) (*iter).value)->setValueInteger (*((double *) (*iter).storage));
				case RTS2_VALUE_INTEGER:
					(*iter).value->setValueInteger (*((int*) (*iter).storage));
					break;
				case RTS2_VALUE_LONGINT:
					((rts2core::ValueLong *)(*iter).value)->setValueLong (*((long*) (*iter).storage));
					break;
				case RTS2_VALUE_BOOL:
					((rts2core::ValueBool *)(*iter).value)->setValueBool ((*((int*) (*iter).storage)) != 0);
					break;
				default:
					throw ConnEpicsErrorChannel ("callPendIO unsupported value type ", ca_name ((*iter).vchid), ECA_NORMAL);
			}
			free ((*iter).storage);
			(*iter).storage = NULL;
		}
	}
}
