/* 
 * Daemon class.
 * Copyright (C) 2005-2010 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_DAEMON__
#define __RTS2_DAEMON__

#include "block.h"
#include "iniparser.h"
#include "logstream.h"
#include "value.h"
#include "valuelist.h"
#include "valuestat.h"
#include "valueminmax.h"
#include "valuerectangle.h"
#include "valuearray.h"

#include <vector>

namespace rts2core
{

/**
 * Abstract class for centrald and all devices.
 *
 * This class contains functions which are common to components with one listening socket.
 *
 * @ingroup RTS2Block
 */
class Daemon:public Block
{
	public:
		/**
		 * Construct daemon.
		 *
		 * @param _argc         Number of arguments.
		 * @param _argv         Arguments values.
		 * @param _init_state   Initial state.
		 */
		Daemon (int in_argc, char **in_argv, int _init_state = 0);

		virtual ~ Daemon (void);

		/**
		 * Handle autorestart option - if really needed.
		 */
		int setupAutoRestart ();

		virtual int run ();

		virtual void endRunLoop ();

		/**
		 * Init daemon.
		 * This is call to init daemon. It calls @see Daemon::init and @see Daemon::initValues
		 * functions to complete daemon initialization.
		 */
		void initDaemon ();
		
		/**
		 * Set timer to send updates every interval seconds. This method is designed to keep user 
		 * infromed about progress of actions which rapidly changes values read by info call.
		 *
		 * @param interval If > 0, set idle info interval to this value. Daemon will then call info method every interval 
		 *   seconds and distribute updates to all connected clients. If <= 0, disables automatic info.
		 *
		 * @see info()
		 */  
		void setIdleInfoInterval (double interval);

		/**
		 * Updates info_time to current time.
		 */
		void updateInfoTime ()
		{
			info_time->setNow ();
		}

		virtual void postEvent (Event *event);

		virtual void forkedInstance ();
		virtual void sendMessage (messageType_t in_messageType, const char *in_messageString);
		virtual void centraldConnRunning (Rts2Connection *conn);
		virtual void centraldConnBroken (Rts2Connection *conn);

		/**
		 * Send status message to all connected clients.
		 *
		 * @param state State value which will be send.
		 *
		 * @callergraph
		 *
		 * @see PROTO_STATUS
		 */
		void sendStatusMessage (rts2_status_t state, const char * msg = NULL, Rts2Connection *commandedConn = NULL);

		/**
		 * Send status message to one connection.
		 *
		 * @param state State value which will be send.
		 * @param conn Rts2Connection to which the state will be send.
		 *
		 * @callergraph
		 *
		 * @see PROTO_STATUS
		 */
		void sendStatusMessageConn (rts2_status_t state, Rts2Connection *conn);

		virtual int baseInfo ();
		int baseInfo (Rts2Connection * conn);
		int sendBaseInfo (Rts2Connection * conn);

		virtual int info ();
		int info (Rts2Connection * conn);
		int infoAll ();
		void constInfoAll ();
		int sendInfo (Rts2Connection * conn, bool forceSend = false);

		int sendMetaInfo (Rts2Connection * conn);

		virtual void addPollSocks ();
		virtual void pollSuccess ();

		virtual int setValue (Rts2Connection * conn);

		/**
		 * Return full device state. You are then responsible
		 * to use approproate mask defined in status.h to extract
		 * from state information that you want.
		 *
		 * @return Block state.
		 */
		rts2_status_t getState () { return state; }

		/**
		 * Retrieve value with given name.
		 *
		 * @return NULL if value with specifed name does not exists
		 */
		Value *getOwnValue (const char *v_name);

		/**
		 * Return iterator to the first value.
		 */
		CondValueVector::iterator getValuesBegin () { return values.begin (); }

		/**
		 * Return iterator to the last value.
		 */
		CondValueVector::iterator getValuesEnd () { return values.end (); }

