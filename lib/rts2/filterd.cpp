/* 
 * Filter base class.
 * Copyright (C) 2003-2007 Petr Kubanek <petr@kubanek.net>
 * Copyright (C) 2014 Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
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

#include "filterd.h"

using namespace rts2filterd;

Filterd::Filterd (int in_argc, char **in_argv, const char *defName):rts2core::Device (in_argc, in_argv, DEVICE_TYPE_FW, defName)
{
	createValue (filter, "filter", "used filter", false, RTS2_VALUE_WRITABLE);

	addOption ('F', NULL, 1, "filter names, separated by : (double colon)");
}

Filterd::~Filterd (void)
{
}

int Filterd::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'F':
			return setFilters (optarg);
		default:
			return rts2core::Device::processOption (in_opt);
	}
	return 0;
}

int Filterd::initValues ()
{
	return rts2core::Device::initValues ();
}

int Filterd::info ()
{
	filter->setValueInteger (getFilterNum ());
	return rts2core::Device::info ();
}

int Filterd::setFilterNum (int new_filter)
{
	filter->setValueInteger (new_filter);
  	sendValueAll (filter);
	return 0;
}

int Filterd::getFilterNum ()
{
	return filter->getValueInteger ();
}

int Filterd::setValue (rts2core::Value * old_value, rts2core::Value * new_value)
{
	if (old_value == filter)
		return setFilterNumMask (new_value->getValueInteger ()) == 0 ? 0 : -2;
	return rts2core::Device::setValue (old_value, new_value);
}

int Filterd::homeFilter ()
{
	return -1;
}

int Filterd::setFilters (const char *filters)
{
	char *top;
	char *tf = new char[strlen (filters) + 1];
	strcpy (tf, filters);
	while (*tf)
	{
		// skip leading spaces
		while (*tf && (*tf == ':' || *tf == '"' || *tf == '\''))
			tf++;
		if (!*tf)
			break;
		top = tf;
		// find filter string
		while (*top && *top != ':' && *top != '"' && *top != '\'')
			top++;
		// it's natural end, add and break..
		if (!*top)
		{
			if (top != tf)
				filter->addSelVal (tf);
			break;
		}
		*top = '\0';
		filter->addSelVal (tf);
		tf = top + 1;
	}
	if (filter->selSize () == 0)
		return -1;
	return 0;
}

int Filterd::setFilterNumMask (int new_filter)
{
	int ret;
	maskState (FILTERD_MASK | BOP_EXPOSURE, FILTERD_MOVE | BOP_EXPOSURE, "filter move started");
	logStream (MESSAGE_INFO) << "moving filter from #" << filter->getValueInteger () << " (" << filter->getSelName () << ")" << " to #" << new_filter << "(" << filter->getSelName (new_filter) << ")" << sendLog;
	ret = setFilterNum (new_filter);
	infoAll ();
	if (ret == -1)
	{
		maskState (DEVICE_ERROR_MASK | FILTERD_MASK | BOP_EXPOSURE, DEVICE_ERROR_HW | FILTERD_IDLE, "filter movement failed");
		return ret;
	}
	logStream (MESSAGE_INFO) << "filter moved to #" << new_filter << " (" << filter->getSelName () << ")" << sendLog;
	maskState (FILTERD_MASK | BOP_EXPOSURE, FILTERD_IDLE);
	return ret;
}

int Filterd::setFilterNum (rts2core::Connection * conn, int new_filter)
{
	int ret;
	ret = setFilterNumMask (new_filter);
	if (ret == -1)
	{
		conn->sendCommandEnd (DEVDEM_E_HW, "filter set failed");
	}
	return ret;
}

int Filterd::commandAuthorized (rts2core::Connection * conn)
{
	if (conn->isCommand ("filter"))
	{
		int new_filter;
		if (conn->paramNextInteger (&new_filter) || !conn->paramEnd ())
			return -2;
		return setFilterNum (conn, new_filter);
	}
	else if (conn->isCommand ("home"))
	{
		if (!conn->paramEnd ())
			return -2;
		return homeFilter ();
	}
	else if (conn->isCommand ("help"))
	{
		conn->sendMsg ("info - information about camera");
		conn->sendMsg ("exit - exit from connection");
		conn->sendMsg ("help - print, what you are reading just now");
		return 0;
	}
	return rts2core::Device::commandAuthorized (conn);
}
