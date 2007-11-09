/* 
 * Connection class.
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

#ifndef __RTS2_CONN__
#define __RTS2_CONN__

/**
 * @file Contains Rts2Conn class.
 */

#include <string>
#include <string.h>
#include <time.h>
#include <list>
#include <netinet/in.h>

#include <status.h>

#include "rts2object.h"
#include "rts2serverstate.h"
#include "rts2message.h"
#include "rts2logstream.h"
#include "rts2valuelist.h"

#define MAX_DATA    200

typedef enum conn_type_t
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
	/** Socket to device is opened, connection waits for other side to reply to connect call. */
	CONN_CONNECTING,
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

class Rts2Address;

class Rts2Block;

class Rts2Command;
class Rts2CommandStatusInfo;

class Rts2ClientTCPDataConn;

class Rts2DevClient;

class Rts2Event;

class Rts2Value;

/**
 * Represents one connection. It keeps connection running, check it states, and
 * handles various TCP/IP issues.  Connection is primary network connection,
 * but there are descendand classes which holds forked instance output.
 *
 * Rts2Conn is used primarly in @see Rts2Block, which holds list of connections
 * and provide function to manage them.
 *
 * @addgroup RTS2Block
 */
class Rts2Conn:public Rts2Object
{
	private:
		char *buf;
		size_t buf_size;
		char *buf_top;
		char *command_buf_top;

		conn_type_t type;
								 // name of device/client this connection goes to
		char name[DEVICE_NAME_SIZE];
		int key;
		int priority;			 // priority - number
		int have_priority;		 // priority - flag if we have priority
		int centrald_id;		 // id of connection on central server
		in_addr addr;
		int port;				 // local port & connection

		std::list < Rts2Command * >commandQue;
		Rts2Command *runningCommand;

		// used for monitoring of connection state..
		time_t lastGoodSend;
		time_t lastData;
		time_t lastSendReady;

		// connectionTimeout in seconds
		int connectionTimeout;
		conn_state_t conn_state;

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
		 * Holds connection values.
		 */
		Rts2ValueVector values;

		/**
		 * Time when last information was received.
		 */
		double last_info_time;

		/**
		 * Holds info_time variable.
		 */
		Rts2ValueTime *info_time;

	protected:
		/**
		 * Other side of connection state.
		 */
		Rts2ServerState * serverState;

		/**
		 * BOP mask state.
		 */
		Rts2ServerState * bopState;

		/**
		 * Pointer to master object.
		 */
		Rts2Block *master;
		char *command_start;
		int sock;

		virtual int acceptConn ();

		/**
		 * Set connection state.
		 *
		 * @param in_value New state value.
		 */
		virtual void setState (int in_value);

		/**
		 * Set device full BOP state.
		 *
		 * This is device full BOP mask, which includes BOP mask from
		 * devices blocked by this call.
		 *
		 * @param in_value New full BOP state value.
		 */
		virtual void setBopState (int in_value);

		/**
		 * Reference to other device client.
		 */
		Rts2DevClient *otherDevice;

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
		 * Function called on connection error.
		 *
		 * @param last_data_size  < 0 when real error occurs, =0 when no more data on connection, >0 when there
		 * 	were sucessfully received data, but they were either not allowed or signaled end of connection
		 *
		 */
		virtual void connectionError (int last_data_size);

	public:
		Rts2Conn (Rts2Block * in_master);
		Rts2Conn (int in_sock, Rts2Block * in_master);
		virtual ~ Rts2Conn (void);

		virtual void postEvent (Rts2Event * event);

		virtual int add (fd_set * set);

		/**
		 * Set if command is in progress.
		 *
		 * @param in_progress True if command is in progress and running.
		 */
		void setCommandInProgress (bool in_progress)
		{
			commandInProgress = in_progress;
		}

		int getState ()
		{
			return serverState->getValue ();
		}
		int getErrorState ()
		{
			return getState () & DEVICE_ERROR_MASK;
		}
		int getBopState ()
		{
			return getState () & BOP_MASK;
		}

		/**
		 * Returns full BOP state, which include connections mentioned in block_by.
		 *
		 * @return Connection full BOP state.
		 */
		int getFullBopState ()
		{
			return bopState->getValue ();
		}

