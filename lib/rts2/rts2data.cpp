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

#include "connection.h"
#include "rts2data.h"

using namespace rts2core;

int DataRead::readDataSize (Connection *conn)
{
	return conn->paramNextLong (&binaryReadChunkSize);
}

DataWrite::DataWrite (int channum, long *chansize)
{
	for (int i = 0; i < channum; i++)
		binaryWriteDataSize.push_back (chansize[i]);
}

long DataWrite::getDataSize ()
{
	long ret = 0;
	for (std::vector <long>::iterator iter = binaryWriteDataSize.begin (); iter != binaryWriteDataSize.end (); iter++)
		ret += *iter;
	return ret;
}

DataChannels::~DataChannels ()
{
	for (DataChannels::iterator iter = begin (); iter != end (); iter++)
		delete (*iter);
}

void DataChannels::initFromConnection (Connection *conn)
{
	int data_type, channum;
	if (conn->paramNextInteger (&data_type) || conn->paramNextInteger (&channum))
		throw Error ("cannot read number of channels");
	for (int i = 0; i < channum; i++)
	{
		long chansize;
		if (conn->paramNextLong (&chansize))
			throw Error ("cannot get parse channel size");
		push_back (new DataRead (chansize, data_type));
	}
	if (!conn->paramEnd ())
		throw Error ("too much parameters in PROTO_BINARY data header");
}

long DataChannels::getRestSize ()
{
	long ret = 0;
	for (DataChannels::iterator iter = begin (); iter != end (); iter++)
		ret += (*iter)->getRestSize ();
	return ret;	
}
