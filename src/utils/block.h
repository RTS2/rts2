/* 
 * Basic RTS2 devices and clients building block.
 * Copyright (C) 2003-2007,2010 Petr Kubanek <petr@kubanek.net>
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
 * Holds base Block class. This class is common ancestor of RTS2 devices, daemons and clients.
 *
 * @defgroup RTS2Block Core RTS2 classes
 * @defgroup RTS2Protocol RTS2 protocol
 */

#include <string.h>
#include <list>
#include "status.h"

#include <sstream>
#include <sys/inotify.h>

#include "rts2event.h"
#include "rts2object.h"
#include "rts2conn.h"
#include "rts2option.h"
#include "rts2address.h"
#include "rts2user.h"
#include "rts2devclient.h"
#include "value.h"
#include "app.h"
#include "rts2serverstate.h"

// protocol specific commands
/** The command is variable value update. @ingroup RTS2Protocol */
#define PROTO_VALUE            "V"
/** The command set variable value. @ingroup RTS2Protocol */
#define PROTO_SET_VALUE        "X"
/** The command is authorization request. @ingroup RTS2Protocol */
#define PROTO_AUTH             "A"
/** The command set device status. @ingroup RTS2Protocol */
#define PROTO_STATUS           "S"
/** Command reporting state progress. */
#define PROTO_PROGRESS         "P"
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
/** Shared memory segment key, which holds data */
#define PROTO_SHARED           "H"
/** Full shared data received */
#define PROTO_SHARED_FULL      "I"


class Rts2ClientTCPDataConn;

class Rts2LogStream;

/** Hold list of connections. It is used to store @see Rts2Conn objects. */
typedef std::vector < Rts2Conn * > connections_t;

namespace rts2core
{

class Rts2Command;

class Rts2DevClient;

/**
 * Base class of RTS2 devices and clients.
 *
 * Contain RTS2 related management functions - manage list of connections,
 * and basic commands which are passed on conditions (e.g. status messages).
 *
 * @ingroup RTS2Block
 */
class Block: public rts2core::App
{
	public:

		/**
		 * Basic constructor. Fill argc and argv values.
		 *
		 * @param in_argc Number of agruments, ussually argc passed from main call.
		 * @param in_argv Block arguments, ussually passed from main call.
		 */
		Block (int in_argc, char **in_argv);

		/**
		 * Delete list of conncection, clear Block structure.
		 */
		virtual ~Block (void);

		/**
		 * Set port number of listening socket.
		 *
		 * Block ussually create listening socket, so other RTS2 programs can connect to the component.
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
		 * Add connection to block. Block select call then take into
		 * account connections file descriptor and call hooks either
		 * when data arrives or writing on connection is possible.
		 *
		 * @param conn Connection which will be added to connections of
		 * the block.
		 */
		void addConnection (Rts2Conn *_conn);

		/**
		 * Remove connection from list of connections. The programme is then
		 * responsible to call destructor for the connection. This is handy
		 * when destructor is called for some other reason.
		 */
		void removeConnection (Rts2Conn *_conn);

		/**
		 * Add connection as connection to central server,
		 *
		 * @param _conn Connection which will be added.
		 * @param added True if connection can be added directly
		 */
		void addCentraldConnection (Rts2Conn *_conn, bool added);

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
		 * If command is running (e.g. was send to the conection, but Block does
		 * not received reply), it will return True.
		 *
		 * @return True if command que is empty and new command will be executed
		 * immediately (after running command returns), otherwise returns false.
		 */
		bool commandQueEmpty ();

		/**
		 * Event handling mechanism.
		 *
		 * Send Event to all connections which are members of Block structure.
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
		void sendStatusMessage (int state, const char * msg = NULL);

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
		void sendBopMessage (int state, int bop_state);

		/**
		 * Send BOP message to a single connection.
		 *
		 * @param bop_state
		 * @param conn Connection which will receive BOP state.
		 *
		 * @callergraph
		 *
		 * @see PROTO_BOP_STATE
		 */
		void sendBopMessage (int state, int bop_state, Rts2Conn *conn);

