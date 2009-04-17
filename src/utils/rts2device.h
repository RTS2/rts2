/* 
 * Device basic class.
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

#ifndef __RTS2_DEVICE__
#define __RTS2_DEVICE__

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 255
#endif

#include "hoststring.h"
#include "rts2command.h"
#include "rts2daemon.h"
#include "rts2configraw.h"

#define CHECK_PRIORITY if (!conn->havePriority ()) { conn->sendCommandEnd (DEVDEM_E_PRIORITY, "haven't priority"); return -1; }

class Rts2Device;

/**
 * Device connection.
 *
 * Handles both connections which are created from clients to device, as well
 * as connections created from device to device. They are distinguished by
 * connType (set by setType, get by getType calls).
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Rts2DevConn:public Rts2Conn
{
	private:
		// in case we know address of other side..
		Rts2Address * address;

		Rts2Device *master;

	protected:
		virtual int command ();
		virtual int init ();

		virtual void connConnected ();
	public:
		Rts2DevConn (int in_sock, Rts2Device * in_master);

		virtual int authorizationOK ();
		virtual int authorizationFailed ();
		void setHavePriority (int in_have_priority);

		virtual void setDeviceAddress (Rts2Address * in_addr);
		void setDeviceName (int _centrald_num, char *_name);

		void setDeviceKey (int _centraldId, int _key);
		virtual void setConnState (conn_state_t new_conn_state);
};

/**
 * Device connection to master - Rts2Centrald.
 *
 * @see Rts2Centrald
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Rts2DevConnMaster:public Rts2Conn
{
	private:
		char *device_host;
		char master_host[HOST_NAME_MAX];
		int master_port;
		int centrald_num;
		char device_name[DEVICE_NAME_SIZE];
		int device_type;
		int device_port;
		time_t nextTime;
	protected:
		virtual int command ();
		virtual int priorityChange ();
		virtual void setState (int in_value);
		virtual void setBopState (int in_value);
		virtual void connConnected ();
		virtual void connectionError (int last_data_size);
	public:
		/**
		 * Construct new connection to central server.
		 *
		 * @param _master        Rts2Block which commands connection.
		 * @param _device_host   Hostname of computer with device.
		 * @param _device_port   Listening port for incoming connections
		 * @param _device_name   Name of the device.
		 * @param _device_type   Device type.
		 * @param _master_host   Central server hostname for the connectio.
		 * @param _master_port   Central server port for the connectio.
		 * @param _serverNum     Server number (number of centrald which device is connected to)
		 */
		Rts2DevConnMaster (Rts2Block * _master, char *_device_host, int _device_port,
			const char *_device_name, int _device_type,
			const char *_master_host, int _master_port, int _serverNum);
		virtual ~ Rts2DevConnMaster (void);

		virtual int init ();
		virtual int idle ();
		int authorize (Rts2DevConn * conn);

		virtual void setConnState (conn_state_t new_conn_state);
};

