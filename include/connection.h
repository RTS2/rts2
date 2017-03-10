/* 
 * Basic Connection class.
 * Copyright (C) 2017 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_CONNECTION__
#define __RTS2_CONNECTION__

/**
 * @file Contains Connection class.
 */

#include <string>
#include <string.h>
#include <time.h>
#include <list>

#include <status.h>

#include "error.h"
#include "data.h"
#include "object.h"

namespace rts2core
{

class Event;

class Connection;

class Value;

class Block;

/**
 * Superclass for any connection errors. All errors which occurs on connection
 * inherit from this class.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class ConnError: public Error
{
	public:
		ConnError (Connection *conn, const char *_msg);
		ConnError (Connection *conn, const char *_msg, int _errn);
};

/**
 * Represents one connection. It keeps connection running. It is a generic
 * entry point for any character device.
 *
 * Connection is used primarly in @see Block, which holds list of connections
 * and provide function to manage them.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 *
 * @ingroup RTS2Block
 */
class Connection:public Object
{
	public:
		Connection (Block * in_master);
		virtual ~ Connection (void);

		/**
		 * Set if debug messages from port communication will be printed.
		 *
		 * @param printDebug  True if all port communication should be written to log.
		 */
		void setDebug (bool printDebug = true) { debugComm = printDebug; }
		
		/**
		 * Log all trafix as hex.
		 *
		 * @param logArrAsHex If true, all traffic will be logged in hex values.
		 */
		void setLogAsHex (bool logArrAsHex = true) { logTrafficAsHex = logArrAsHex; }

		/**
		 * Add to poll which belong to the current connection. Those
		 * sets are used in main poll call of Block.
		 *
		 * @param Block	pointer to the block which required adding of the FDs
		 */
		virtual int add (Block * block);

		virtual int init () { return -1; }
		void postMaster (Event * event);
		virtual int idle ();

		virtual void endConnection () { connectionError (-1); }

		virtual void processLine ();

		/**
		 * Returns true if connection is in read_set, and so can be read.
		 *
		 * @param block master block
		 */
		bool receivedData (Block *block);

		/**
		 * Called when select call indicates that socket holds new
		 * data for reading.
		 *
		 * @param fds	poll returned structure
		 *
		 * @return -1 on error, 0 on success.
		 */
		virtual int receive (Block *block);

		/**
		 * Called when select call indicates that socket 
		 * can accept new data for writing.
		 *
		 * @param write  Write FD_SET, connection must test if socket is among this set.
		 * @return -1 on error, 0 on success.
		 */
		virtual int writable (Block *block);

		/**
		 * Called when connection will be deleted from system to
		 * remove any possible reference to this connection.
		 *
		 * @param conn Connection which will be removed.
		 */
		virtual void deleteConnection (Connection * conn);

		Block *getMaster () { return master; }

		virtual void childReturned (pid_t child_pid) {}

		friend class ConnError;

	protected:
		char *buf;
		size_t buf_size;
		char *buf_top;

		/**
		 * Pointer to master object.
		 */
		Block *master;

		// if we will print connection communication
		bool debugComm;

		// if debugging will be in hex only
		bool logTrafficAsHex;

		/**
		 * Check if buffer is fully filled. If that's the case, increase buffer size.
		 */
		void checkBufferSize ();

		// if we can delete connection..
		virtual bool canDelete () { return true; };

		/**
		 * Called when we sucessfully send some data over the connection.
		 */
		void successfullSend ();

		/**
		 * Return time when some data were sucessfully sended.
		 */
		void getSuccessSend (time_t * in_t);

		/**
		 * Called when some data are readed from socket. It updates connection timeout, so connection keeping
		 * packets will be send only when needed.
		 */
		void successfullRead ();

		/**
		 * Called when socket which is waiting for finishing of connectect call is connected.
		 */
		virtual void connConnected ();

		/**
		 * Function called on connection error.
		 *
		 * @param last_data_size  < 0 when real error occurs, =0 when no more data on connection, >0 when there
		 * 	were sucessfully received data, but they were either not allowed or signaled end of connection
		 *
		 */
		virtual void connectionError (int last_data_size);

		// used for monitoring of connection state..
		time_t lastGoodSend;
		time_t lastData;
		time_t lastSendReady;
};

}

#endif // !__RTS2_CONNECTION__
