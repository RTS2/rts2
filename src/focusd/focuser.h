/**
 * Copyright (C) 2005-2008 Petr Kubanek <petr@kubanek.net>
 * Copyright (C) 2005-2007 Stanislav Vitek
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __RTS2_FOCUSD_CPP__
#define __RTS2_FOCUSD_CPP__

#include "../utils/rts2device.h"

/**
 * Abstract base class for focuser.
 *
 * Defines interface for fouser, put on various variables etc..
 *
 * @author Petr Kubanek <petr@kubanek.net>
 * @author Stanisla Vitek
 */
class Rts2DevFocuser:public Rts2Device
{
	private:
		time_t focusTimeout;
		int homePos;
	protected:
		std::string focType;
		Rts2ValueInteger *focPos;
		Rts2ValueInteger *focTarget;
		Rts2ValueFloat *focTemp;
		// minimal steps/sec count; 5 sec will be added to top it
		int focStepSec;
		int startPosition;

		virtual int processOption (int in_opt);

		virtual int stepOut (int num)
		{
			return -1;
		}
		// set to given number
		// default to use stepOut function
		virtual int setTo (int num);
		virtual int home ();

		virtual int isFocusing ();
		virtual int endFocusing ();

		void setFocusTimeout (int timeout);

		virtual bool isAtStartPosition () = 0;
		int checkStartPosition ();

		virtual int setValue (Rts2Value * old_value, Rts2Value * new_value);

		void createFocTemp ()
		{
			createValue (focTemp, "FOC_TEMP", "focuser temperature");
		}

		/**
		 * Returns focuser target value.
		 *
		 * @return Focuser target position.
		 */
		int getFocTarget ()
		{
			return focTarget->getValueInteger ();
		}

		/**
		 * Set focuser target position.
		 *
		 * @param _focTarget New target position.
		 */
		void setFocTarget (int _focTarget)
		{
			focTarget->setValueInteger (_focTarget);
		}

		// callback functions from focuser connection
		virtual int initValues ();
		virtual int idle ();

	public:
		Rts2DevFocuser (int argc, char **argv);

		/**
		 * TODO remove
		 */
		virtual int setSwitch (int switch_num, int new_state)
		{
			return -1;
		}

		void checkState ();
		int stepOut (Rts2Conn * conn, int num);
		int setTo (Rts2Conn * conn, int num);
		int home (Rts2Conn * conn);
		int autoFocus (Rts2Conn * conn);

		int getFocPos ()
		{
			return focPos->getValueInteger ();
		}

		virtual int commandAuthorized (Rts2Conn * conn);
};
#endif