		/**
		 * Get state without any error bits
		 *
		 * \return State without error bits
		 */
		int getRealState ()
		{
			return getState () & DEVICE_STATUS_MASK;
		}
		std::string getCameraChipState (int chipN);
		std::string getStateString ();
		virtual int init ()
		{
			return -1;
		}
		void postMaster (Rts2Event * event);
		virtual int idle ();

		virtual void endConnection ()
		{
			connectionError (-1);
		}

		virtual int authorizationOK ();
		virtual int authorizationFailed ();

		inline int isCommand (const char *cmd)
		{
			return !strcmp (cmd, getCommand ());
		}

		virtual int sendMsg (const char *msg);
		int sendMsg (std::string msg);
		virtual int sendMessage (Rts2Message & msg);
		int sendValue (std::string val_name, int value);
		int sendValue (std::string val_name, int val1, int val2);
		int sendValue (std::string val_name, int val1, double val2);
		int sendValue (std::string val_name, const char *value);
		int sendValueRaw (std::string val_name, const char *value);
		int sendValue (std::string val_name, double value);
		int sendValue (char *val_name, char *val1, int val2);
		int sendValue (char *val_name, int val1, int val2, double val3, double val4,
			double val5, double val6);
		int sendValueTime (std::string val_name, time_t * value);
		int sendCommandEnd (int num, char *in_msg);
		virtual int processLine ();
		virtual int receive (fd_set * set);
		conn_type_t getType ()
		{
			return type;
		};
		void setType (conn_type_t in_type)
		{
			type = in_type;
		}
		int getOurAddress (struct sockaddr_in *addr);
		void setAddress (struct in_addr *in_address);
		void setPort (int in_port)
		{
			port = in_port;
		}
		void setOtherType (int other_device_type);
		void getAddress (char *addrBuf, int buf_size);
		int getLocalPort ()
		{
			return port;
		}
		const char *getName ()
		{
			return name;
		};
		int isName (const char *in_name)
		{
			return (!strcmp (getName (), in_name));
		}
		void setName (char *in_name)
		{
			strncpy (name, in_name, DEVICE_NAME_SIZE);
			name[DEVICE_NAME_SIZE - 1] = '\0';
		}
		int getKey ()
		{
			return key;
		};
		virtual void setKey (int in_key)
		{
			if (key == 0)
				key = in_key;
		}
		int havePriority ();
		void setHavePriority (int in_have_priority);
		int getHavePriority ()
		{
			return have_priority;
		};
		int getPriority ()
		{
			return priority;
		};
		void setPriority (int in_priority)
		{
			priority = in_priority;
		};
		int getCentraldId ()
		{
			return centrald_id;
		};
		void setCentraldId (int in_centrald_id);
		int sendPriorityInfo ();

		/**
		 * Que command on connection.
		 * Commands are send over TCP/IP ordered, and next command is send only after
		 * last command was received.  The optional notBop parameter describe at
		 * which system states commands should not be send, and rather kept in que.
		 *
		 * @see Rts2Command
		 * @see EVENT_COMMAND_OK
		 * @see EVENT_COMMAND_FAILED
		 *
		 * @param cmd Command which will be send.
		 * @param notBop Block of OPeration bitfield. Put BOP bits ored in this one.
		 * @param originator Originator of the command request. If fill, it will be
		 *   sent EVENT_COMMAND_xxx messgae.
		 *
		 * @return 0 when sucessfull, -1 on error.
		 *
		 * @callergraph
		 */
		void queCommand (Rts2Command * cmd, int notBop, Rts2Object * originator =
			NULL);

		/**
		 * Que command on connection.
		 *
		 * @see Rts2Conn::queCommand(Rts2Command*,int)
		 *
		 * @param cmd Command which will be send.
		 *
		 * @return 0 when sucessfull, -1 on error.
		 */
		void queCommand (Rts2Command * cmd);

		/**
		 * Send immediatelly command to connection.
		 * This call is different from @see Rts2Conn::queCommand, that it will send
		 * command immediatly, and will not wait for end of previous command block.
		 * As command is send immeditely, BOP mask does not make any sence in such case.
		 *
		 * @param cmd Command which will be qued.
		 *
		 * @return 0 when sucessfull, -1 on error.
		 */
		void queSend (Rts2Command * cmd);

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
		virtual void commandReturn (Rts2Command * cmd, int in_status);

