/* 
 * Connection class.
 * Copyright (C) 2003-2008 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_CONN__
#define __RTS2_CONN__

/**
 * @file Contains Connection class.
 */

#include <algorithm>
#include <map>
#include <string>
#include <string.h>
#include <time.h>
#include <list>
#include <netinet/in.h>

#include <status.h>

#include "error.h"
#include "data.h"
#include "object.h"
#include "serverstate.h"
#include "message.h"
#include "logstream.h"
#include "valuelist.h"

#define MAX_DATA    200

/**
 * Identifier of shared data connection.
 */
#define SHARED_DATA_CONN       -1

enum conn_type_t
{ NOT_DEFINED_SERVER, CLIENT_SERVER, DEVICE_SERVER, DEVICE_DEVICE };

/**
 * Connection states.
 */
typedef enum
{
	/** Unknow connection state. */
	CONN_UNKNOW,
	/** Connection waits for address of device. */
	CONN_RESOLVING_DEVICE,
	/** Socket is opened in listening mode, waiting for connection from clients (accept call for socket). */
	CONN_CONNECTING,
	/** Socket to device is opened, connection waits for other side to reply to connect call. */
	CONN_INPROGRESS,
	/** Connection is connected and everything works. */
	CONN_CONNECTED,
	/** Connection is broken, other side does not replied to previous commands before timeout on command reply expires. */
	CONN_BROKEN,
	/** Connection is marked for deletion and will be deleted. */
	CONN_DELETE,
	/** Connection waits for authorization, which is routed through central server. */
	CONN_AUTH_PENDING,
	/** Connection was sucessufylly authorized. */
	CONN_AUTH_OK,
	/** Connection was not authorized, it will be deleted. */
	CONN_AUTH_FAILED
} conn_state_t;


namespace rts2core
{

class NetworkAddress;

class Event;

class Connection;

class Value;

class Block;

class Command;

class CommandStatusInfo;

class DevClient;

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
 * Represents one connection. It keeps connection running, check it states, and
 * handles various TCP/IP issues.  Connection is primary network connection,
 * but there are descendand classes which holds forked instance output.
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
		Connection (int in_sock, Block * in_master);
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

		virtual void postEvent (Event * event);

		/**
		 * Add to poll which belong to the current connection. Those
		 * sets are used in main poll call of Block.
		 *
		 * @param Block	pointer to the block which required adding of the FDs
		 */
		virtual int add (Block * block);

		/**
		 * Set if command is in progress.
		 *
		 * @param in_progress True if command is in progress and running.
		 */
		void setCommandInProgress (bool in_progress) { commandInProgress = in_progress; }

		rts2_status_t getState () { return serverState->getValue (); }
		
		/**
		 * Get weather state of the connection.
		 *
		 * @return True if connection is reporting good weather, false if it is reporting bad weather.
		 */
		bool isGoodWeather () { return (getState () & WEATHER_MASK) == GOOD_WEATHER; }

		/**
		 * Return whenewer device is in idle state.
		 */
		bool isIdle () { return (getState () & DEVICE_STATUS_MASK) == DEVICE_IDLE; }

		/**
		 * Retrieved move allowed state.
		 *
		 * @return True if connection is reporting that moves are allowed.
		 */
		bool canMove () { return (getState () & STOP_MASK) == CAN_MOVE; }

		rts2_status_t getErrorState () { return getState () & DEVICE_ERROR_MASK; }
		rts2_status_t getBopState () { return getState () & BOP_MASK; }

		/**
		 * Returns full BOP state, which include connections mentioned in block_by.
		 *
		 * @return Connection full BOP state.
		 */
		rts2_status_t getFullBopState () { return bopState->getValue (); }

		/**
		 * Get state without any error bits
		 *
		 * \return State without error bits
		 */
		rts2_status_t getRealState () { return getState () & DEVICE_STATUS_MASK; }
		std::string getCameraChipState (int chipN);
		std::string getStateString ();
		virtual int init () { return -1; }
		void postMaster (Event * event);
		virtual int idle ();

		virtual void endConnection () { connectionError (-1); }

		virtual int authorizationOK ();
		virtual int authorizationFailed ();