		/**
		 * Send message to all connections.
		 *
		 * @param msg Message which will be send.
		 *
		 * @return -1 on error, otherwise 0.
		 */
		int sendAll (const char *msg);

		/**
		 * Send message to all connections.
		 *
		 * @param _os Output stream holding the message.
		 *
		 * @return -1 on error, otherwise 0.
		 */
		int sendAll (std::ostringstream &_os)
		{
			return sendAll (_os.str ().c_str ());
		}

		/**
		 * Send variable value to all connections.
		 *
		 * @param val_name Name of the variable.
		 * @param value Variable value.
		 */
		void sendValueAll (char *val_name, char *value);

		// only used in centrald!
		void sendMessageAll (Rts2Message & msg);

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

		/**
		 * This function is called when device on given connection is ready
		 * to accept commands.
		 *
		 * @param conn connection representing device which became ready
		 */
		virtual void deviceReady (Rts2Conn * conn);

		/**
		 * Called when some device connected to us become idle.
		 *
		 * @param conn connection representing device which became idle
		 */
		virtual void deviceIdle (Rts2Conn * conn);

		/**
		 * Called when master state is changed.
		 *
		 * @param old_state  old system state
		 * @param new_state  new system state (as commanded by system state change).
		 */
		virtual void changeMasterState (int old_state, int new_state);

		/**
		 * Called when new state information arrives.
		 */
		virtual int setMasterState (Rts2Conn *_conn, int new_state);

		/**
		 * Returns master state. This does not returns master BOP mask or weather state. Usually you
		 * will need this call to check if master is in day etc..
		 *
		 * @see Block::getMasterStateFull()
		 *
		 * @return masterState & (SERVERD_STATUS_MASK | SERVERD_STANDBY_MASK)
		 */
		const int getMasterState ()
		{
			return masterState & (SERVERD_STATUS_MASK | SERVERD_STANDBY_MASK);
		}

		/**
		 * Returns full master state, including BOP mask and weather. For checking server state. see Block::getMasterState()
		 *
		 * @return Master state.
		 */
		const int getMasterStateFull ()
		{
			return masterState;
		}

		std::string getMasterStateString ();

		/**
		 * Returns true if all masters thinks it is safe weather to
		 * operate observatory.  Masters can have multiple weather
		 * sensors connected which can block weather.
		 * 
		 * Master is responsible for handling those devices bad weather
		 * state and set its state accordingly. It can also hold a list 
		 * of devices which are necessary for observatory operation, and
		 * if any of this devices is missing, it will set weather to bad
		 * signal.
		 *
		 * Also all central daemons which were specified on command
		 * line using the --server argument must be running.
		 */
		virtual bool isGoodWeather ();

		/**
		 * Returns true if system can physicali move devices connected to the system.
		 *
		 * Similarly to weather state, the system has move state. If
		 * moves are inhibited at the software, system shall stop any move\
		 * and ingnore any further move commands.
		 */
		virtual bool canMove ();

		/**
		 * Returns true if all connections to central servers are up and running.
		 *
		 * @return True if all connections to central servers are up and running.
		 */
		bool allCentraldRunning ();

		/**
		 * Returns true if at least one connection to centrald is up and running. 
		 * When that is the case, it is possible to log through RTS2 logging.
		 * Otherwise, logging shall be diverted to syslog.
		 *
		 * @return True if at least one centrald is running.
		 */
		bool someCentraldRunning ();

		Rts2Address *findAddress (const char *blockName);
		Rts2Address *findAddress (int centraldNum, const char *blockName);

		void addAddress (int p_host_num, int p_centrald_num, int p_centrald_id, const char *p_name, const char *p_host, int p_port, int p_device_type);

		void deleteAddress (int p_centrald_num, const char *p_name);