		/**
		 * Determines if que is empty and there is not any running command.
		 * This is usefull to check if all commands were processed by the connection.
		 *
		 * @return True if command que is empty, false if it is not empty or some command is running.
		 */
		bool queEmpty ()
		{
			return (runningCommand == NULL && commandQue.empty ());
		}

		/**
		 * Query if list of command (including running command) contains given command.
		 *
		 * @param cmd Rts2Command * we ask for
		 *
		 * @return true if we containt given command, false otherwise
		 */
		bool commandPending (Rts2Command * cmd);

		/**
		 * Clear que of pending command.
		 * This is usefull when reseting connection.
		 */
		void queClear ();

		/**
		 * Called when new device connect to the system.
		 *
		 * @param in_addr New device address.
		 */
		virtual void addressUpdated (Rts2Address * in_addr)
		{
		}

		virtual void setConnState (conn_state_t new_conn_state);

		int isConnState (conn_state_t in_conn_state);

		/**
		 * Return connection state.
		 *
		 * @return Connection state.
		 */
		conn_state_t getConnState ()
		{
			return conn_state;
		}

		int paramEnd ();
		int paramNextString (char **str);
		char *paramNextWholeString ();
		int paramNextStringNull (char **str);
		int paramNextInteger (int *num);
		int paramNextLong (long int *num);
		int paramNextSizeT (size_t * num);
		int paramNextDouble (double *num);
		int paramNextFloat (float *num);
		int paramNextTimeval (struct timeval *tv);

		// called when some data were sucessfully received
		virtual void dataReceived (Rts2ClientTCPDataConn * dataConn);

		Rts2Block *getMaster ()
		{
			return master;
		}

		virtual void childReturned (pid_t child_pid)
		{
		}

		Rts2Value *getValue (const char *value_name);

		int getOtherType ();
		// set to -1 if we don't need timeout checks..
		void setConnTimeout (int new_connTimeout)
		{
			connectionTimeout = new_connTimeout;
		}
		int getConnTimeout ()
		{
			return connectionTimeout;
		}

		Rts2ServerState *getStateObject ()
		{
			return serverState;
		}

		Rts2DevClient *getOtherDevClient ()
		{
			return otherDevice;
		}

		virtual void updateStatusWait (Rts2Conn * conn);
		virtual void masterStateChanged ();

	protected:
		virtual int command ();
		virtual void priorityChanged ();
		virtual int priorityChange ();
		int priorityInfo ();
		int sendNextCommand ();

		int commandReturn ();
		inline char *getCommand ()
		{
			return command_start;
		}
		inline int isCommandReturn ()
		{
			return (*(getCommand ()) == '+' || *(getCommand ()) == '-');
		}

	public:
		// value management functions
		void addValue (Rts2Value * value);

		int metaInfo (int rts2Type, std::string m_name, std::string desc);
		int selMetaInfo (const char *value_name, char *sel_name);

		const char *getValueChar (const char *value_name);
		double getValueDouble (const char *value_name);
		int getValueInteger (const char *value_name);
		const char *getValueSelection (const char *value_name, int val);
		const char *getValueSelection (const char *value_name);

		virtual int commandValue (const char *v_name);

		Rts2ValueVector::iterator valueBegin ()
		{
			return values.begin ();
		}
		Rts2ValueVector::iterator valueEnd ()
		{
			return values.end ();
		}
		Rts2Value *valueAt (int index)
		{
			if ((size_t) index < values.size ())
				return values[index];
			return NULL;
		}
		int valueSize ()
		{
			return values.size ();
		}

		/**
		 * Return time when values were valid.
		 *
		 * @return Time (as double seconds from 1-1-1970).
		 */
		double getInfoTime ();

		/**
		 * Check if info time has changed.
		 *
		 * @return True if info time changed from last check.
		 */
		bool infoTimeChanged ();

		/**
		 * Set last info time to 0, so when infoTimeChanged will be called, it will return true.
		 */
		void resetInfoTime ()
		{
			last_info_time = 0;
		}

		/**
		 * Returns true if we hold any value with given write type.
		 */
		bool existWriteType (int w_type);
};
#endif							 /* ! __RTS2_CONN__ */
