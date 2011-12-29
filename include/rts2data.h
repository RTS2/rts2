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

#ifndef __RTS2_DATA__
#define __RTS2_DATA__

#include <errno.h>
#include <unistd.h>
#include <vector>

namespace rts2core
{

class Connection;

/**
 * Interface for varius DataRead classes.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class DataAbstractRead
{
	public:
		DataAbstractRead () {}
		virtual ~DataAbstractRead () {}

		/**
		 * Read data size from connection.
		 */
		virtual int readDataSize (Connection *conn) = 0;

		/**
		 * Adds data to the buffer.
		 *
		 * @param data Data to add.
		 * @param data_size Size of data.
		 *
		 * @return Actual size of data added.
		 */
		virtual long addData (char *data, long data_size) = 0;

		/**
		 * Receive data from socket.
		 *
		 * @return -1 on error, otherwise size of read data.
		 */
		virtual int getData (int sock) = 0;
		
		virtual char *getDataBuff () = 0;
		virtual char *getDataTop () = 0;

		/**
		 * Return size of data to read.
		 *
		 * @return Size of data which remains to be read.
		 */
		virtual long getRestSize () = 0;

		/**
		 * Return remaining size of chunk, which has to be read from actual data chunk.
		 */
		virtual long getChunkSize () = 0;
};

/**
 * Represents data readed from connection.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class DataRead:public DataAbstractRead
{
	public:
		DataRead (long in_binaryReadDataSize, int in_type)
		{
			binaryReadDataSize = in_binaryReadDataSize;
			binaryReadBuff = new char[binaryReadDataSize];
			binaryReadTop = binaryReadBuff;
			binaryReadType = in_type;
			binaryReadChunkSize = -1;
		}

		~DataRead (void)
		{
			delete[] binaryReadBuff;
		}

		virtual int readDataSize (Connection *conn);

		/**
		 * Receive data from socket.
		 *
		 * @return -1 on error, otherwise size of read data.
		 */
		virtual int getData (int sock)
		{
			int data_size;
			data_size = read (sock, binaryReadTop, binaryReadChunkSize);
			if (data_size == -1)
			{
				// ignore EINTR
				if (errno == EINTR)
					return 0;
				return -1;
			}

			binaryReadDataSize -= data_size;
			binaryReadChunkSize -= data_size;
			binaryReadTop += data_size;
			return data_size;
		}

		virtual long addData (char *data, long data_size)
		{
			if (data_size > binaryReadChunkSize)
				data_size = binaryReadChunkSize;
			memcpy (binaryReadTop, data, data_size);
			binaryReadTop += data_size;
			binaryReadDataSize -= data_size;
			binaryReadChunkSize -= data_size;
			return data_size;
		}

		virtual char *getDataBuff () { return binaryReadBuff; }

		virtual char *getDataTop () { return binaryReadTop; }

		virtual long getRestSize () { return binaryReadDataSize; }

		virtual long getChunkSize () { return binaryReadChunkSize; }

	private:
		// binary data
		// when it is positive, there are binary data to read from connection
		// when it is 0, we should send confirmation that we received binary data
		// when it is negative, there aren't any data waiting
		long binaryReadDataSize;

		char *binaryReadBuff;
		char *binaryReadTop;

		// type of data we are reading
		int binaryReadType;

		// remaining size of binary data chunk which needed to be read
		long binaryReadChunkSize;
};

/**
 * Shared memory data - returns buffer specified in constructor.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class DataShared: public DataAbstractRead
{
	public:
		DataShared (char *_data) { data = _data; }

		virtual int readDataSize (Connection *conn) { return 0; }
		virtual long addData (char *_data, long data_size) { return -1; }
		virtual int getData (int sock) { return -1; }
		virtual char *getDataBuff () { return data + sizeof (unsigned long); }
		virtual char *getDataTop () { return data + *((unsigned long *) data) + sizeof (unsigned long); }
		virtual long getRestSize () { return 0; }
		virtual long getChunkSize () { return -1; }
	private:
		char *data;
};

/**
 * Represents data written to connection.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class DataWrite
{
	public:
		DataWrite (int channum, long *chansize);
		long getDataSize ();
		long getChannelSize (int chan) { return binaryWriteDataSize[chan]; }

		void dataWritten (int chan, long size) { binaryWriteDataSize[chan] -= size; }

	private:
		// connection data size
		std::vector <long> binaryWriteDataSize;
};

/**
 * Structure holding all channels of the data.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class DataChannels:public std::vector <DataAbstractRead *>
{
	public:
		DataChannels () {}
		~DataChannels ();

		/**
		 * Initiliaze data channels from connection. Expect to receive number of channels and their size.
		 *
		 * @param conn connection from which data channel will be initialized
		 */
		void initFromConnection (Connection *conn);

		/**
		 * Read data for given channel.
		 *
		 * @param chan  channel number
		 * @param conn  connection which will be used to read the data
		 */
		int readChannel (int chan, Connection *conn) { return at(chan)->readDataSize (conn); }

		long addData (int chan, char *_data, long data_size) { return at(chan)->addData (_data, data_size); }

		int getData (int chan, int sock) { return at(chan)->getData (sock); }

		/**
		 * Return remaining size of chunk, which has to be read from actual data chunk.
		 */
		long getChunkSize (int chan) { return at(chan)->getChunkSize (); }

		long getRestSize ();
};

}
#endif							 // ! __RTS2_DATA__
