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

// maximal number of shared clients
#define MAX_SHARED_CLIENTS       10

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
		virtual ssize_t addData (char *data, ssize_t data_size) = 0;

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
		virtual size_t getRestSize () = 0;

		/**
		 * Return remaining size of chunk, which has to be read from actual data chunk.
		 */
		virtual size_t getChunkSize () = 0;
};

/**
 * Represents data readed from connection.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class DataRead:public DataAbstractRead
{
	public:
		DataRead (size_t in_binaryReadDataSize, int in_type)
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

		void setChunkSizeFromData () { binaryReadChunkSize = binaryReadDataSize; }

		/**
		 * Receive data from socket.
		 *
		 * @return -1 on error, otherwise size of read data.
		 */
		virtual int getData (int sock)
		{
			ssize_t data_size;
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

		virtual ssize_t addData (char *data, ssize_t data_size)
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

		virtual size_t getRestSize () { return binaryReadDataSize; }

		virtual size_t getChunkSize () { return binaryReadChunkSize; }

	private:
		// binary data
		// when it is positive, there are binary data to read from connection
		// when it is 0, we should send confirmation that we received binary data
		// when it is negative, there aren't any data waiting
		size_t binaryReadDataSize;

		char *binaryReadBuff;
		char *binaryReadTop;

		// type of data we are reading
		int binaryReadType;

		// remaining size of binary data chunk which needed to be read
		ssize_t binaryReadChunkSize;
};

/**
 * Shared data header structure.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
struct SharedDataHeader
{
	// number of data buffers.
	int nseg;
	// semaphore associated with data; it has nbuffers values, each associated with a single buffer
	int shared_sem;
	// shared client IDs, segment sizes - SharedData - follows immediately this field
};

/**
 * Shared data segment header. Described segments of shared data.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
struct SharedDataSegment
{
	// ID of client connection reading data. Data cannot be reused if this field is non-empty.
	int client_ids[MAX_SHARED_CLIENTS];
	// segment size
	size_t size;
	// size of written data (so far; process can update this as new data arrives
	size_t bytesSoFar;
	// segment offset
	size_t offset;
};

/**
 * Encampulates basic shared data management functions.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class DataAbstractShared
{
	public:
		DataAbstractShared () { data = NULL; shm_id = -1; }
		DataAbstractShared (DataAbstractShared *d) { data = d->data; shm_id = -1; }

		int getShmId () { return shm_id; }

		/**
		 * Remove client from reader set.
		 */
		int removeClient (int segnum, int client_id, bool verbose = true);

	protected:
		struct SharedDataSegment *getSegment (int segnum) { return (struct SharedDataSegment *) (((char *) data) + sizeof (struct SharedDataHeader) + segnum * sizeof (struct SharedDataSegment)); }

		// semaphore op
		int lockSegment (int seg);
		int unlockSegment (int seg);

		struct SharedDataHeader *data;
		int shm_id;
};

/**
 * Shared memory data - handles both client and server side of data connections.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class DataSharedRead: public DataAbstractRead, public DataAbstractShared
{
	public:
		/**
		 * Crate new DataSharedRead structure, prepare it for attach call.
		 */
		DataSharedRead () { data = NULL; segment = -1; activeSegment = NULL; shm_id = -1; }


		/**
		 * Initialize data from existing, already mapped shared memory segment.
		 *
		 * @param _data   
		 * @param _seg
		 */
		DataSharedRead (DataSharedRead *_data, int _seg) { data = _data->data; segment = _seg; activeSegment = getSegment (_seg); shm_id = -1; }

		virtual ~DataSharedRead ();

		int attach (int _shm_id);

		virtual int readDataSize (Connection *conn) { return 0; }
		virtual ssize_t addData (char *_data, ssize_t _data_size) { return -1; }
		virtual int getData (int sock) { return -1; }
		virtual char *getDataBuff () { return ((char *) data) + activeSegment->offset; }
		virtual char *getDataTop () { return ((char *) data) + activeSegment->offset + activeSegment->bytesSoFar; }
		virtual size_t getRestSize () { return activeSegment->size - activeSegment->bytesSoFar; }
		virtual size_t getChunkSize () { return getRestSize (); }

		int confirmClient (int segnum, int client_id);
		int removeActiveClient (int client_id) { return removeClient (segment, client_id); }

	private:
		// shared data segment
		struct SharedDataSegment *activeSegment;
		int segment;
};

/**
 * Abstract class for sending data.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class DataAbstractWrite
{
	public:
		DataAbstractWrite () {}
		virtual ~DataAbstractWrite () {}

		virtual size_t getDataSize () = 0;
		virtual size_t getChannelSize (int chan) = 0;

		virtual void dataWritten (int chan, size_t size) = 0;

		virtual void endChannels () {}
};

/**
 * Represents data written to connection through socket.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class DataWrite:public DataAbstractWrite
{
	public:
		DataWrite (int _channum, size_t *chansizes);
		virtual ~DataWrite ();

		virtual size_t getDataSize ();
		virtual size_t getChannelSize (int chan) { return binaryWriteDataSize[chan]; }
		virtual void dataWritten (int chan, size_t size) { binaryWriteDataSize[chan] -= size; }

	private:
		// connection data size
		size_t *binaryWriteDataSize;
		int channum;
};

/**
 * Keeps track of shared memory writes.
 */
class DataSharedWrite:public DataAbstractWrite, public DataAbstractShared
{
	public:
		/**
		 * Fill empty shared data structure. This constructor needs to be followed by create call.
		 */
		DataSharedWrite ():DataAbstractWrite (), DataAbstractShared () {}

		/**
		 * Shallow copy from existing data connection.
		 */
		DataSharedWrite (DataSharedWrite *d):DataAbstractWrite (), DataAbstractShared (d) { chan2seg = d->chan2seg; }

		virtual ~DataSharedWrite ();

		/**
		 * Create new shared data.
		 */
		struct SharedDataHeader *create (int numseg, size_t segsize);

		virtual size_t getDataSize ();
		virtual size_t getChannelSize (int chan) { return chan2seg[chan]->size - chan2seg[chan]->bytesSoFar; }
		virtual void dataWritten (int chan, size_t size) { chan2seg[chan]->bytesSoFar += size; }

		/**
		 * Find unused segment, allocate it for a single client.
		 *
		 * @param segsize   segment size
		 * @param chan      channel for which segment is allocated
		 */
		int addClient (size_t segsize, int chan, int client);

		virtual void endChannels ();
		void clearChan2Seg () { chan2seg.clear (); }

		void *getChannelData (int chan) { return ((char *) data) + chan2seg[chan]->offset; }

	private:
		// maps channels to segments
		std::map <int, struct SharedDataSegment *> chan2seg;
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
		 * Initiliaze shared channels. Expect to find channels and shared memory segments.
		 */
		void initSharedFromConnection (Connection *conn, DataSharedRead *shm);

		/**
		 * Read data for given channel.
		 *
		 * @param chan  channel number
		 * @param conn  connection which will be used to read the data
		 */
		int readChannel (int chan, Connection *conn) { return at(chan)->readDataSize (conn); }

		ssize_t addData (int chan, char *_data, ssize_t data_size) { return at(chan)->addData (_data, data_size); }

		int getData (int chan, int sock) { return at(chan)->getData (sock); }

		/**
		 * Return remaining size of chunk, which has to be read from actual data chunk.
		 */
		size_t getChunkSize (int chan) { return at(chan)->getChunkSize (); }

		size_t getRestSize ();
};

}
#endif							 // ! __RTS2_DATA__
