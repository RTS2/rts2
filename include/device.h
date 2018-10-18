/*
 * Device basic class.
 * Copyright (C) 2003-2010 Petr Kubanek <petr@kubanek.net>
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
#include "command.h"
#include "daemon.h"

namespace rts2core
{

class Device;

/**
 * Device connection.
 *
 * Handles both connections which are created from clients to device, as well
 * as connections created from device to device. They are distinguished by
 * connType (set by setType, get by getType calls).
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class DevConnection:public Connection
{
	public:
		DevConnection (int in_sock, Device * in_master);

		virtual int authorizationOK ();
		virtual int authorizationFailed ();

		virtual void setDeviceAddress (NetworkAddress * in_addr);
		void setDeviceName (int _centrald_num, char *_name);

		void setDeviceKey (int _centraldId, int _key);
		virtual void setConnState (conn_state_t new_conn_state);
	protected:
		virtual int command ();
		virtual int init ();

		virtual void connConnected ();
	private:
		// in case we know address of other side..
		NetworkAddress * address;

		Device *master;
};

/**
 * Device connection to master - Rts2Centrald.
 *
 * @see Rts2Centrald
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class DevConnectionMaster:public Connection
{
	public:
		/**
		 * Construct new connection to central server.
		 *
		 * @param _master        rts2core::Block which commands connection.
		 * @param _device_host   Hostname of computer with device.
		 * @param _device_port   Listening port for incoming connections
		 * @param _device_name   Name of the device.
		 * @param _device_type   Device type.
		 * @param _master_host   Central server hostname for the connectio.
		 * @param _master_port   Central server port for the connectio.
		 * @param _serverNum     Server number (number of centrald which device is connected to)
		 */
		DevConnectionMaster (Device * _master, char *_device_host, int _device_port, const char *_device_name, int _device_type, const char *_master_host, int _master_port, int _serverNum);
		virtual ~ DevConnectionMaster (void);

		virtual int init ();
		virtual int idle ();
		int authorize (DevConnection * conn);

		virtual void setConnState (conn_state_t new_conn_state);
	protected:
		virtual int command ();
		virtual void setState (rts2_status_t in_value, char * msg);
		virtual void setBopState (rts2_status_t in_value);
		virtual void connConnected ();
		virtual void connectionError (int last_data_size);
	private:
		char *device_host;
		char master_host[HOST_NAME_MAX];
		int master_port;
		int centrald_num;
		char device_name[DEVICE_NAME_SIZE];
		int device_type;
		int device_port;
		time_t nextTime;

		Device *master;
};


