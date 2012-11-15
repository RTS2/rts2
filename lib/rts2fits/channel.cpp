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

#include "rts2fits/channel.h"
#include "error.h"
#include "imghdr.h"
#include "nan.h"

#include <malloc.h>
#include <string.h>
#include <math.h>
#include <iostream>

using namespace rts2image;

Channel::Channel (int16_t _dataType)
{
	channelnum = 0;
	data = NULL;
	allocated = false;

	dataType = _dataType;

	naxis = 0;
	sizes = NULL;

	pixelSum = average = stdev = NAN;
}

Channel::Channel (int ch, char *_data, int _naxis, long *_sizes, int16_t _dataType, bool dealloc)
{
	channelnum = ch;

	data = _data;
	allocated = dealloc;

	naxis = _naxis;

	dataType = _dataType;

	sizes = new long [naxis];
	memcpy (sizes, _sizes, naxis * sizeof (long));

	pixelSum = average = stdev = NAN;
}


Channel::Channel (int ch, char *_data, long dataSize, int _naxis, long *_sizes, int16_t _dataType)
{
	channelnum = ch;

	data = new char [dataSize];
	memcpy (data, _data, dataSize);
	allocated = true;

	naxis = _naxis;

	dataType = _dataType;

	sizes = new long [naxis];
	memcpy (sizes, _sizes, naxis * sizeof (long));

	pixelSum = average = stdev = NAN;
}

Channel::~Channel ()
{
	if (allocated)
		delete[] data;
	delete[] sizes;
}

template <typename pixel_type> void computeDataStatistics (pixel_type *data, long totalPixels, long double &pixelSum, double &average, double &stdev)
{
	// calculate average of all channels..
	pixel_type *pixel = data;
	pixel_type *fullTop = pixel + totalPixels;

	pixelSum = 0;

	while (pixel < fullTop)
	{
		pixelSum += *pixel;
		pixel++;
	}
	if (totalPixels > 0)
	{
		average = pixelSum / totalPixels;
		// calculate stdev
		pixel = data;
		stdev = 0;
		while (pixel < fullTop)
		{
			long double tmp_s = *pixel - average;
			long double tmp_ss = tmp_s * tmp_s;
			stdev += tmp_ss;
			pixel++;
		}
		stdev = sqrt (stdev / totalPixels);
	}
	else
	{
		average = 0;
		stdev = 0;
	}
}

void Channel::computeStatistics (size_t _from, size_t _dataSize)
{
	if (_dataSize == 0)
		_dataSize = getNPixels ();
	switch (dataType)
	{
		case RTS2_DATA_BYTE:
			computeDataStatistics ((unsigned char *) (getData ()) + _from, _dataSize, pixelSum, average, stdev);
			break;
		case RTS2_DATA_SHORT:
			computeDataStatistics ((int16_t *) (getData ()) + _from, _dataSize, pixelSum, average, stdev);
			break;
		case RTS2_DATA_LONG:
			computeDataStatistics ((int32_t *) (getData ()) + _from, _dataSize, pixelSum, average, stdev);
			break;
		case RTS2_DATA_LONGLONG:
			computeDataStatistics ((int64_t *) (getData ()) + _from, _dataSize, pixelSum, average, stdev);
			break;
		case RTS2_DATA_FLOAT:
			computeDataStatistics ((float *) (getData ()) + _from, _dataSize, pixelSum, average, stdev);
			break;
		case RTS2_DATA_DOUBLE:
			computeDataStatistics ((double *) (getData ()) + _from, _dataSize, pixelSum, average, stdev);
			break;
		case RTS2_DATA_SBYTE:
			computeDataStatistics ((signed char *) (getData ()) + _from, _dataSize, pixelSum, average, stdev);
			break;
		case RTS2_DATA_USHORT:
			computeDataStatistics ((uint16_t *) (getData ()) + _from, _dataSize, pixelSum, average, stdev);
			break;
		case RTS2_DATA_ULONG:
			computeDataStatistics ((uint32_t *) (getData ()) + _from, _dataSize, pixelSum, average, stdev);
			break;
		default:
			throw rts2core::Error ("unknow dataType");
	}
}

Channels::Channels ()
{
}

Channels::~Channels ()
{
	for (Channels::iterator iter = begin (); iter != end (); iter++)
		delete (*iter);
}
