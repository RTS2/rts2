/* 
 * Basic RTS2 devices and clients building block.
 * Copyright (C) 2003-2007 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_BLOCK__
#define __RTS2_BLOCK__

/**
 * @file
 * Holds base Rts2Block class. This class is common ancestor of RTS2 devices, daemons and clients.
 *
 * @defgroup RTS2Block Core RTS2 classes
 * @defgroup RTS2Protocol RTS2 protocol
 */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <vector>
#include <list>
#include "status.h"

#include <sstream>

#include "rts2event.h"
#include "rts2object.h"
#include "rts2conn.h"
#include "rts2option.h"
#include "rts2address.h"
#include "rts2user.h"
#include "rts2devclient.h"
#include "rts2value.h"
#include "rts2valuestat.h"
#include "rts2valueminmax.h"
#include "rts2valuerectangle.h"
#include "rts2app.h"
#include "rts2serverstate.h"

// protocol specific commands
/** The command is variable value update. @ingroup RTS2Protocol */
#define PROTO_VALUE            "V"
/** The command set variable value. @ingroup RTS2Protocol */
#define PROTO_SET_VALUE        "X"
/** The command set default value of variable. @ingroup RTS2Protocol */
#define PROTO_SET_VALUE_DEF    "Y"
/** The command is authorization request. @ingroup RTS2Protocol */
#define PROTO_AUTH             "A"
/** The command is priority request. @ingroup RTS2Protocol */
#define PROTO_PRIORITY         "P"
/** The command is priority infor request. @ingroup RTS2Protocol */
#define PROTO_PRIORITY_INFO    "Q"
/** The command set device status. @ingroup RTS2Protocol */
#define PROTO_STATUS           "S"
/** The command set device BOP state. @ingroup RTS2Protocol */
#define PROTO_BOP_STATE        "B"
/** The command is technical command, used to check device responsiveness. @ingroup RTS2Protocol */
#define PROTO_TECHNICAL        "T"
/** The command is message with some message information. @ingroup RTS2Protocol */
#define PROTO_MESSAGE          "M"
/** The command is metainformation describing variable. @ingroup RTS2Protocol */
#define PROTO_METAINFO         "E"
/** The command is metainformation about selection variable. @ingroup RTS2Protocol */
#define PROTO_SELMETAINFO      "F"

/** The command defines binary channel. @ingroup RTS2Protocol */
#define PROTO_BINARY           "C"
/** The command is followed by data which goes to binary channel. @ingroup RTS2Protocol */
#define PROTO_DATA             "D"

#define USEC_SEC    1000000

class Rts2Command;

class Rts2ClientTCPDataConn;

class Rts2DevClient;

class Rts2LogStream;

/** Hold list of connections. It is used to store @see Rts2Conn objects. */
typedef std::list < Rts2Conn * > connections_t;

/**
 * Base class of RTS2 devices and clients.
 *
 * Contain RTS2 related management functions - manage list of connections,
 * and basic commands which are passed on conditions (e.g. status messages).
 *
 * @ingroup RTS2Block
 */
class Rts2Block: public Rts2App
{
	private:
		int port;
		long int idle_timeout;	 // in msec
		int priority_client;

		connections_t connections;

		std::list <Rts2Address *> blockAddress;
		std::list <Rts2User * > blockUsers;

		int masterState;

	protected:

		virtual Rts2Conn *createClientConnection (char *in_deviceName) = 0;
		virtual Rts2Conn *createClientConnection (Rts2Address * in_addr) = 0;

		virtual void cancelPriorityOperations ();

		virtual void childReturned (pid_t child_pid);

		/**
		 * Determine if the device wants to connect to recently added device; returns 0 if we won't connect, 1 if we will connect
		 */
		virtual int willConnect (Rts2Address * in_addr);

		/***
		 * Address list related functions.
		 **/
		virtual int addAddress (Rts2Address * in_addr);

		/**
		 * Socket set containing descriptors which can be read.
		 */
		fd_set read_set;

		/**
		 * Socket set containing descriptors which can be write.
		 */
		fd_set write_set;