		/**
		 * Create new value, and return pointer to it.
		 * It also saves value pointer to the internal values list.
		 *
		 * @param value          Value which will be created.
		 * @param in_val_name    Value name.
		 * @param in_description Value description.
		 * @param writeToFits    When true, value will be writen to FITS.
		 * @param valueFlags     Value display type, one of the RTS2_DT_xxx constant.
		 * @param queCondition   Conditions in which the change will be put to que.
		 */
		template < typename T > void createValue (
			T * &val,
			const char *in_val_name,
			std::string in_description,
			bool writeToFits = true,
			int32_t valueFlags = 0,
			int queCondition = 0)
		{
			if (getOwnValue (in_val_name))
				throw rts2core::Error (std::string ("duplicate call to create value ") + in_val_name);
			val = new T (in_val_name, in_description, writeToFits, valueFlags);
			addValue (val, queCondition);
		}

		/**
		 * Send new value over the wire to all connections.
		 */
		void sendValueAll (Value * value);

		/**
		 * Send progress to newly created connections.
		 */
		void resendProgress (Rts2Connection *conn)
		{
			if (!(isnan (state_start) && isnan (state_expected_end)))
				conn->sendProgress (state_start, state_expected_end);
		}

		/**
		 * Load value file, specified as argument.
		 * Value file list variable names and values on "variable_name=value " lines.
		 */
		int loadValuesFile (const char *valuefile, bool use_extensions = false);

		/**
		 * Autosave values marked for autosaving.
		 */
		int autosaveValues ();

	protected:
		/**
		 * Delete all saved reference of given value.
		 *
		 * Used to clear saved values when the default value shall be changed.
		 * 
		 * @param val Value which references will be deleted.
		 */
		void deleteSaveValue (CondValue * val);

		/**
		 * Send progress parameters.
		 *
		 * @param start  operation start time
		 * @param end    operation end time
		 */
		void sendProgressAll (double start, double end, Rts2Connection *except);

		int checkLockFile (const char *_lock_fname);


		void setNotDaemonize ()
		{
			daemonize = DONT_DAEMONIZE;
		}

		/**
		 * Do not create lock file. Usefull when lock file handling is part of another
		 * class in the process (e.g. in multidev design).
		 */
		void setNoLock ()
		{
			lock_file = -2;
		}

		/**
		 * Returns true if daemon should stream debug messages to standart output.
		 *
		 * @return True when debug messages can be printed.
		 */
		bool printDebug ()
		{
			return daemonize == DONT_DAEMONIZE;
		}

		/**
		 * Called before main daemon loop.
		 */
		virtual void beforeRun () {}

		/**
		 * Returns true if connection is running.
		 */
		virtual bool isRunning (Rts2Connection *conn) = 0;

		virtual int checkNotNulls ();

		/**
		 * Call fork to start true daemon. This method must be called
		 * before any threads are created.
		 */
		int doDaemonize ();

		/**
		 * Set lock prefix.
		 *
		 * @param _lockPrefix New lock prefix.
		 */
		void setLockPrefix (const char *_lockPrefix)
		{
			lockPrefix = _lockPrefix;
		}
	
		/**
		 * Return prefix (directory) for lock files.
		 *
		 * @return Prefix for lock files.
		 */
		const char *getLockPrefix ();

		/**
		 * Write to lock file process PID, and lock it.
		 *
		 * @return 0 on success, -1 on error.
		 */
		int lockFile ();

		ValueQueVector queValues;

		/**
		 * Return conditional value entry for given value name.
		 *
		 * @param v_name Value name.
		 *
		 * @return CondValue for given value, if this exists. NULL if it does not exist.
		 */
		CondValue *getCondValue (const char *v_name);

		/**
		 * Return conditional value entry for given value.
		 *
		 * @param val Value entry.
		 *
		 * @return CondValue for given value, if this exists. NULL if it does not exist.
		 */
		CondValue *getCondValue (const Value *val);

		/**
		 * Duplicate variable.
		 *
		 * @param old_value Old value of variable.
		 * @param withVal When set to true, variable will be duplicated with value.
		 *
		 * @return Duplicate of old_value.
		 */
		Value *duplicateValue (Value * old_value, bool withVal = false);

