/*
 * Copyright (C) 2005-2009 Petr Kubanek <petr@kubanek.net>
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
 * Focuser interface.
 */
namespace rts2focusd
{

/**
 * Abstract base class for focuser.
 *
 * Defines interface for fouser, put on various variables etc..
 *
 * @author Petr Kubanek <petr@kubanek.net>
 * @author Stanislav Vitek
 */
class Focusd:public Rts2Device
{
	private:
		time_t focusTimeout;
		int homePos;

		/**
		 * Update target value by given offset.
		 *
		 * @param off Offset
		 */
		int updatePosition (int off)
		{
			return setPosition (target->getValueInteger () + off);
		}

	protected:
		std::string focType;

		Rts2ValueInteger *position;
		Rts2ValueInteger *target;
		Rts2ValueFloat *temperature;

		Rts2ValueInteger *defaultPosition;
		Rts2ValueInteger *focusingOffset;
		Rts2ValueInteger *tempOffset;

		virtual int processOption (int in_opt);

		virtual int setTo (int num) = 0;

		virtual int home ();

		virtual int isFocusing ();
		virtual int endFocusing ();

		virtual bool isAtStartPosition () = 0;

		virtual int setValue (Rts2Value * old_value, Rts2Value * new_value);

		void createTemperature ()
		{
			createValue (temperature, "FOC_TEMP", "focuser temperature");
		}

		/**
		 * Returns focuser target value.
		 *
		 * @return Focuser target position.
		 */
		int getTarget ()
		{
			return target->getValueInteger ();
		}

		// callback functions from focuser connection
		virtual int initValues ();
		virtual int idle ();

	public:
		Focusd (int argc, char **argv);

		/**
		 * TODO remove
		 */
		virtual int setSwitch (int switch_num, int new_state)
		{
			return -1;
		}

		void checkState ();
		int setPosition (int num);
		int home (Rts2Conn * conn);
		int autoFocus (Rts2Conn * conn);

		int getPosition ()
		{
			return position->getValueInteger ();
		}

		virtual int scriptEnds ();

		virtual int commandAuthorized (Rts2Conn * conn);
};

};
#endif