		/**
		 * Socket set containing descriptors which can produce exception.
		 */
		fd_set exp_set;

		/**
		 * Enable application to add arbitary sockets.
		 *
		 * This hook is usefull for various applications that gets input from other then connection sockets,
		 * and for which creating extra Rts2Conn instance will be too heavy solution.
		 */
		virtual void addSelectSocks ();

		/**
		 * Called when select call suceed.
		 *
		 * This method is called when select call on registered sockects succeed.
		 *
		 * @param read_set fd_set structure holding selected sockets. Inside this function, developer can use
		 *   FD_ISSET to query if socket of his/her interests is inside modified socket set.
		 */
		virtual void selectSuccess ();

		/**
		 * Set which messages will be accepted by connection.
		 *
		 * @see Rts2Centrald
		 */
		void setMessageMask (int new_mask);

		/**
		 * Called before connection is deleted from connection list.
		 * This hook method can cause connection to not be deleted by returning
		 * non-zero value.
		 *
		 * @param conn This connection is marked for deletion.
		 * @return 0 when connection can be deleted, non-zero when some error is
		 *    detected and connection should be keeped in list of active connections.
		 *
		 * @post conn is removed from the list, @see Rts2Block::connectionRemoved is
		 * called, and conn is deleted.
		 */
		virtual int deleteConnection (Rts2Conn * conn);

		/**
		 * Called when connection is removed from connection list, but before connection object is deleted.
		 *
		 * @param conn Connection which is removed from connection list, and will be deleted after this command returns.
		 *
		 * @pre conn is removed from connection list.
		 * @post conn instance is deleted.
		 */
		virtual void connectionRemoved (Rts2Conn * conn);

		/**
		 * Called when BOP state is changed.
		 */
		void bopStateChanged ();

	public:

		/**
		 * Basic constructor. Fill argc and argv values.
		 *
		 * @param in_argc Number of agruments, ussually argc passed from main call.
		 * @param in_argv Block arguments, ussually passed from main call.
		 */
		Rts2Block (int in_argc, char **in_argv);

		/**
		 * Delete list of conncection, clear Rts2Block structure.
		 */
		virtual ~Rts2Block (void);

		/**
		 * Set port number of listening socket.
		 *
		 * Rts2Block ussually create listening socket, so other RTS2 programs can connect to the component.
		 *
		 * @param in_port Port number. Usually RTS2 blocks will use ports above 1020.
		 */
		void setPort (int in_port);

		/**
		 * Return listening port number.
		 *
		 * @return Listening port number, -1 when listening port is not opened.
		 */
		int getPort (void);

		/**
		 * Add connection to given block.
		 *
		 * @param conn Connection which will be added to connections of the block.
		 */
		void addConnection (Rts2Conn * conn);

		/**
		 * Returns begin iterator of connections structure.
		 *
		 * @return connections.begin() iterator.
		 */
		connections_t::iterator connectionBegin ()
		{
			return connections.begin ();
		}

		/**
		 * Returns end iterator of connections structure.
		 *
		 * @see Rts2Block::connectionBegin
		 *
		 * @return connections.end() iterator.
		 */
		connections_t::iterator connectionEnd ()
		{
			return connections.end ();
		}

		/**
		 * Return connection at given number.
		 *
		 * @param i Number of connection which will be returned.
		 *
		 * @return NULL if connection with given number does not exists, or @see Rts2Conn reference if it does.
		 *
		 * @bug since connections_t is list, [] operator cannot be used. vector caused some funny problems.
		 */
		Rts2Conn *connectionAt (int i)
		{
			int j;
			connections_t::iterator iter;
			for (j = 0, iter = connections.begin ();
				j < i && iter != connections.end (); j++, iter++);
			if (iter == connections.end ())
				return NULL;
			return *iter;
		}

		/**
		 * Return number of connections in connections structure.
		 *
		 * @return Number of connections in block.
		 */
		int connectionSize ()
		{
			return connections.size ();
		}