		/**
		 * Create new constant value, and return pointer to it.
		 *
		 * @param value          Value which will be created
		 * @param in_val_name    value name
		 * @param in_description value description
		 * @param writeToFits    when true, value will be writen to FITS
		 */
		template < typename T > void createConstValue (T * &val, const char *in_val_name,
			std::string in_description,
			bool writeToFits =
			true, int32_t valueFlags = 0)
		{
			val = new T (in_val_name, in_description, writeToFits, valueFlags);
			addConstValue (val);
		}

		/**
		 * Adds value to list of values supported by daemon.
		 *
		 * @param value Value which will be added.
		 */
		void addValue (Value * value, int queCondition = 0);

		void addConstValue (Value * value);
		void addConstValue (const char *in_name, const char *in_desc, const char *in_value);
		void addConstValue (const char *in_name, const char *in_desc, std::string in_value);
		void addConstValue (const char *in_name, const char *in_desc, double in_value);
		void addConstValue (const char *in_name, const char *in_desc, int in_value);
		void addConstValue (const char *in_name, const char *in_value);
		void addConstValue (const char *in_name, double in_value);
		void addConstValue (const char *in_name, int in_value);

		/**
		 * Set value. This is the function that get called when user want to change some value, either interactively through
		 * rts2-mon, XML-RPC or from the script. You can overwrite this function in descendants to allow additional variables 
		 * beiing overwritten. If variable has flag RTS2_VALUE_WRITABLE, default implemetation returns sucess. If setting variable
		 * involves some special commands being send to the device, you most probably want to overwrite setValue, and provides
		 * set action for your values in it.
		 *
		 * Suppose you have variables var1 and var2, which you would like to be settable by user. When user set var1, system will just change
		 * value and pick it up next time it will use it. If user set integer value var2, method setVar2 should be called to communicate
		 * the change to the underliing hardware. Then your setValueMethod should looks like:
		 *
		 * @code
		 * class MyClass:public MyParent
		 * {
		 *   ....
		 *   protected:
		 *       virtual int setValue (Value * old_value, Value *new_value)
		 *       {
		 *             if (old_value == var1)
		 *                   return 0;
		 *             if (old_value == var2)
		 *                   return setVar2 (new_value->getValueInteger ()) == 0 ? 0 : -2;
		 *             return MyParent::setValue (old_value, new_value);
	         *       }
		 *   ...
		 * };
		 *
		 * @endcode
		 *
		 * @param  old_value	Old value (pointer), can be directly
		 *        accesed with the pointer stored in object.
		 * @param new_value	New value.
		 *
		 * @return 1 when value can be set, but it will take longer
		 * time to perform, 0 when value can be se immediately, -1 when
		 * value set was queued and -2 on an error.
		 */
		virtual int setValue (Value * old_value, Value * new_value);

		/**
		 * Sets value from the program. Should be called if the code
		 * needs to change value and cannot gurantee change to happen at
		 * correct time - e.g. when device is idle, if the value require so.
		 *
		 * @param value   value which is to change (or queue for change)
		 * @param nval    new value
		 */
		void changeValue (Value * value, int nval);

		/**
		 * Sets value from the program. Should be called if the code
		 * needs to change value and cannot gurantee change to happen at
		 * correct time - e.g. when device is idle, if the value require so.
		 *
		 * @param value   value which is to change (or queue for change)
		 * @param nval    new value
		 */
		void changeValue (Value * value, bool nval);

		/**
		 * Sets value from the program. Should be called if the code
		 * needs to change value and cannot gurantee change to happen at
		 * correct time - e.g. when device is idle, if the value require so.
		 *
		 * @param value   value which is to change (or queue for change)
		 * @param nval    new value
		 */
		void changeValue (Value * value, double nval);

		/**
		 * Perform value changes. Check if value can be changed before performing change.
		 *
		 * @return 0 when value change can be performed, -2 on error, -1 when value change is qued.
		 */
		int setCondValue (CondValue * old_value_cond, char op, Value * new_value);

		/**
		 * Perform value change.
		 *
		 * @param old_cond_value
		 * @param op Operation string. Permited values depends on value type
		 *   (numerical or string), but "=", "+=" and "-=" are ussualy supported.
		 * @param new_value
		 */
		int doSetValue (CondValue * old_cond_value, char op, Value * new_value);