		virtual Rts2DevClient *createOtherType (Rts2Conn * conn, int other_device_type);
		void addUser (int p_centraldId, const char *p_login);
		int addUser (Rts2ConnUser * in_user);

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
		 * Return next established connection to device of given type.
		 *
		 * Type represents device class. For list of types, please look to RTS2_DEVICE_xxx defines in rts2/include/status.h.
		 *
		 * @param  deviceType Type of device to search for.
		 * @param  current    Current iterator. If called to find all devices of given type, pass this along to keep references.On return.
		 *     this iterrator holds reference to next device of given type. If such device cannot be found, it is equal to getConnection ()->end ().
		 */
		void getOpenConnectionType (int deviceType, connections_t::iterator &current);

		/**
		 * Queue command for all connections with a given type.
		 *
		 * @see Rts2Command
		 *
		 * @param deviceType numeric device type
		 * @param command    command (subclass of Rts2Command)
		 */
		template <typename T> void queueCommandForType (int deviceType, T &command)
		{
			connections_t::iterator iter = connections.begin ();
			for (getOpenConnectionType (deviceType, iter); iter != connections.end (); getOpenConnectionType (deviceType, iter))
			{
				(*iter)->queCommand (new T (command));
				iter++;
			}
		}

		/**
		 * Return connection with given type. Return first connection with given type, if no connection can be found,
		 * return NULL.
		 *
		 * @param device_type  Type of device (see DEVICE_XXX constants).
		 *
		 * @return Rts2Conn pointer to opened device connection with given type.
		 */
		Rts2Conn *getOpenConnection (int device_type);

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

		/**
		 * Return centrald id of device at given centrald
		 * num server.
		 *
		 * @param centrald_num  Number at centrald.
		 * 
		 * @return -1 on error, otherwise centrald id of
		 * device which holds this connection on centrald
		 * server with number centrald_num.
		 */
		int getCentraldIdAtNum (int centrald_num);

		/**
		 * Return vector of active connections to devices and clients.
		 *
		 */
		connections_t* getConnections ()
		{
			return &connections;
		}

		/**
		 * Returns vector of connections to central server.
		 *
		 * @return Vector of connections.
		 */
		connections_t* getCentraldConns ()
		{
			return &centraldConns;
		}

		Rts2Conn *getSingleCentralConn ()
		{
			if (centraldConns.size () != 1)
			{
				std::cerr << "getSingleCentralConn does not have 1 conn: " << centraldConns.size () << std::endl;
				exit (0);
			}
			return *(centraldConns.begin ());
		}


		/**
		 * Called when new message is received.
		 *
		 * @param msg Message which is received.
		 */
		virtual void message (Rts2Message & msg);

		/**
		 * Clear all connections from pending commands.
		 */
		void clearAll ();

		int queAll (Rts2Command * cmd);
		int queAll (const char *text);

		void queAllCentralds (const char *command);

		/**
		 * Return connection with minimum (integer) value.
		 */
		Rts2Conn *getMinConn (const char *valueName);

		/**
		 * Called when connection to centrald is established and running.
		 *
		 * @param conn centrald connection
		 */
		virtual void centraldConnRunning (Rts2Conn *conn)
		{
		}

		/**
		 * Called when connection with centrald breaks. Usefull for hooking up
		 * functions defining what should be done in this case - e.g.
		 * shut down the roof,..
		 *
		 * @param conn broken centrald connection
		 */
		virtual void centraldConnBroken (Rts2Conn *conn)
		{
		}

		virtual int setValue (Rts2Conn * conn)
		{
			return -2;
		}

		/**
		 * Search for value from connection to a given device. This
		 * method is overwritten in Device, so the object can
		 * return own values.
		 *
		 * @param device_name Name of the device.
		 * @param value_name  Name of the value.
		 *
		 * @return Pointer to Value object holding the value, or NULL if device or value with given name
		 * does not exists.
		 */
		virtual Value *getValue (const char *device_name, const char *value_name);

		virtual void endRunLoop ()
		{
			setEndLoop (true);
		}

		virtual int statusInfo (Rts2Conn * conn);

		virtual int progress (Rts2Conn *conn, double start, double end) { return -1; }