		/**
		 * Ask if command que is empty.
		 *
		 * If command is running (e.g. was send to the conection, but Rts2Block does
		 * not received reply), it will return True.
		 *
		 * @return True if command que is empty and new command will be executed
		 * immediately (after running command returns), otherwise returns false.
		 */
		bool commandQueEmpty ();

		/**
		 * Event handling mechanism.
		 *
		 * Send Event to all connections which are members of Rts2Block structure.
		 *
		 * @see Rts2Event
		 * @see Rts2Object::postEvent
		 *
		 * @param event Event which is passed to postEvent method.
		 */
		virtual void postEvent (Rts2Event * event);

		/**
		 * Create new connection.
		 * This function is used in descenadants to override class of connections being created.
		 *
		 * @param in_sock Socket file descriptor which holds connection.
		 *
		 * @return Rts2Conn or descenand object.
		 */
		virtual Rts2Conn *createConnection (int in_sock);

		/**
		 * Finds connection with given name.
		 *
		 * @param in_name Name of connection which will be looked for.
		 *
		 * @return NULL if connection cannot be found, otherwise reference to connection object.
		 */
		Rts2Conn *findName (const char *in_name);

		Rts2Conn *findCentralId (int in_id);

		/**
		 * Send status message to all connected clients.
		 *
		 * @param state State value which will be send.
		 *
		 * @callergraph
		 *
		 * @see PROTO_STATUS
		 */
		void sendStatusMessage (int state);

		/**
		 * Send status message to one connection.
		 *
		 * @param state State value which will be send.
		 * @param conn Connection to which the state will be send.
		 *
		 * @callergraph
		 *
		 * @see PROTO_STATUS
		 */
		void sendStatusMessage (int state, Rts2Conn *conn);

		/**
		 * Send BOP state to all connections.
		 *
		 * @param bop_state New BOP state.
		 *
		 * @callergraph
		 *
		 * @see PROTO_BOP_STATE
		 */
		void sendBopMessage (int bop_state);

		/**
		 * Send BOP message to single connection.
		 *
		 * @param bop_state
		 * @param conn Connection which will receive BOP state.
		 *
		 * @callergraph
		 *
		 * @see PROTO_BOP_STATE
		 */
		void sendBopMessage (int bop_state, Rts2Conn *conn);

		/**
		 * Send message to all connections.
		 *
		 * @param msg Message which will be send.
		 *
		 * @return -1 on error, otherwise 0.
		 */
		int sendAll (char *msg);

		/**
		 * Send variable value to all connections.
		 *
		 * @param val_name Name of the variable.
		 * @param value Variable value.
		 */
		void sendValueAll (char *val_name, char *value);

		// only used in centrald!
		void sendMessageAll (Rts2Message & msg);

		/**
		 * Called when block does not have anything to do. This is
		 * right place to put in various hooks, which will react to
		 * timer handlers or do any other maintanence work. Should be
		 * fast and quick, longer IO operations should be split to
		 * reduce time spend in idle call.
		 *
		 * When idle call is called, block does not react to any
		 * incoming requests.
		 */
		virtual int idle ();

		void setTimeout (long int new_timeout)
		{
			idle_timeout = new_timeout;
		}

		void setTimeoutMin (long int new_timeout)
		{
			if (new_timeout < idle_timeout)
				idle_timeout = new_timeout;
		}
		void oneRunLoop ();

		int setPriorityClient (int in_priority_client, int timeout);
		void checkPriority (Rts2Conn * conn)
		{
			if (conn->getCentraldId () == priority_client)
			{
				conn->setHavePriority (1);
			}
		}

		/**
		 * This function is called when device on given connection is ready
		 * to accept commands.
		 *
		 * \param conn connection representing device which became ready
		 */
		virtual void deviceReady (Rts2Conn * conn);

		/**
		 * Called when connection receive/lost priority.
		 * This method is hook for descendand to hook actions performed
		 * when device receive priority.
		 *
		 * @param conn Connection which reports priority status.
		 * @param have If connection have (true) or just lost (false) priority.
		 */
		virtual void priorityChanged (Rts2Conn * conn, bool have);