/**
 * Register device to central server.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class CommandRegister:public Command
{
	public:
		CommandRegister (rts2core::Block * in_master, int centrald_num, const char *device_name, int device_type, const char *device_host, int device_port)
			:Command (in_master)
		{
			std::ostringstream _os;
			_os << "register " << centrald_num
				<< " " << device_name
				<< " " << device_type
				<< " " << device_host
				<< " " << device_port;
			setCommand (_os);
		}

		virtual int commandReturnOK (Connection *conn)
		{
			conn->setConnState (CONN_AUTH_OK);
			return Command::commandReturnOK (conn);
		}

		virtual int commandReturnFailed (int status, Connection *conn)
		{
			conn->endConnection ();
			return Command::commandReturnFailed (status, conn);
		}
};

/**
 * Send status_info command from device to master, and wait for reply.
 *
 * This command works with CommandStatusInfo to ensure, that proper device
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
class CommandDeviceStatusInfo:public Command
{
	public:
		CommandDeviceStatusInfo (Device * master, Connection * in_owner_conn);
		virtual int commandReturnOK (Connection * conn);
		virtual int commandReturnFailed (int status, Connection * conn);

		Connection *getOwnerConn ()
		{
			return owner_conn;
		}

		virtual void deleteConnection (Connection * conn);
	private:
		Connection * owner_conn;
};

class MultiDev;

/**
 * Represents RTS2 device. From this class, different devices are
 * derived.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Device:public Daemon
{
	public:
		Device (int in_argc, char **in_argv, int in_device_type, const char *default_name);
		virtual ~Device (void);
		virtual DevConnection *createConnection (int in_sock);

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
		virtual int commandAuthorized (Connection * conn);

		int authorize (int centrald_num, DevConnection * conn);

		/**
		 * Send command to all masters which device has connected.
		 *
		 * @param msg Message that will be send to the masters.
		 *
		 * @return 0 on success, -1 on error.
		 */
		int sendMasters (const char *msg);

		virtual void centraldConnRunning (Connection *conn);

		/**
		 * Send full state, including device full BOP state.
		 */
		void sendFullStateInfo (Connection * conn);

		// only devices can send messages
		virtual void sendMessage (messageType_t in_messageType, const char *in_messageString);

		/**
		 * The interrupt call. This is called on every device on
		 * interruption. The device shall react by switching back to
		 * initial state and be ready for next commands.
		 *
		 * @param callScriptEnd  whenever to call script end
		 *
		 * @return -1 on error.
		 */
		virtual int killAll (bool callScriptEnd);

		/**
		 * This is called from sequencer to let device know that scripts
		 * has ended. Device shall respond by ending all operations and
		 * reseting to original state.
		 */
		virtual int scriptEnds ();

		const char *getDeviceName () { return device_name; }
		int getDeviceType () { return device_type; }

		virtual int statusInfo (Connection * conn);

		/**
		 * Called to set current device BOP state.
		 *
		 * @param new_state New BOP state.
		 */
		virtual void setFullBopState (rts2_status_t new_state);

		virtual rts2core::Value *getValue (const char *_device_name, const char *value_name);

		/**
		 * Hook called to mask device BOP state with possible blocking values from que.
		 *
		 * @param new_state New BOP state.
		 * @param valueQueCondition Que condition of the value.
		 *
		 * @return Masked BOP state.
		 */
		virtual int maskQueValueBopState (rts2_status_t new_state, int valueQueCondition);

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

		/**
		 * Returns true if device connection must be authorized.
		 */
		bool requireAuthorization () { return doAuth; }

		virtual void setWeatherState (bool good_weather, const char *msg);

		friend class MultiDev;

	protected:
		/**
		 * Process on option, when received from getopt () call.
		 *
		 * @param in_opt  return value of getopt
		 * @return 0 on success, -1 if option wasn't processed
		 */
		virtual int processOption (int in_opt);
		virtual int init ();

		/**
		 * Setup device as part of MultiDev, call init method.
		 */
		void setMulti ();

		/**
		 * Init hardware. This method shall close any opened connection
		 * to hardware, and try to (re)-initialize hardware.
		 *
		 * TODO should become pure virtual
		 */
		virtual int initHardware () { return 0; }

		virtual void initAutoSave ();

		virtual void beforeRun ();

		virtual bool isRunning (Connection *conn) { return conn->isConnState (CONN_AUTH_OK) || requireAuthorization () == false; }

		/**
		 * Return device BOP state.
		 * This state is specific for device, and contains BOP values
		 * only from devices which can block us.
		 */
		rts2_status_t getDeviceBopState () { return fullBopState; }

		/**
		 * Return central connection for given central server name.
		 */
		Connection *getCentraldConn (const char *server);

		void queDeviceStatusCommand (Connection *in_owner_conn);

		// sends operation block commands to master
		// this functions should mark critical blocks during device execution
		void blockExposure () { maskState (BOP_EXPOSURE, BOP_EXPOSURE, "exposure not possible"); }

		/**
		 * Get blocking exposure status.
		 *
		 * @return true if device is blocking exposure.
		 */
		bool blockingExposure () { return getState () & BOP_EXPOSURE; }

		void clearExposure () { maskState (BOP_EXPOSURE, 0, "exposure possible"); }

		void blockReadout () { maskState (BOP_READOUT, BOP_READOUT, "readout not possible"); }

		void clearReadout () { maskState (BOP_READOUT, 0, "readout possible"); }

		void blockTelMove () { maskState (BOP_TEL_MOVE, BOP_TEL_MOVE, "telescope move not possible"); }
		void clearTelMove () { maskState (BOP_TEL_MOVE, 0, "telescope move possible"); }

		virtual Connection *createClientConnection (NetworkAddress * in_addr);

		/**
		 * Loop through que values and tries to free as much of them as is possible.
		 *
		 * @param fakeState State of the device. This one is not set in
		 * server state, it's only used during value tests.
		 */
		virtual void checkQueChanges (rts2_status_t fakeState);

		virtual void stateChanged (rts2_status_t new_state, rts2_status_t old_state, const char *description, Connection *commandedConn);

		virtual int setValue (rts2core::Value * old_value, rts2core::Value * new_value);

		void setDeviceName (const char *n) { device_name = n; }

		virtual void initLockFile () { setLockFile (std::string (getLockPrefix ()) + std::string (device_name)); }

	private:
		std::list <HostString> centraldHosts;

		// device current full BOP state
		int fullBopState;

		bool doCheck;
		bool doAuth;

		int device_port;
		const char *device_name;
		int device_type;

		int log_option;

		char *device_host;

		int blockState;
		CommandDeviceStatusInfo *deviceStatusCommand;

		char *last_weathermsg;

		bool multidevPart;
};

}

#endif							 /* !__RTS2_DEVICE__ */
