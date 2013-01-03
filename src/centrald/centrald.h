/*
 * Centrald - RTS2 coordinator
 * Copyright (C) 2003-2013 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_CENTRALD__
#define __RTS2_CENTRALD__

#include "riseset.h"

#include <libnova/libnova.h>

#include <rts2-config.h>
#include "daemon.h"
#include "configuration.h"
#include "status.h"

using namespace rts2core;

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX   255
#endif

namespace rts2centrald
{

class ConnCentrald;

/**
 * Class for central server.
 *
 * Central server supports following functions:
 *
 * - holds list of active system components
 * - authorizes new connections to the system
 * - activates pool of system BOP mask via "status_info" (CommandStatusInfo) command
 * - holds and manage connection priorities
 *
 * Centrald is written to be as simple as possible. Commands are ussually
 * passed directly to devices on separate connections, they do not go through
 * centrald.
 *
 * @see Device
 * @see DevConnMaster
 * @see Client
 * @see ConnCentraldClient
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Centrald:public Daemon
{
	public:
		Centrald (int argc, char **argv);
		virtual ~ Centrald (void);

		virtual int info ();
		virtual int idle ();

		virtual void deviceReady (rts2core::Connection * conn);

		/**
		 * Switch centrald state to ON.
		 *
		 * @param user Name of user who initiated state change.
		 */
		int changeStateOn (const char *user)
		{
			return changeState ((next_event_type + 5) % 6, user);
		}

		/**
		 * Switch centrald to standby.
		 *
		 * @param user Name of user who initiated state change.
		 */
		int changeStateStandby (const char *user)
		{
			return changeState (SERVERD_STANDBY | ((next_event_type + 5) % 6), user);
		}

		/**
		 * Switch centrald to off.
		 *
		 * @param user Name of user who initiated state change.
		 */
		int changeStateHardOff (const char *user)
		{
			return changeState (SERVERD_HARD_OFF, user);
		}

		/**
		 * Change state tp soft off.
		 *
		 * @param user Name of the user who initiated state change.
		 */
		int changeStateSoftOff (const char *user)
		{
			return changeState (SERVERD_SOFT_OFF, user);
		}

		virtual rts2core::Connection *createConnection (int in_sock);
		void connAdded (ConnCentrald * added);

		/**
		 * Return connection based on conn_num.
		 *
		 * @param conn_num Connection number, which uniquely identified
		 * every connection.
		 *
		 * @return Connection with given connection number.
		 */
		rts2core::Connection *getConnection (int conn_num);

		void sendMessage (messageType_t in_messageType, const char *in_messageString);

		virtual void message (Message & msg);

		/**
		 * Called when conditions which determines weather state changed.
		 * Those conditions are:
		 *
		 * <ul>
		 *   <li>changed weather state of a single device connected to centrald</li>
		 *   <li>creating or removal of a connection to an device</li>
		 * </ul>
		 *
		 * This routine is also called periodically, as weather can go
		 * bad if we do not hear from a device for some time. This
		 * periodic call is there to prevent situations when connection
		 * will not be broken, but will not transwer any usable data to
		 * centrald.
		 *
		 * @param deviceName  Name of device triggering weather change.
		 * @param msg         Message associated with weather change.
		 *
		 * @callgraph
		 */
		void weatherChanged (const char * device, const char * msg);

		/**
		 * Call to update weather state of a connection.
		 */
		int weatherUpdate (rts2core::Connection *conn);

		/**
		 * Called when some device signal stop state.
		 *
		 * @callgraph
		 */
		void stopChanged (const char *device, const char * msg);

		/**
		 * Called when block of operation device mask changed. It checks
		 * blocking state of all devices and updates accordingly master
		 * blocking state.
		 */
		void bopMaskChanged ();

		virtual int statusInfo (rts2core::Connection * conn);

		/**
		 * Return state of system, as seen from device identified by connection.
		 *
		 * This command return state. It is similar to Daemon::getState() call.
		 * It result only differ when connection which is asking for state is a
		 * device connection. In this case, BOP mask is composed only from devices
		 * which can block querying device.
		 *
		 * The blocking devices are specified by blocking_by parameter in rts2.ini
		 * file.
		 *
		 * @param conn Connection which is asking for state.
		 */
		int getStateForConnection (rts2core::Connection * conn);

	protected:
		/**
		 * @param new_state	new state, if -1 -> 3
		 */
		int changeState (int new_state, const char *user);

		virtual int processOption (int in_opt);

		/**
		 * Those callbacks are for current centrald implementation empty and returns
		 * NULL. They can be used in future to link two centrald to enable
		 * cooperative observation.
		 */
		virtual rts2core::Connection *createClientConnection (char *in_deviceName)
		{
			return NULL;
		}

		virtual rts2core::Connection *createClientConnection (NetworkAddress * in_addr)
		{
			return NULL;
		}

		virtual int init ();
		virtual int initValues ();

		virtual bool isRunning (rts2core::Connection *conn)
		{
			return conn->isConnState (CONN_CONNECTED);
		}

		virtual void connectionRemoved (rts2core::Connection * conn);

		virtual int setValue (rts2core::Value *old_value, rts2core::Value *new_value);

		virtual void stateChanged (int new_state, int old_state, const char *description, rts2core::Connection *commandedConn);

		virtual void signaledHUP ();

	private:
		// called to change state, check if last_night_on should be set
		void maskCentralState (int state_mask, int new_state, const char *description = NULL, double start = NAN, double end = NAN, Connection *commandedConn = NULL);

		// -1 if no connection has priority, -2 if the process is exiting and there aren't any valid connections,
		// otherwise connection number of priority client
		int priority_client;

		int next_event_type;
		time_t next_event_time;
		struct ln_lnlat_posn *observer;

		rts2core::ValueBool *morning_off;
		rts2core::ValueBool *morning_standby;

		StringArray *requiredDevices;
		StringArray *failedDevices;

		rts2core::ValueString *badWeatherReason;
		rts2core::ValueString *badWeatherDevice;

		char *configFile;
		std::string logFile;
		// which sets logfile
		enum { LOGFILE_ARG, LOGFILE_DEF, LOGFILE_CNF }
		logFileSource;

		std::ofstream * fileLog;

		void openLog ();
		int reloadConfig ();

		int connNum;

		rts2core::ValueString *priorityClient;
		rts2core::ValueInteger *priority;

		rts2core::ValueTime *nextStateChange;
		rts2core::ValueSelection *nextState;
		rts2core::ValueDouble *observerLng;
		rts2core::ValueDouble *observerLat;

		rts2core::ValueDouble *nightHorizon;
		rts2core::ValueDouble *dayHorizon;

		rts2core::ValueInteger *eveningTime;
		rts2core::ValueInteger *morningTime;

		rts2core::ValueTime *nightStart;
		rts2core::ValueTime *nightStop;

		rts2core::TimeArray *switchedStandby;

		// time when system was last switched to ON and was in ready_night
		rts2core::ValueTime *lastOn;

		rts2core::ValueDouble *sunAlt;
		rts2core::ValueDouble *sunAz;

		rts2core::ValueTime *sunRise;
		rts2core::ValueTime *sunSet;

		rts2core::ValueDouble *moonAlt;
		rts2core::ValueDouble *moonAz;

		rts2core::ValueDouble *lunarPhase;
		rts2core::ValueDouble *lunarLimb;

		rts2core::ValueTime *moonRise;
		rts2core::ValueTime *moonSet;

		void processMessage (Message & msg);
};