		/**
		 * Check if command was not replied.
		 *
		 * @param object Object which orignated command.
		 * @param exclude_conn Connection which should be excluded from check.
		 *
		 * @return True if command was not send or command reply was not received, false otherwise.
		 *
		 * @callergraph
		 */
		bool commandOriginatorPending (Rts2Object * object, Rts2Conn * exclude_conn);

		/**
		 * Add new user timer.
		 *
		 * @param timer_time  Timer time in seconds, counted from now.
		 * @param event       Event which will be posted for triger. Event argument
		 *
		 * @see Rts2Event
		 */
		void addTimer (double timer_time, Rts2Event *event)
		{
			timers[getNow () + timer_time] = event;
		}

		/**
		 * Remove timer with a given type from the list of timers.
		 *
		 * @param event_type Type of event.
		 */
		void deleteTimers (int event_type);

		/**
		 * Updates metainformation about given value.
		 *
		 * @param value Value whose metainformation will be send out.
		 */
		void updateMetaInformations (Value *value);

		/**
		 * Return vector of failed values.
		 */
		std::map <Rts2Conn *, std::vector <Value *> > failedValues ();

		/**
		 * Called when modified file entry is read from inotify file descriptor.
		 */
		 virtual void fileModified (struct inotify_event *event) {};

	protected:

		virtual Rts2Conn *createClientConnection (Rts2Address * in_addr) = 0;

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
		 */
		virtual void selectSuccess ();

		/**
		 * Set which messages will be accepted by connection.
		 *
		 * @see Rts2Centrald
		 */
		void setMessageMask (int new_mask);

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

		/**
		 * Called before connection is deleted from connection list.
		 * This hook method can cause connection to not be deleted by returning
		 * non-zero value.
		 *
		 * @param conn This connection is marked for deletion.
		 * @return 0 when connection can be deleted, non-zero when some error is
		 *    detected and connection should be keeped in list of active connections.
		 *
		 * @post conn is removed from the list, @see Block::connectionRemoved is
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

		/**
		 * Set good state master connection.
		 *
		 * This connection is the only centrald connection, which is
		 * allowed to set good weather state. If good weather / ready
		 * state arrives on other connection, it will be ignored.
		 *
		 * @param _conn New master state connection.
		 */
		void setMasterConn (Rts2Conn *_conn) { stateMasterConn = _conn; }

		/**
		 * Check if some of the central connections is in given server
		 * state. This routine is used to check if some of the servers
		 * is in HARD_OFF state.
		 */
		bool centralServerInState (int state);

		/**
		 * Make sure that the value is reporting all good - e.g. is inside limits.
		 *
		 * @param value value which error flags will be possibly cleared
		 */
		void valueGood (Value *val) { valueMaskError (val, RTS2_VALUE_GOOD); }

		/**
		 * Display warning on value.
		 */
		void valueWarning (Value *val) { valueMaskError (val, RTS2_VALUE_WARNING); }

		void valueError (Value *val) { valueMaskError (val, RTS2_VALUE_ERROR); }

	private:
		int port;
		long int idle_timeout;	 // in nsec

		// timers - time when they should be executed, event which should be triggered
		std::map <double, Rts2Event*> timers;

		connections_t connections;
		
		// vector which holds connections which were recently added - idle loop will move them to connections
		connections_t connections_added;

		connections_t centraldConns;

		// vector which holds connections which were recently added - idle loop will move them to connections
		connections_t centraldConns_added;

		std::list <Rts2Address *> blockAddress;
		std::list <Rts2ConnUser * > blockUsers;

		int masterState;
		Rts2Conn *stateMasterConn;

		/**
		 * Set value error mask.
		 *
		 * @param err error bits to set
		 */
		void valueMaskError (Value *val, int32_t err);
};

}

/**
 * Determine if name of the device is representing centrald. Centrald device
 * can be named either '..' or 'centrald'.
 *
 * @return true if name matches centrald.
 */
bool isCentraldName (const char *_name);

#endif							 // !__RTS2_NETBLOCK__
