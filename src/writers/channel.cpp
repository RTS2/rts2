/* 
 * Image channel class.
 * Copyright (C) 2010 Petr Kubánek <petr@kubanek.net>
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

#include "channel.h"

#include <malloc.h>
#include <string.h>

using namespace rts2image;

Channel::Channel ()
{
	data = NULL;
	naxis = 0;
	sizes = NULL;
	allocated = false;
}

Channel::Channel (char *_data, int _naxis, long *_sizes)
{
	data = _data;

	naxis = _naxis;

	sizes = new long [naxis];
	memcpy (sizes, _sizes, naxis * sizeof (long));

	allocated = false;
}


Channel::Channel (char *_data, long dataSize, int _naxis, long *_sizes)
{
	data = new char [dataSize];
	memcpy (data, _data, dataSize);

	naxis = _naxis;

	sizes = new long [naxis];
	memcpy (sizes, _sizes, naxis * sizeof (long));

	allocated = true;
}

Channel::~Channel ()
{
	if (allocated)
		delete[] data;
	delete[] sizes;
}

Channels::Channels ()
{
}

Channels::~Channels ()
{
	for (Channels::iterator iter = begin (); iter != end (); iter++)
		delete (*iter);
}