/**
 * Register device to central server.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Rts2CommandRegister:public Rts2Command
{
	public:
		Rts2CommandRegister (Rts2Block * in_master, int centrald_num, const char *device_name, int device_type, const char *device_host, int device_port)
			:Rts2Command (in_master)
		{
			std::ostringstream _os;
			_os << "register " << centrald_num
				<< " " << device_name
				<< " " << device_type
				<< " " << device_host
				<< " " << device_port;
			setCommand (_os);
		}

		virtual int commandReturnOK (Rts2Conn *conn)
		{
			conn->setConnState (CONN_AUTH_OK);
			return Rts2Command::commandReturnOK (conn);
		}

		virtual int commandReturnFailed (int status, Rts2Conn *conn)
		{
			conn->endConnection ();
			return Rts2Command::commandReturnFailed (status, conn);
		}
};

/**
 * Send status_info command from device to master, and wait for reply.
 *
 * This command works with Rts2CommandStatusInfo to ensure, that proper device
 * state is retrieved.
 *
 * @msc
 *
 * Block, Device, Centrald, Devices;
 *
 * Block->Device [label="device_status"];
 * Device->Centrald [label="status_info"];
 * Centrald->Devices [label="status_info"];
 * Devices->Centrald [label="S xxxx"];
 * Centrald->Device [label="S xxxx"];
 * Device->Block [label="S xxxx"];
 * Device->Block [label="OK"];
 * Centrald->Device [label="OK"];
 * Devices->Centrald [label="OK"];
 *
 * @endmsc
 *
 * @ingroup RTS2Command
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Rts2CommandDeviceStatusInfo:public Rts2Command
{
	private:
		Rts2Conn * owner_conn;
	public:
		Rts2CommandDeviceStatusInfo (Rts2Device * master, Rts2Conn * in_owner_conn);
		virtual int commandReturnOK (Rts2Conn * conn);
		virtual int commandReturnFailed (int status, Rts2Conn * conn);

		Rts2Conn *getOwnerConn ()
		{
			return owner_conn;
		}

		virtual void deleteConnection (Rts2Conn * conn);
};


/**
 * Represents RTS2 device. From this class, different devices are
 * derived.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Rts2Device:public Rts2Daemon
{
	private:
		std::list <HostString> centraldHosts;

		// device current full BOP state
		int fullBopState;

		// mode related variable
		char *modefile;
		Rts2ConfigRaw *modeconf;
		Rts2ValueSelection *modesel;

		int loadModefile ();

		int device_port;
		const char *device_name;
		int device_type;

		int log_option;

		char *device_host;

		char *mailAddress;

		/**
		 * Set mode from modefile.
		 *
		 * Values for each device can be specified in modefiles.
		 *
		 * @param new_mode       New mode number.
		 * @param defaultValues  If true, will set default values.
		 *
		 * @return -1 on error, 0 if sucessful.
		 */
		int setMode (int new_mode, bool defaultValues = false);

		int blockState;
		Rts2CommandDeviceStatusInfo *deviceStatusCommand;

	protected:
		/**
		 * Process on option, when received from getopt () call.
		 *
		 * @param in_opt  return value of getopt
		 * @return 0 on success, -1 if option wasn't processed
		 */
		virtual int processOption (int in_opt);
		virtual int init ();
		virtual int initValues ();

		virtual bool isRunning (Rts2Conn *conn)
		{
			if (!conn->isConnState (CONN_AUTH_OK))
			{
				logStream (MESSAGE_DEBUG) << "is_running " << conn->getConnState () << sendLog;
			}
			return conn->isConnState (CONN_AUTH_OK);
		}

		/**
		 * Return device BOP state.
		 * This state is specific for device, and contains BOP values
		 * only from devices which can block us.
		 */
		int getDeviceBopState ()
		{
			return fullBopState;
		}

		void queDeviceStatusCommand (Rts2Conn *in_owner_conn);

		void clearStatesPriority ();

		// sends operation block commands to master
		// this functions should mark critical blocks during device execution
		void blockExposure ()
		{
			maskState (BOP_EXPOSURE, BOP_EXPOSURE, "exposure not possible");
		}

		/**
		 * Get blocking exposure status.
		 *
		 * @return true if device is blocking exposure.
		 */
		bool blockingExposure ()
		{
			return getState () & BOP_EXPOSURE;
		}

		void clearExposure ()
		{
			maskState (BOP_EXPOSURE, 0, "exposure possible");
		}

		void blockReadout ()
		{
			maskState (BOP_READOUT, BOP_READOUT, "readout not possible");
		}
		void clearReadout ()
		{
			maskState (BOP_READOUT, 0, "readout possible");
		}

		void blockTelMove ()
		{
			maskState (BOP_TEL_MOVE, BOP_TEL_MOVE, "telescope move not possible");
		}
		void clearTelMove ()
		{
			maskState (BOP_TEL_MOVE, 0, "telescope move possible");
		}

		virtual Rts2Conn *createClientConnection (Rts2Address * in_addr);

		/**
		 * Loop through que values and tries to free as much of them as is possible.
		 *
		 * @param fakeState State of the device. This one is not set in
		 * server state, it's only used during value tests.
		 */
		virtual void checkQueChanges (int fakeState);

		virtual void stateChanged (int new_state, int old_state, const char *description);

		virtual void cancelPriorityOperations ();

		virtual int setValue (Rts2Value * old_value, Rts2Value * new_value);

	public:
		Rts2Device (int in_argc, char **in_argv, int in_device_type,
			const char *default_name);
		virtual ~Rts2Device (void);
		virtual Rts2DevConn *createConnection (int in_sock);

		/**
		 * Called for commands when connection is authorized. Command processing is
		 * responsible for sending back error message when -1 is returned. Connection
		 * command handling sends out OK acknowledgement when 0 is returned.
		 *
		 * @param conn Connection which is sending the command.
		 *
		 * @return -1 when an error occured while processing command, 0 when command
		 * was accepted and performed, -5 when command was not accepted, but should
		 * be left to processing.
		 *
		 */
		virtual int commandAuthorized (Rts2Conn * conn);

		int authorize (int centrald_num, Rts2DevConn * conn);

		/**
		 * Send commands to all masters which device has connected.
		 *
		 * @param msg Message that will be send to the masters.
		 *
		 * @return 0 on success, -1 on error.
		 */
		int sendMasters (const char *msg);

		virtual void centraldConnRunning (Rts2Conn *conn);

		/**
		 * Send device status info to given connection.
		 */
		void sendStateInfo (Rts2Conn * conn);

		/**
		 * Send full state, including device full BOP state.
		 */
		void sendFullStateInfo (Rts2Conn * conn);

		// only devices can send messages
		virtual void sendMessage (messageType_t in_messageType,
			const char *in_messageString);

		int sendMail (const char *subject, const char *text);

		int killAll ();
		virtual int scriptEnds ();

		const char *getDeviceName ()
		{
			return device_name;
		};
		int getDeviceType ()
		{
			return device_type;
		};

		virtual int statusInfo (Rts2Conn * conn);

		/**
		 * Called to set current device BOP state.
		 *
		 * @param new_state New BOP state.
		 */
		virtual void setFullBopState (int new_state);

		/**
		 * Hook called to mask device BOP state with possible blocking values from que.
		 *
		 * @param new_state New BOP state.
		 * @param valueQueCondition Que condition of the value.
		 *
		 * @return Masked BOP state.
		 */
		virtual int maskQueValueBopState (int new_state, int valueQueCondition);

		/**
		 * Called when status_info command ends.
		 */
		void endDeviceStatusCommand ()
		{
			if (deviceStatusCommand && deviceStatusCommand->getOwnerConn ())
			{
				deviceStatusCommand->getOwnerConn ()->sendCommandEnd (DEVDEM_OK, "device status updated");
				deviceStatusCommand = NULL;
			}
		}

		virtual void signaledHUP ();
};
#endif							 /* !__RTS2_DEVICE__ */