		/**
		 * Return current command.
		 */
		inline char *getCommand () { return command_start; }

		/**
		 * Check if the command match given string.
		 *
		 * @param cmd  command to check
		 *
		 * @return 1 if the input string match the current command, 0 otherwise.
		 */
		inline int isCommand (const char *cmd) { return !strcmp (cmd, getCommand ()); }

		/**
		 * Send char message to other side.
		 *
		 * @return -1 on error, 0 on sucess
		 */
		virtual int sendMsg (const char *msg);
		int sendMsg (std::string msg);
		int sendMsg (std::ostringstream &_os);

		/**
		 * Switch connection to binary connection.
		 *
		 * @param dataType type of data
		 * @param channum  number of channels
		 * @param chansize size of channels
		 *
		 * @return -1 on error, otherwise ID of data connection.
		 */
		int startBinaryData (int dataType, int channum, size_t *chansize);

		/**
		 * Sends part of binary data.
		 *
		 * @param data_conn  ID of data connection
		 * @param chan       data channel
		 * @param data       data to send
		 * @param dataSize   size of data to send (in bytes)
		 */
		int sendBinaryData (int data_conn, int chan, char *data, size_t dataSize);

		void endBinaryData (int data_conn);

		/**
		 * Image data will be transfered in shared memory, attachable by key.
		 * Those functions are called by client. The receiving side can check in 
		 * idle loop with sharedDataUpdate call, and get informed about shared
		 * data start/end with newDataConn and fullDataReceived calls.
		 */
		int startSharedData (DataSharedWrite *data, int channum, int *segnums);

		void endSharedData (int data_conn, bool complete);

		int fitsDataTransfer (const char *fn);

		virtual int sendMessage (Message & msg);
		int sendValue (std::string val_name, int value);
		int sendValue (std::string val_name, int val1, double val2);
		int sendValue (std::string val_name, const char *value);
		int sendValueRaw (std::string val_name, const char *value);
		int sendValue (std::string val_name, double value);
		int sendValue (char *val_name, char *val1, int val2);
		int sendValue (char *val_name, int val1, int val2, double val3, double val4, double val5, double val6);
		int sendValueTime (std::string val_name, time_t * value);

		int sendProgress (double start, double end);

		/**
		 * Send end of command on the connection.
		 *
		 * @param num      Error number (0 for no error).
		 * @param in_msg   Message which will be send with command end.
		 *
		 * @return -1 on error, 0 on sucess.
		 */
		int sendCommandEnd (int num, const char *in_msg);

		virtual void processLine ();

		/**
		 * Returns true if connection is in read_set, and so can be read.
		 *
		 * @param fds	poll returned structure
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

		conn_type_t getType () { return type; };
		void setType (conn_type_t in_type) { type = in_type; }
		int getOurAddress (struct sockaddr_in *addr);
		void setAddress (struct in_addr *in_address);
		void setPort (int in_port) { port = in_port; }
		void setOtherType (int other_device_type);
		void getAddress (char *addrBuf, int _buf_size);
		int getLocalPort () { return port; }
		const char *getName () { return name.c_str (); };
		int isName (const char *in_name) { return (!strcmp (getName (), in_name)); }
		void setName (int _centrald_num, const char *in_name)
		{
			centrald_num = _centrald_num;
			name = std::string (in_name);
		}
		int getKey () { return key; };
		virtual void setKey (int in_key)
		{
			if (key == 0)
				key = in_key;
		}

		int getCentraldNum () { return centrald_num; }

		int getCentraldId () { return centrald_id; }

		void setCentraldId (int in_centrald_id);
		void setCentraldNum (int _centrald_num) { centrald_num = _centrald_num; }

		/**
		 * Que command on connection.
		 * Commands are send over TCP/IP ordered, and next command is send only after
		 * last command was received.  The optional notBop parameter describe at
		 * which system states commands should not be send, and rather kept in que.
		 *
		 * @see Command
		 * @see EVENT_COMMAND_OK
		 * @see EVENT_COMMAND_FAILED
		 *
		 * @param cmd Command which will be send.
		 * @param notBop Block of OPeration bitfield. Put BOP bits ored in this one.
		 * @param originator Originator of the command request. If fill, it will be
		 *   sent EVENT_COMMAND_xxx messgae.
		 *
		 * @callergraph
		 */
		void queCommand (Command * cmd, int notBop, Object * originator = NULL);

