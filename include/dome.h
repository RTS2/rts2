/* 
 * Dome driver skeleton.
 * Copyright (C) 2005-2008 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_DOME__
#define __RTS2_DOME__

#include "device.h"

#define DEF_WEATHER_TIMEOUT 10

/**
 * Dome, copulas and roof controllers.
 */
namespace rts2dome {

/**
 * Skeleton for dome control. Provides pure virtual functions,
 * which needs to be implemented in order to operate roof.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Dome:public rts2core::Device
{
	public:
		Dome (int argc, char **argv, int in_device_type = DEVICE_TYPE_DOME);
		virtual ~Dome ();

		virtual void changeMasterState (int old_state, int new_state);

		/**
		 * Increases ignore timeout by given amount of seconds.
		 *
		 * @param _ignore_time  Seconds by which a timeout will be increased.
		 */ 
		void setIgnoreTimeout (time_t _ignore_time);

		/**
		 * Returns true if weather status (BAD_WEATHER system-wide flag) should be ignored.
		 */
		virtual bool getIgnoreMeteo ();

		void setWeatherTimeout (time_t wait_time, const char *msg);

		double getNextOpen () { return nextGoodWeather->getValueDouble (); }

		virtual int commandAuthorized (rts2core::Connection * conn);

		virtual int ready () { return 0; }

	protected:
		virtual int processOption (int in_opt);

		virtual int init ();

		virtual void cancelPriorityOperations ()
		{
			// we don't want to get back to not-moving state if we were moving..so we don't request to reset our state
		}

		// functions to operate dome
		// they call protected functions, which does the job
		int domeOpenStart ();

		int domeCloseStart ();

		/**
		 * Checks if weather is acceptable for observing.
		 *
		 * @return True if weather is acceptable, otherwise false.
		 */
		virtual bool isGoodWeather ();

		/**
		 * Open dome. Called either for open command, or when system
		 * transitioned to on state.
		 *
		 * @return 0 on success, -1 on error.
		 */
		virtual int startOpen () = 0;

		/**
		 * Check if dome is opened.
		 *
		 * @return -2 if dome is opened, -1 if there was an error, >=0 is timeout in miliseconds till
		 * next isOpened call.
		 */
		virtual long isOpened () = 0;

		/**
		 * Called when dome is fully opened. Can be used to turn off compressors used
		 * to open dome etc..
		 *
		 * @return -1 on error, 0 on success.
		 */
		virtual int endOpen () = 0;

		/**
		 * Called when dome needs to be closed. Should start dome
		 * closing sequence.
		 *
		 * @return -1 on error, 0 on success, 1 if dome will close (trigger for close) after equipment is stowed
		 */
		virtual int startClose () = 0;

		/**
		 * Called to check if dome is closed. It is called also outside
		 * of the closing sequence, to check if dome is closed when bad
		 * weather arrives. When implemented correctly, it should check
		 * state of dome end switches, and return proper values.
		 *
		 * @return -2 if dome is closed, -1 if there was an error, >=0 is timeout in miliseconds till
		 * next isClosed call (when dome is closing).
		 */
		virtual long isClosed () = 0;

		/**
		 * Called after dome is closed. Can turn of various devices
		 * used to close dome,..
		 *
		 * @return -1 on error, 0 on sucess.
		 */
		virtual int endClose () = 0;

		// called when dome passed some states..
		virtual int observing ();
		virtual int standby ();
		virtual int off ();

		int setMasterStandby ();
		int setMasterOn ();

		virtual int idle ();

	private:
		// time for which weather will be ignored - usefull for manual override of
		// dome operations
		rts2core::ValueTime *ignoreTimeout;

		rts2core::ValueBool *weatherOpensDome;
		rts2core::ValueTime *nextGoodWeather;

		// call isOpened and isClosed and decide what to do..
		int checkOpening ();

		int closeDomeWeather ();

		rts2core::ValueString *stateMaster;
};

}
#endif	 // ! __RTS2_DOME__
