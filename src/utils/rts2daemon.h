/* 
 * Daemon class.
 * Copyright (C) 2005-2009 Petr Kubanek <petr@kubanek.net>
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

#include "rts2block.h"
#include "rts2logstream.h"
#include "rts2value.h"
#include "rts2valuelist.h"
#include "rts2valuestat.h"
#include "rts2valueminmax.h"
#include "rts2valuerectangle.h"
#include "rts2valuearray.h"

#include <vector>

/**
 * Abstract class for centrald and all devices.
 *
 * This class contains functions which are common to components with one listening socket.
 *
 * @ingroup RTS2Block
 */
class Rts2Daemon:public Rts2Block
{
	private:
		// 0 - don't daemonize, 1 - do daemonize, 2 - is already daemonized, 3 - daemonized & centrald is running, don't print to stdout
		enum
		{ DONT_DAEMONIZE, DO_DAEMONIZE, IS_DAEMONIZED, CENTRALD_OK }
		daemonize;
		int listen_sock;
		void addConnectionSock (int in_sock);
		const char * lock_fname;
		int lock_file;

		// daemon state
		int state;

		Rts2CondValueVector values;
		// values which do not change, they are send only once at connection
		// initialization
		Rts2ValueVector constValues;

		// This vector holds list of values which are currenlty beeing changed
		// It is used to manage BOP mask
		Rts2ValueVector bopValues;

		Rts2ValueTime *info_time;

		time_t idleInfoInterval;
		time_t nextIdleInfo;

		/**
		 * Adds value to list of values supported by daemon.
		 *
		 * @param value Rts2Value which will be added.
		 */
		void addValue (Rts2Value * value, int queCondition = 0, bool save_value = false);

		/**
		 * Holds vector of values which are indendet to be saved. There
		 * are two methods, saveValues and loadValues.  saveValues is
		 * called from setValue(Rts2Value,char,Rts2ValueVector) before
		 * first value value is changed. Once values are saved,
		 * values_were_saved turns to true and we don't call saveValues
		 * before loadValues is called.  loadValues reset
		 * values_were_saved flag, load all values from savedValues
		 */
		Rts2ValueVector savedValues;

		void saveValue (Rts2CondValue * val);

		bool doHupIdleLoop;

		/**
		 * Prefix (directory) for lock file.
		 */
		const char *lockPrefix;

	protected:
		
		/**
		 * Delete all saved reference of given value.
		 *
		 * Used to clear saved values when the default value shall be changed.
		 * 
		 * @param val Value which references will be deleted.
		 */
		void deleteSaveValue (Rts2CondValue * val);

		/**
		 * Send new value over the wire to all connections.
		 */
		void sendValueAll (Rts2Value * value);

		/**
		 * If value needs to be saved, save it.
		 *
		 * @param val Value which will be checked and possibly saved.
		 */
		void checkValueSave (Rts2Value *val);

