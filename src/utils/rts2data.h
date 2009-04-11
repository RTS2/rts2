/* 
 * Utilities for data connection.
 * Copyright (C) 2007 Petr Kubanek <petr@kubanek.net>
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

class Rts2Conn;

/**
 * Represents data readed from connection.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Rts2DataRead
{
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
	public:
		Rts2DataRead (int in_binaryReadDataSize, int in_type)
		{
			binaryReadDataSize = in_binaryReadDataSize;
			binaryReadBuff = new char[binaryReadDataSize];
			binaryReadTop = binaryReadBuff;
			binaryReadType = in_type;
			binaryReadChunkSize = -1;
		}

		~Rts2DataRead (void)
		{
			delete[] binaryReadBuff;
		}

		/**
		 * Read data size from connection.
		 */
		int readDataSize (Rts2Conn *conn);

		/**
		 * Receive data from socket.
		 *
		 * @return -1 on error, otherwise size of readed data.
		 */
		int getData (int sock)
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

		/**
		 * Adds data to the buffer.
		 *
		 * @param data Data to add.
		 * @param data_size Size of data.
		 *
		 * @return Actual size of data added.
		 */
		long addData (char *data, long data_size)
		{
			if (data_size > binaryReadChunkSize)
				data_size = binaryReadChunkSize;
			memcpy (binaryReadTop, data, data_size);
			binaryReadTop += data_size;
			binaryReadDataSize -= data_size;
			binaryReadChunkSize -= data_size;
			return data_size;
		}

		char *getDataBuff ()
		{
			return binaryReadBuff;
		}

		char *getDataTop ()
		{
			return binaryReadTop;
		}

		/**
		 * Return size of data to read.
		 *
		 * @return Size of data which remains to be read.
		 */
		long getRestSize ()
		{
			return binaryReadDataSize;
		}

		/**
		 * Return remaining size of chunk, which has to be read from actual data chunk.
		 */
		long getChunkSize ()
		{
			return binaryReadChunkSize;
		}
};

/**
 * Represents data written to connection.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Rts2DataWrite
{
	private:
		// number of data to write on conection
		long binaryWriteDataSize;
	public:
		Rts2DataWrite (int in_dataSize)
		{
			binaryWriteDataSize = in_dataSize;
		}

		long getDataSize ()
		{
			return binaryWriteDataSize;
		}

		void dataWritten (int size)
		{
			binaryWriteDataSize -= size;
		}
};
#endif							 // ! __RTS2_DATA__