		/**
		 * Que command on connection.
		 *
		 * @see Connection::queCommand(Command*,int)
		 *
		 * @param cmd Command which will be send.
		 *
		 * @return 0 when sucessfull, 1 if command was queued (waiting for external state transition)
		 */
		int queCommand (Command * cmd);

		/**
		 * Send immediatelly command to connection.
		 * This call is different from @see Connection::queCommand, that it will send
		 * command immediatly, and will not wait for end of previous command block.
		 * As command is send immeditely, BOP mask does not make any sence in such case.
		 *
		 * @param cmd Command which will be qued.
		 *
		 * @return 0 when sucessfull, -1 on error.
		 */
		void queSend (Command * cmd);

		/**
		 * Hook for command return.
		 *
		 * This metthod is called when command is performed by the other
		 * side of connection and return code is returned.
		 *
		 * It is called after +000 or -<error code> message is received.
		 *
		 * @param cmd Finished command.
		 * @param in_status Command return status.
		 */
		virtual void commandReturn (Command * cmd, int in_status);

		/**
		 * Determines if que is empty and there is not any running command.
		 * This is usefull to check if all commands were processed by the connection.
		 *
		 * @return True if command que is empty, false if it is not empty or some command is running.
		 */
		bool queEmpty ()
		{
			if (runningCommandStatus != RETURNING && runningCommand)
				return false;
			return commandQue.empty ();
		}

		/**
		 * Return true, if command que is empty for given
		 * originator.
		 *
		 * @param testOriginator Test if in que is command
		 * 	with this object as originator.
		 *
		 * @return True if in connectionQue isn't any command with originator set to testOriginator.
		 */
		bool queEmptyForOriginator (Object *testOriginator);
		bool commandOriginatorPending (Object *originator) { return !queEmptyForOriginator (originator); }

		/**
		 * Clear connection from all pending commands.
		 * Will remove all pending commands from que. Will also
		 * delete runningCommand, if it was not send.
		 */
		void queClear ();

		/**
		 * Called when connection will be deleted from system to
		 * remove any possible reference to this connection.
		 *
		 * @param conn Connection which will be removed.
		 */
		virtual void deleteConnection (Connection * conn);

		/**
		 * Called when new device connect to the system.
		 *
		 * @param in_addr New device address.
		 */
		virtual void addressUpdated (NetworkAddress * in_addr) {}

		virtual void setConnState (conn_state_t new_conn_state);

		int isConnState (conn_state_t in_conn_state);

		/**
		 * Return connection state.
		 *
		 * @return Connection state.
		 */
		conn_state_t getConnState () { return conn_state; }

		bool paramEnd ();

		/**
		 * Parse next string.
		 *
		 * After this function is called, str points to start of next, \0 delimited string,
		 *
		 * @param str       Location where pointer to the begging of string will be stored.
		 * @param enddelim  Possible additional end delimiters. isspace chars are default delimiters.
		 *
		 * @return 0 on success, -1 if end of buffer was reached before extracting any usefull data.
		 *
		 */
		int paramNextString (char **str, const char *enddelim = NULL);
		char *paramNextWholeString ();
		int paramNextStringNull (char **str);
		int paramNextInteger (int *num);
		int paramNextLong (long int *num);
		int paramNextLongLong (long long int *num);
		int paramNextSizeT (size_t * num);
		int paramNextSSizeT (ssize_t * num);
		int paramNextDouble (double *num);
		/**
		 * Retrieve next parameter as time. String starting with + is
		 * considered as second offset from now, without + is absolute time.
		 */
		int paramNextDoubleTime (double *num);

		int paramNextFloat (float *num);

		/**
		 * Retrieve next parameter as (possibly) HMS value.
		 */
		int paramNextHMS (double *num);

		int paramNextDMS (double *num);

		int paramNextTimeval (struct timeval *tv);

		Block *getMaster () { return master; }

		virtual void childReturned (pid_t child_pid) {}