		/**
		 * Called after value was changed.
		 *
		 * @param changed_value Pointer to changed value.
		 */
		virtual void valueChanged (Value *changed_value);

		/**
		 * Returns whenever value change with old_value needs to be qued or
		 * not.
		 *
		 * @param old_value CondValue object describing the old_value
		 * @param fakeState Server state agains which value change will be checked.
		 */
		bool queValueChange (CondValue * old_value, rts2_status_t fakeState)
		{
			return old_value->queValueChange (fakeState);
		}

		virtual int processOption (int in_opt);
		virtual int processArgs (const char *arg);

		virtual int init ();
		virtual int initValues ();
		virtual int idle ();

		/**
		 * Set info time to supplied date. Please note that if you use this function,
		 * you should consider not calling standard info () routine, which updates
		 * info time - just overwrite its implementation with empty body.
		 *
		 * @param _date  Date of the infotime.
		 */
		void setInfoTime (struct tm *_date);

		/**
		 * Set infotime from time_t structure.
		 *
		 * @param _time Time_t holding time (in seconds from 1-1-1970) to which set info time.
		 */
		void setInfoTime (time_t _time)
		{
			info_time->setValueInteger (_time);
		}

		double getInfoTime () { return info_time->getValueDouble (); }

		/**
		 * Get time from last info time in seconds (and second fractions).
		 *
		 * @return Difference from last info time in seconds. 86400 (one day) if info time was not defined.
		 */
		double getLastInfoTime ()
		{
			if (isnan (info_time->getValueDouble ()))
				return 86400;
			return getNow () - info_time->getValueDouble ();	
		}


		/**
		 * Called to set new state value
		 */
		void setState (rts2_status_t new_state, const char *description, Rts2Connection *commandedConn);

		/**
		 * Called when state of the device is changed.
		 *
		 * @param new_state   New device state.
		 * @param old_state   Old device state.
		 * @param description Text description of state change.
		 */
		virtual void stateChanged (rts2_status_t new_state, rts2_status_t old_state, const char *description, Rts2Connection *commandedConn);

		/**
		 * Called from idle loop after HUP signal occured.
		 *
		 * This is most probably callback you needed for handling HUP signal.
		 * Handling HUP signal when it occurs can be rather dangerous, as it might
		 * reloacte memory location - if you read pointer before HUP signal and use
		 * it after HUP signal, RTS2 does not guarantee that it will be still valid.
		 */
		virtual void signaledHUP ();

		/**
		 * Send state change to all connection.
		 */
		void maskState (rts2_status_t state_mask, rts2_status_t new_state, const char *description = NULL, double start = NAN, double end = NAN, Rts2Connection *commandedConn = NULL);

		/**
		 * Raise hardware error status bit.
		 */
		void raiseHWError () { maskState (DEVICE_ERROR_HW, DEVICE_ERROR_HW, "device error", getNow ()); }

		/**
		 * Clear hardware error status bit.
		 */
		void clearHWError () { maskState (DEVICE_ERROR_HW, 0, "error cleared"); }

		/**
		 * Return daemon state without ERROR information.
		 */
		int getDaemonState ()
		{
			return state & ~DEVICE_ERROR_MASK;
		}

		/**
		 * Call external script on trigger. External script gets in
		 * enviroment values current RTS2 values as
		 * RTS2_{device_name}_{value}, and trigger reason as first
		 * argument. This call creates new connection. Everything
		 * outputed to standart error will be logged as WARN message.
		 * Everything outputed to standart output will be logged as
		 * DEBUG message.
		 *
		 * @param reason Trigger name.
		 */
		void execTrigger (const char *reason);

		/**
		 * Stop any movement - emergency situation.
		 */
		bool getStopEverything ()
		{
			return (getState () & STOP_MASK) == STOP_EVERYTHING;
		}

		/**
		 * Get daemon local weather state. Please use isGoodWeather()
		 * to test for system weather state.
		 *
		 * @ returns true if weather state is good.
		 */
		bool getWeatherState ()
		{
			return (getState () & WEATHER_MASK) == GOOD_WEATHER;
		}

