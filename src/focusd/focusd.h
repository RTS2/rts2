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

#include "../utils/device.h"

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
class Focusd:public rts2core::Device
{
	public:
		Focusd (int argc, char **argv);

		void checkState ();
		int setPosition (float num);
		int autoFocus (Rts2Conn * conn);

		float getPosition ()
		{
			return position->getValueFloat ();
		}

		virtual int scriptEnds ();

		virtual int commandAuthorized (Rts2Conn * conn);

	protected:
		std::string focType;

		rts2core::ValueFloat *position;
		rts2core::ValueFloat *target;
		rts2core::ValueFloat *temperature;

		rts2core::ValueFloat *defaultPosition;
		rts2core::ValueFloat *focusingOffset;
		rts2core::ValueFloat *tempOffset;

		virtual int processOption (int in_opt);

		virtual int setTo (float num) = 0;
		virtual float tcOffset () = 0;

		virtual int isFocusing ();
		virtual int endFocusing ();

		virtual bool isAtStartPosition () = 0;

		virtual int setValue (rts2core::Value * old_value, rts2core::Value * new_value);

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
	private:
		time_t focusTimeout;
};

}
#endif