		int checkLockFile (const char *_lock_fname);
		void setNotDeamonize ()
		{
			daemonize = DONT_DAEMONIZE;
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
		 * Returns true if connection is running.
		 */
		virtual bool isRunning (Rts2Conn *conn) = 0;

		int doDeamonize ();

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

		virtual void addSelectSocks ();
		virtual void selectSuccess ();

		Rts2ValueQueVector queValues;

		Rts2Value *getValue (const char *v_name);

		/**
		 * Return conditional value entry for given value name.
		 *
		 * @param v_name Value name.
		 *
		 * @return Rts2CondValue for given value, if this exists. NULL if it does not exist.
		 */
		Rts2CondValue *getCondValue (const char *v_name);

		/**
		 * Return conditional value entry for given value.
		 *
		 * @param val Value entry.
		 *
		 * @return Rts2CondValue for given value, if this exists. NULL if it does not exist.
		 */
		Rts2CondValue *getCondValue (const Rts2Value *val);

		/**
		 * Duplicate variable.
		 *
		 * @param old_value Old value of variable.
		 * @param withVal When set to true, variable will be duplicated with value.
		 *
		 * @return Duplicate of old_value.
		 */
		Rts2Value *duplicateValue (Rts2Value * old_value, bool withVal = false);

		void loadValues ();

		/**
		 * Create new value, and return pointer to it.
		 * It also saves value pointer to the internal values list.
		 *
		 * @param value          Rts2Value which will be created.
		 * @param in_val_name    Value name.
		 * @param in_description Value description.
		 * @param writeToFits    When true, value will be writen to FITS.
		 * @param valueFlags     Value display type, one of the RTS2_DT_xxx constant.
		 * @param queCondition   Conditions in which the change will be put to que.
		 * @param save_value     True if value is saved and reseted at end of script execution.
		 */
		template < typename T > void createValue (
			T * &val,
			const char *in_val_name,
			std::string in_description,
			bool writeToFits = true,
			int32_t valueFlags = 0,
			int queCondition = 0,
			bool save_value = false)
		{
			val = new T (in_val_name, in_description, writeToFits, valueFlags);
			addValue (val, queCondition, save_value);
		}
		/**
		 * Create new constant value, and return pointer to it.
		 *
		 * @param value          Rts2Value which will be created
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
		void addConstValue (Rts2Value * value);
		void addConstValue (const char *in_name, const char *in_desc, const char *in_value);
		void addConstValue (const char *in_name, const char *in_desc, std::string in_value);
		void addConstValue (const char *in_name, const char *in_desc, double in_value);
		void addConstValue (const char *in_name, const char *in_desc, int in_value);
		void addConstValue (const char *in_name, const char *in_value);
		void addConstValue (const char *in_name, double in_value);
		void addConstValue (const char *in_name, int in_value);

		/**
		 * BOP management routines.
		 * Those routines are used to manage BOP (Block OPeration) mask. It is used
		 * to decide, which actions can be performed without disturbing observation flow.
		 *
		 * @param in_value Value which will be added to blocking values.
		 *
		 * @note This function and @see Rts2Daemon::removeBopValue are called
		 * dynamically during execution of value changes when it is discovered, that
		 * value cannot be changed due to different state of device then is required.
		 */
		void addBopValue (Rts2Value * in_value);

		/**
		 * Remove value from BOP list.
		 *
		 * @see Rts2Daemon::addBopValue
		 *
		 * @param in_value Value which will be removed.
		 *
		 * @pre Value was sucessfully updated.
		 */
		void removeBopValue (Rts2Value * in_value);

		/**
		 * Check if some of the BOP values can be removed from the list.
		 */
		void checkBopStatus ();

		/**
		 * Set value. That one should be owerwrited in descendants.
		 *
		 * @param  old_value	Old value (pointer), can be directly
		 * 	accesed th with pointer stored in object.
		 * @param  new_value	New value.
		 *
		 * @return 1 when value can be set, but it will take longer time to perform,
		 * 0 when value can be se imedietly, -1 when value set was qued and -2 on an
		 * error.
		 */
		virtual int setValue (Rts2Value * old_value, Rts2Value * new_value);

		/**
		 * Perform value changes. Check if value can be changed before performing change.
		 *
		 * @return 0 when value change can be performed, -2 on error, -1 when value change is qued.
		 */
		int setCondValue (Rts2CondValue * old_value_cond, char op, Rts2Value * new_value);

		/**
		 * Really perform value change.
		 *
		 * @param old_cond_value
		 * @param op Operation string. Permited values depends on value type
		 *   (numerical or string), but "=", "+=" and "-=" are ussualy supported.
		 * @param new_value
		 */
		int doSetValue (Rts2CondValue * old_cond_value, char op, Rts2Value * new_value);

		/**
		 * Called after value was changed.
		 *
		 * @param changed_value Pointer to changed value.
		 */
		virtual void valueChanged (Rts2Value *changed_value);

		/**
		 * Returns whenever value change with old_value needs to be qued or
		 * not.
		 *
		 * @param old_value Rts2CondValue object describing the old_value
		 * @param fakeState Server state agains which value change will be checked.
		 */
		bool queValueChange (Rts2CondValue * old_value, int fakeState)
		{
			return old_value->queValueChange (fakeState);
		}

		virtual int processOption (int in_opt);
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
		void setInfoTime (struct tm *_date)
		{
			setInfoTime (mktime (_date));
		}

		/**
		 * Set infotime from time_t structure.
		 *
		 * @param _time Time_t holding time (in seconds from 1-1-1970) to which set info time.
		 */
		void setInfoTime (time_t _time)
		{
			info_time->setValueInteger (_time);
		}

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

	public:
		/**
		 * Construct daemon.
		 *
		 * @param _argc         Number of arguments.
		 * @param _argv         Arguments values.
		 * @param _init_state   Initial state.
		 */
		Rts2Daemon (int in_argc, char **in_argv, int _init_state = 0);

		virtual ~ Rts2Daemon (void);
		virtual int run ();

		/**
		 * Init daemon.
		 * This is call to init daemon. It calls @see Rts2Daemon::init and @see Rts2Daemon::initValues
		 * functions to complete daemon initialization.
		 */
		void initDaemon ();

		void setIdleInfoInterval (time_t interval)
		{
			idleInfoInterval = interval;
			setTimeoutMin ((long int) interval * USEC_SEC);
		}

		/**
		 * Updates info_time to current time.
		 */
		void updateInfoTime ()
		{
			info_time->setValueDouble (getNow ());
		}

		virtual void forkedInstance ();
		virtual void sendMessage (messageType_t in_messageType,
			const char *in_messageString);
		virtual void centraldConnRunning (Rts2Conn *conn);
		virtual void centraldConnBroken (Rts2Conn *conn);

		virtual int baseInfo ();
		int baseInfo (Rts2Conn * conn);
		int sendBaseInfo (Rts2Conn * conn);

		virtual int info ();
		int info (Rts2Conn * conn);
		int infoAll ();
		void constInfoAll ();
		int sendInfo (Rts2Conn * conn);

		int sendMetaInfo (Rts2Conn * conn);

		virtual int setValue (Rts2Conn * conn, bool overwriteSaved);

		// state management functions
	protected:
		/**
		 * Called to set new state value
		 */
		void setState (int new_state, const char *description);

		/**
		 * Called when state of the device is changed.
		 *
		 * @param new_state   New device state.
		 * @param old_state   Old device state.
		 * @param description Text description of state change.
		 */
		virtual void stateChanged (int new_state, int old_state, const char *description);

		/**
		 * Called from idle loop after HUP signal occured.
		 *
		 * This is most probably callback you needed for handling HUP signal.
		 * Handling HUP signal when it occurs can be rather dangerous, as it might
		 * reloacte memory location - if you read pointer before HUP signal and use
		 * it after HUP signal, RTS2 does not guarantee that it will be still valid.
		 */
		virtual void signaledHUP ();
	public:
		/**
		 * Called when state is changed.
		 */
		void maskState (int state_mask, int new_state, const char *description = NULL);

		/**
		 * Return full device state. You are then responsible
		 * to use approproate mask defined in status.h to extract
		 * from state information that you want.
		 *
		 * @return Block state.
		 */
		int getState ()
		{
			return state;
		}

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
		 */
		void setWeatherState (bool good_weather)
		{
			if (good_weather)
				maskState (WEATHER_MASK, GOOD_WEATHER, "weather set to good");
			else
				maskState (WEATHER_MASK, BAD_WEATHER, "weather set to bad");
		}

		virtual void sigHUP (int sig);
};
#endif							 /* ! __RTS2_DAEMON__ */
