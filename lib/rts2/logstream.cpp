/*
 * Log steam, used for logging output.
 * Copyright (C) 2006-2009 Petr Kubanek <petr@kubanek.net>
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

#include "app.h"
#include "logstream.h"

#include <iomanip>
#include <iostream>

using namespace rts2core;

void LogStream::logArr (const char *arr, int len)
{
	bool lastIsHex = false;
	for (int i = 0; i < len; i++)
	{
		if (isprint (arr[i]))
		{
			*this << arr[i];
			lastIsHex = false;
		}
		else
		{
			int b = arr[i];
			if (!lastIsHex)
				*this << ' ';
			*this << "0x" << std::setfill ('0') << std::hex << std::setw (2) << (0x000000ff & b) << ' ';
			lastIsHex = true;
		}
	}
}

void LogStream::logArrAsHex (const char *arr, int len)
{
	for (int i = 0; i < len; i++)
	{
		int b = arr[i];
		*this << "0x" << std::setfill ('0') << std::hex << std::setw (2) << (0x000000ff & b) << " ";
	}
}

void LogStream::sendLog ()
{
	if (masterApp != NULL)
		masterApp->sendMessage (messageType, ls.str ().c_str ());
	else
		std::cerr << "log " << ls.str () << std::endl;
}

void LogStream::sendLogNoEndl ()
{
	if (masterApp != NULL)
		masterApp->sendMessageNoEndl (messageType, ls.str ().c_str ());
	else
		std::cerr << "log " << ls.str ();
}

LogStream & sendLog (LogStream & _ls)
{
	_ls.sendLog ();
	return _ls;
}

LogStream & sendLogNoEndl (LogStream & _ls)
{
	_ls.sendLogNoEndl ();
	return _ls;
}
