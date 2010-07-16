/* 
 * Utilities for data connection.
 * Copyright (C) 2007,2010 Petr Kubanek <petr@kubanek.net>
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

#include "rts2conn.h"
#include "rts2data.h"

using namespace rts2core;

int DataRead::readDataSize (Rts2Conn *conn)
{
	return conn->paramNextLong (&binaryReadChunkSize);
}

DataChannels::~DataChannels ()
{
	for (std::vector <DataRead *>::iterator iter = begin (); iter != end (); iter++)
		delete (*iter);
}

long DataChannels::getRestSize ()
{
	long ret = 0;
	for (std::vector <DataRead *>::iterator iter = begin (); iter != end (); iter++)
		ret += (*iter)->getRestSize ();
	return ret;	
}
