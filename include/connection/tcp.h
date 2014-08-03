/* 
 * Pure TCP connection.
 * Copyright (C) 2009 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_CONNECTION_TCP__
#define __RTS2_CONNECTION_TCP__

#include "connnosend.h"
#include "error.h"

#include <ostream>

namespace rts2core
{


/**
 * Raised when connection cannot be created.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class ConnCreateError:public ConnError
{
	public:
		ConnCreateError (Connection *conn, const char *_msg, int _errn):ConnError (conn, _msg, _errn)
		{
		}
};


/**
 * Error raised when connection cannot send data.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class ConnSendError:public ConnError
{
	public:
		ConnSendError (Connection *conn, const char *_msg, int _err):ConnError (conn, _msg, _err)
		{
		}
};


/**
 * Error raised when connection cannot send data.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class ConnTimeoutError:public ConnError
{
	public:
		ConnTimeoutError (Connection *conn, const char *_msg):ConnError (conn, _msg)
		{
		}
};

/**
 * Error when receiving data from socket.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class ConnReceivingError:public ConnError
{
	public:
		ConnReceivingError (Connection *conn, const char *_msg, int _err):ConnError (conn, _msg, _err)
		{
		}
};

/**
 * Class for TCP/IP connection.
 *
 * Provides basic operations on TCP/IP connection - establish it, send data to
 * it, detect errors. This connection class is not inntended for RTS2
 * communication, but for access to external TCp/IP services.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class ConnTCP:public ConnNoSend
{
	public:
		/**
		 * Create new connection to APC UPS daemon.
		 *
		 * @param _master   Reference to master holding this connection.
		 *
		 * @param _hostname APC UPSD IP address or hostname.
		 * @param _port     Portnumber of APC UPSD daemon (default to 3551).
		 */
		ConnTCP (rts2core::Block *_master, const char *_hostname, int _port);

		/**
		 * Init TCP/IP connection to host given at constructor.
		 *
		 * @return -1 on error, 0 on success.
		 */
		virtual int init ();

		/**
		 * Set debugging flag. When debugging is enabled, all data send
		 * to and from the socket will be logged using standard RTS2
		 * logging with MESSAGE_DEBUG type.
		 */
		void setDebug (bool _debug = true) { debug = _debug; }

		/**
		 * Send data to TCP/IP socket.
		 *
		 * @param data   Data to send to the socket.
		 * @param len    Data buffer length.
		 * @param binary If true, data will be logged as hex characters.
		 *
		 * @throw ConnError on error.
		 */
		void sendData (const void *data, int len, bool binary = true);

		void sendData (const char *data);

		void sendData (std::string data);

		/**
		 * Receive data from TCP/IP socket. Pefroms select call before,
		 * wait for wtime second.
		 *
		 * @param data   Buffer of size at least len where data will be received.
		 * @param len    Length of expected data.
		 * @param wtime  Wait time in seconds.
		 * @param binary True if data should be printed as binary.
		 *
		 * @throw ConnError on errror.
		 */
		void receiveData (void *data, size_t len, int wtime, bool binary = true);

		/**
		 * Receive data from connection till character in end is encoutered.
		 *
		 * @param _is    Input string stream which will receive data.
		 * @param wtime  Wait time in seconds.
		 * @param end    End character.
		 */
		void receiveData (std::istringstream **_is, int wtime, char end_char);

		/**
		 * Send data, wait for reply.
		 *
		 * @param wbuf    buffer with data to write
		 * @param wlne    length of the write buffer
		 * @param rbuf    buffer where readed data will be stored
		 * @param rlen    length of the read buffer
		 * @param endChar end character
		 * @param wtime   connection timeout
		 *
		 * @return size of data readed on success.
		 *
		 * @throw ConnError on error.
		 */
		int writeRead (const char* wbuf, int wlen, char *rbuf, int rlen, char endChar, int wtime = 5);

		virtual void postEvent (Event * event);

	protected:
		virtual bool canDelete () { return false; }

		virtual void connectionError (int last_data_size);

		bool getDebug () { return debug; }

	private:
		std::string hostname;
		int port;

		bool debug;

		bool checkBufferForChar (std::istringstream **_is, char end_char);
};

};

#endif // !__RTS2_CONNECTION_TCP__