		/**
		 * Called when some device connected to us become idle.
		 *
		 * @param conn connection representing device which became idle
		 */
		virtual void deviceIdle (Rts2Conn * conn);

		virtual int changeMasterState (int new_state);

		/**
		 * Called when new state information arrives.
		 */
		virtual int setMasterState (int new_state);

		/**
		 * Returns master state. This does not returns master BOP mask. Usually you
		 * will need this call to check if master is in day etc..
		 *
		 * @see Rts2Block::getMasterStateFull()
		 *
		 * @return masterState & (SERVERD_STATUS_MASK | SERVERD_STANDBY_MASK)
		 */
		const int getMasterState ()
		{
			return masterState & (SERVERD_STATUS_MASK | SERVERD_STANDBY_MASK);
		}

		/**
		 * Returns full master state, including BOP mask. For checking server state. see Rts2Block::getMasterState()
		 *
		 * @return Master state.
		 */
		const int getMasterStateFull ()
		{
			return masterState;
		}

		Rts2Address *findAddress (const char *blockName);

		void addAddress (const char *p_name, const char *p_host, int p_port, int p_device_type);

		void deleteAddress (const char *p_name);

		virtual Rts2DevClient *createOtherType (Rts2Conn * conn, int other_device_type);
		void addUser (int p_centraldId, int p_priority, char p_priority_have, const char *p_login);
		int addUser (Rts2User * in_user);

		/**
		 * Return established connection to device with given name.
		 *
		 * Returns connection to device with deviceName. Device must be know to
		 * system and connection to it must be opened.
		 *
		 * @param deviceName Device which will be looked on.
		 *
		 * @return Rts2Conn pointer to opened device connection.
		 */
		Rts2Conn *getOpenConnection (const char *deviceName);

		/**
		 * Return connection to given device.
		 *
		 * Create and return new connection if if device name isn't found
		 * among connections, but is in address list.
		 *
		 * This function can return 'fake' client connection, which will not resolve
		 * to device name (even after 'info' call on master device).  For every
		 * command enqued to fake devices error handler will be runned.
		 *
		 * @param deviceName Device which will be looked on.
		 *
		 * @return Rts2Conn pointer to device connection.
		 *
		 * @callgraph
		 */
		Rts2Conn *getConnection (char *deviceName);

		virtual Rts2Conn *getCentraldConn ()
		{
			return NULL;
		}

		virtual void message (Rts2Message & msg);

		/**
		 * Clear all connections from pending commands.
		 */
		void clearAll ();

		int queAll (Rts2Command * cmd);
		int queAll (char *text);

		/**
		 * Return connection with minimum (integer) value.
		 */
		Rts2Conn *getMinConn (const char *valueName);

		virtual void centraldConnRunning ()
		{
		}

		virtual void centraldConnBroken ()
		{
		}

		virtual int setValue (Rts2Conn * conn, bool overwriteSaved)
		{
			return -2;
		}

		Rts2Value *getValue (const char *device_name, const char *value_name);

		virtual void endRunLoop ()
		{
			setEndLoop (true);
		}

		double getNow ()
		{
			struct timeval infot;
			gettimeofday (&infot, NULL);
			return infot.tv_sec + (double) infot.tv_usec / USEC_SEC;
		}

		virtual int statusInfo (Rts2Conn * conn);

		/**
		 * Check if command was not replied.
		 *
		 * @param cmd Command which will be checked.
		 * @param exclude_conn Connection which should be excluded from check.
		 *
		 * @return True if command was not send or command reply was not received, false otherwise.
		 *
		 * @callergraph
		 */
		bool commandPending (Rts2Command * cmd, Rts2Conn * exclude_conn);

		bool commandOriginatorPending (Rts2Object * object, Rts2Conn * exclude_conn);

		/**
		 * Called when we have new binary data on connection. Childs
		 * should overwite this method.
		 *
		 * @param conn Connection which received binary data
		 */
		virtual void binaryDataArrived (Rts2Conn *conn)
		{
		}
};
#endif							 // !__RTS2_NETBLOCK__
