/* 
 * Image channel class.
 * Copyright (C) 2010,2012 Petr Kubánek <petr@kubanek.net>
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

#ifndef __RTS2_CHANNEL__
#define __RTS2_CHANNEL__

#include <vector>
#ifdef RTS2_HAVE_MALLOC_H
#include <malloc.h>
#endif
#include <sys/types.h>

namespace rts2image
{

/**
 * Single channel of an image.
 *
 * @author Petr Kubánek <petr@kubanek.net>
 */
class Channel
{
	public:
		Channel (int16_t _dataType);

		/**
                 * Creates channel data. Can deallocate data if needed.
		 *
		 * @param ch         channel number
		 * @param _data      channel data
		 * @param _naxis     number of axis in channel
		 * @param _sizes     size of image (size of this array must be equal to _naxis parameter)
		 * @param _dataType  type of data in channel. Uses FITS datatype notation
		 * @param dealloc    whenever to deallocate data at desctruction
		 */
		Channel (int ch, char *_data, int _naxis, long *_sizes, int16_t _dataType, bool dealloc);

		/**
                 * Creates channel data. Can deallocate data if needed.
		 *
		 * @param ch         channel number
		 * @param _data      channel data
		 * @param dataSize   size of channel data
		 * @param _naxis     number of axis in channel
		 * @param _sizes     size of image (size of this array must be equal to _naxis parameter)
		 * @param _dataType  type of data in channel. Uses FITS datatype notation
		 */
		Channel (int ch, char *_data, long dataSize, int _naxis, long *_sizes, int16_t _dataType);

		~Channel ();

		/**
		 * Returns channel number.
		 *
		 * @return channel number
		 */
		int getChannelNumber () { return channelnum; }

		// those values will become available after call to computeStatistics
		long double getPixelSum () { return pixelSum; }
		double getAverage () { return average; }
		double getStDev () { return stdev; }

		const int16_t getDataType () { return dataType; }

		/**
		 * Return number of axes in channel.
		 */
		const int getNaxis () { return naxis; }

		const long getSize (int nax) { return sizes[nax]; }

		const long getWidth () { return naxis > 0 ? sizes[0] : 0; }
		const long getHeight () { return naxis > 1 ? sizes[1] : 0; }
		const long getNPixels () { return getWidth () * getHeight (); }

		const char *getData () { return (char *) data; }

		void computeStatistics (size_t _from = 0, size_t _dataSize = 0);

	private:
		char *data;
		int naxis;
		long *sizes;
		bool allocated;

		int16_t dataType;

		long double pixelSum;
		double average;
		double stdev;

		// channel number
		int channelnum;
};

class Channels:public std::vector<Channel *>
{
	public:
		Channels ();
		~Channels ();
};

}

#endif /* __RTS2_CHANNEL__ */