		/**
		 * Set weather state.
		 *
		 * @param good_weather If true, weather is good.
		 * @param msg Text message for state transition - in case of bad weather, it will be recorded in centrald
		 */
		virtual void setWeatherState (bool good_weather, const char *msg)
		{
			maskState (WEATHER_MASK, good_weather ? GOOD_WEATHER : BAD_WEATHER, msg);
		}

		bool getStopState ()
		{
			return (getState () & STOP_MASK) == STOP_EVERYTHING;
		}

		void setStopState (bool stop_state, const char *msg)
		{
			maskState (STOP_MASK, stop_state ? STOP_EVERYTHING : CAN_MOVE, msg);
		}

		virtual void sigHUP (int sig);

		/**
		 * Called to verify that the infoAll call can be called.
		 *
		 * @return True if infoAll can be called. When returns False, infoAll will not be called from timer.
		 *
		 * @see setIdleInfoInterval()
		 */
		virtual bool canCallInfoFromTimer () { return true; }

		/**
		 * Set mode from modefile.
		 *
		 * Values for each device can be specified in modefiles.
		 *
		 * @param new_mode       New mode number.
		 *
		 * @return -1 on error, 0 if sucessful.
		 */
		int setMode (int new_mode);

		rts2core::ValueSelection *modesel;

		/**
		 * Add group to group list. Group values are values prefixed with 
		 * group name, followed by '.'.
		 *
		 * For example if group list specify groups with names A and B, A.aa and
		 * B.bb are group values. A_aa is not a group value.
		 */
		void addGroup (const char *groupname);

		void setDefaultsFile (const char *fn) { optDefaultsFile = fn; }

	private:
		rts2core::StringArray *groups;

		// 0 - don't daemonize, 1 - do daemonize, 2 - is already daemonized, 3 - daemonized & centrald is running, don't print to stdout
		enum { DONT_DAEMONIZE, DO_DAEMONIZE, IS_DAEMONIZED, CENTRALD_OK } daemonize;
		// <= 0 - don't autorestart, > 0 - wait autorestart seconds, then restart daemon
		int autorestart;
		pid_t watched_child;
		int listen_sock;
		void addConnectionSock (int in_sock);
		const char * lock_fname;

		// lock file FD. -2 signals device does not create lock file (usually part of multidev, where only master creates lock file)
		int lock_file;

		const char *runAs;

		// daemon state
		rts2_status_t state;

		CondValueVector values;
		// values which do not change, they are send only once at connection
		// initialization
		ValueVector constValues;

		ValueTime *info_time;
		ValueTime *uptime;

		double idleInfoInterval;

		bool doHupIdleLoop;

		// mode related variable
		const char *modefile;
		IniParser *modeconf;

		const char *valueFile;

		/**
		 * Prefix (directory) for lock file.
		 */
		const char *lockPrefix;

		/**
		 * File where to store autosaved values.
		 */
		const char *autosaveFile;
		const char *optAutosaveFile;
		const char *optDefaultsFile;

		/**
		 * Switch running user to new user (and group, if provided with .)
		 *
		 * @param usrgrp user and possibly group, separated by .
		 */
		void switchUser (const char *usrgrp);

		int loadCreateFile ();
		int loadModefile ();

		/**
		 * Load values from ini file section.
		 *
		 * @param sect IniSection containting values to load
		 * @param new_mode New mode. 0 for default/no mode
		 * @param use_extensions If true, will look for .min and .max extensions (and will fail if others are provided)
		 */
		int setSectionValues (IniSection *sect, int new_mode, bool use_extensions);

		/**
		 * Create section values.
		 *
		 * @param sect      section which holds new values
		 */
		int createSectionValues (IniSection *sect);

		/**
		 * Set value, regardless if it is write-enabled or not.
		 *
		 * @param val pointer to value to set
		 * @param new_value pointer to new value (shall be created from val with duplicateValue function)
		 *
		 * @return -2 on error
		 */
		int setInitValue (rts2core::Value *val, rts2core::Value *new_value);

		double state_start;
		double state_expected_end;

		std::map <std::string, std::string> argValues;
};

}
#endif							 /* ! __RTS2_DAEMON__ */