		/**
		 * Search for value by value name.
		 *
		 * @param value_name   name of the searched value
		 *
		 * @return  Value object of value with given name, or NULL if value with this name does not exists.
		 */
		Value *getValue (const char *value_name);

		/**
		 * Returns true if connection has given value.
		 */
		bool hasValue (Value *val) { return std::find (values.begin (), values.end (), val) != values.end (); }

		/**
		 * Returns value with given name and type. Throw error if such value does not exists.
		 *
		 * @param value_name   name of the searched value
		 * @param value_type   required type of the value
		 *
		 * @throw Error 
		 */
		Value *getValueType (const char *value_name, int value_type);

		/**
		 * Return iterator to the next failed value, starting from position
		 * specified as parameter.
		 *
		 * @param iter    starting position from which the failed value will be searched
		 */
		ValueVector::iterator & getFailedValue (ValueVector::iterator &iter);

		int getOtherType ();
		// set to -1 if we don't need timeout checks..
		void setConnTimeout (int new_connTimeout) { connectionTimeout = new_connTimeout; }
		int getConnTimeout () { return connectionTimeout; }

		ServerState *getStateObject () { return serverState; }

		DevClient *getOtherDevClient () { return otherDevice; }

		virtual void updateStatusWait (Connection * conn);
		virtual void masterStateChanged ();

		friend class ConnError;


		// value management functions
		void addValue (Value * value, const ValueVector::iterator eiter);

		int metaInfo (int rts2Type, std::string m_name, std::string desc);
		int selMetaClear (const char *value_name);
		int selMetaInfo (const char *value_name, char *sel_name);

		const char *getValueChar (const char *value_name);
		double getValueDouble (const char *value_name);
		int getValueInteger (const char *value_name);
		const char *getValueSelection (const char *value_name, int val);
		const char *getValueSelection (const char *value_name);

		virtual int commandValue (const char *v_name);

		ValueVector::iterator valueBegin () { return values.begin (); }
		ValueVector::iterator valueEnd () { return values.end (); }
		Value *valueAt (int index)
		{
			if ((size_t) index < values.size ())
				return values[index];
			return NULL;
		}
		int valueSize () { return values.size (); }

		/**
		 * Return time when values were valid.
		 *
		 * @return Time of last information call (as number of seconds from 1.1.1970).
		 */
		double getInfoTime ();

		/**
		 * Get time when values were valid.
		 *
		 * @param tv Returned timeval of last info call.
		 */
		void getInfoTime (struct timeval &tv);

		/**
		 * Check if info time has changed.
		 *
		 * @return True if info time changed from last check.
		 */
		bool infoTimeChanged ();

		/**
		 * Set last info time to 0, so when infoTimeChanged will be called, it will return true.
		 */
		void resetInfoTime () { last_info_time = 0; }

		/**
		 * Returns true if we hold any value with given write type.
		 */
		bool existWriteType (int w_type);

		/**
		 * Return size of data we have to write.
		 */
		size_t getWriteBinaryDataSize (int data_conn)
		{
			// if it exists..
			std::map <int, DataAbstractWrite *>::iterator iter = writeChannels.find (data_conn);
			if (iter == writeChannels.end ())
				return 0;
			return ((*iter).second)->getDataSize ();
		}

		/**
		 * Return size of data we have to write on given channel.
		 */
		size_t getWriteBinaryDataSize (int data_conn, int chan)
		{
			// if it exists..
			std::map <int, DataAbstractWrite *>::iterator iter = writeChannels.find (data_conn);
			if (iter == writeChannels.end ())
				return 0;
			return ((*iter).second)->getChannelSize (chan);
		}

		/**
		 * Returns operation/state progress. Returns NAN if state progress parameter
		 * is not available.
		 *
		 * @return percents (0-100) of the progress, NAN if state progress is not available
		 */
		double getProgress (double now);

		double getProgressStart () { return statusStart; }

		double getProgressEnd () { return statusExpectedEnd; }

		/**
		 * Retrieves last active data channel.
		 *
		 * @param chan  channel number. Channels are counted from 0.
		 */
		DataAbstractRead *lastDataChannel (int chan);

		/**
		 * Send value using sendValueAll?
		 */
		bool getSendAll () { return sendAll; }