/**
 * Represents connection from device or client to centrald.
 *
 * It is used in Centrald.
 */
class ConnCentrald:public rts2core::Connection
{
	private:
		int authorized;
		std::string login;
		Centrald *master;
		char hostname[HOST_NAME_MAX];
		int port;
		int device_type;

		// number of status commands device have running
		int statusCommandRunning;

		/**
		 * Handle serverd commands.
		 *
		 * @return -2 on exit, -1 and set errno on HW failure, 0 otherwise
		 */
		int command ();
		int commandDevice ();
		int commandClient ();
		// command handling functions
		int priorityCommand ();
		int sendDeviceKey ();
		int sendInfo ();

		/**
		 * Prints standard status header.
		 *
		 * It needs to be called after establishing of every new connection.
		 */
		int sendStatusInfo ();
		int sendAValue (const char *name, int value);
		int messageMask;

	protected:
		virtual void setState (int in_value, char * msg);

	public:
		ConnCentrald (int in_sock, Centrald * in_master, int in_centrald_id);
		/**
		 * Called on connection exit.
		 *
		 * Delete client|device login|name, updates priorities, detach shared
		 * memory.
		 */
		virtual ~ ConnCentrald (void);
		virtual int sendMessage (Message & msg);
		int sendConnectedInfo (rts2core::Connection * conn);

		virtual void updateStatusWait (rts2core::Connection * conn);

		void statusCommandSend ()
		{
			statusCommandRunning++;
		}
};

}
#endif							 /*! __RTS2_CENTRALD__ */