		void setSendAll (bool sa) { sendAll = sa; }

	protected:
		char *buf;
		size_t buf_size;
		char *buf_top;

		char *command_buf_top;

		/**
		 * Other side of connection state.
		 */
		ServerState * serverState;

		/**
		 * BOP mask state.
		 */
		ServerState * bopState;

		/**
		 * Pointer to master object.
		 */
		Block *master;
		char *command_start;

		/**
		 * Connection file descriptor.
		 */
		int sock;

		// if we will print connection communication
		bool debugComm;

		// if debugging will be in hex only
		bool logTrafficAsHex;

		/**
		 * Check if buffer is fully filled. If that's the case, increase buffer size.
		 */
		void checkBufferSize ();

		virtual int acceptConn ();

		// if we can delete connection..
		virtual bool canDelete () { return true; };

		/**
		 * Set connection state.
		 *
		 * @param in_value New state value.
		 */
		virtual void setState (rts2_status_t in_value, char * msg);

		/**
		 * Set device full BOP state.
		 *
		 * This is device full BOP mask, which includes BOP mask from
		 * devices blocked by this call.
		 *
		 * @param in_value New full BOP state value.
		 */
		virtual void setBopState (rts2_status_t in_value);

		/**
		 * Reference to other device client.
		 */
		DevClient *otherDevice;

		/**
		 * Type of device.
		 */
		int otherType;

		/**
		 * Called when we sucessfully send some data over connection.
		 */
		void successfullSend ();

		/**
		 * Return time when some data were sucessfully sended.
		 */
		void getSuccessSend (time_t * in_t);

		/**
		 * Called when there was not activity on connection. It sends to the other side IAM_ALIVE command,
		 * and wait for reply.
		 *
		 * @return True when IAM_ALIVE should be send, otherwise false.
		 */
		bool reachedSendTimeout ();

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

		virtual int command ();
		int sendNextCommand ();

		int commandReturn ();
		inline int isCommandReturn () { return (*(getCommand ()) == '+' || *(getCommand ()) == '-'); }

	private:
		char *full_data_end;	 // points to end of full data

		conn_type_t type;

		std::string name; // name of device/client this connection goes to

		int key;
		int centrald_num;		// number of centrald connection
		int centrald_id;		// id of connection on central server
		in_addr addr;
		int port;				 // local port & connection

		bool sendAll;	// if true, sendValueAll will send the value to the connection

		std::list < Command * > commandQue;
		Command *runningCommand;
		enum {WAITING, SEND, RETURNING}
		runningCommandStatus;

		// used for monitoring of connection state..
		time_t lastGoodSend;
		time_t lastData;
		time_t lastSendReady;

		// status times
		double statusStart;
		double statusExpectedEnd;

		std::map <int, DataChannels *> readChannels;
		int activeReadData;
		int activeReadChannel;

		rts2core::DataSharedRead *sharedReadMemory;

		std::map <int, DataAbstractWrite *> writeChannels;
		// ID of outgoing data connection
		int dataConn;

		// connectionTimeout in seconds
		int connectionTimeout;
		conn_state_t conn_state;

		int statusProgress ();
		int status ();
		int bopStatus ();
		int message ();

		/**
		 * Determine, if it's save to send command over network to other side.
		 * It sends runningCommand, which must be filled before sendCommand
		 * is called.
		 *
		 * @pre runningCommand != NULL
		 */
		void sendCommand ();

		/**
		 * If the connection is sending command.
		 *
		 * @invariant True if command is pending (e.g. the connection does not send end of command), otherwise false.
		 */
		bool commandInProgress;

		/**
		 * Called to process buffer.
		 */
		void processBuffer ();

		/**
		 * Holds connection values.
		 */
		ValueVector values;

		/**
		 * Time when last information was received.
		 */
		double last_info_time;

		/**
		 * Holds info_time variable.
		 */
		ValueTime *info_time;

		/**
		 * Called when new data connection is created.
		 *
		 * @param data_conn Number of data connection created.
		 */
		void newDataConn (int data_conn);

		/**
		 * Called when some data were sucessfully received.
		 */
		void dataReceived ();
};

}

#endif							 /* ! __RTS2_CONN__ */
