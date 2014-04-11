/*
 * Copyright (C) 2005-2013 Petr Kubanek <petr@kubanek.net>
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

#include "device.h"

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

		/**
		 * Provides estimate for the offseting duration, based on
		 * offseting speed.
		 *
		 * @return Number of seconds to move for the required steps number.
		 */
		float estimateOffsetDuration (float offset)
		{
			if (speed->getValueDouble () > 0)
				return offset / speed->getValueDouble ();
			return 0;
		}
		int setPosition (float num);

		float getPosition ()
		{
			return position->getValueFloat ();
		}

		virtual int scriptEnds ();

		virtual int commandAuthorized (rts2core::Connection * conn);

	protected:
		std::string focType;

		rts2core::ValueDouble *position;
		rts2core::ValueDoubleMinMax *target;
		rts2core::ValueFloat *temperature;

		rts2core::ValueDoubleMinMax *defaultPosition;
		rts2core::ValueDoubleMinMax *filterOffset;
		rts2core::ValueDoubleMinMax *focusingOffset;
		rts2core::ValueDoubleMinMax *tempOffset;

		rts2core::ValueDouble *speed;

		virtual int processOption (int in_opt);

		virtual int setTo (double num) = 0;
		virtual double tcOffset () = 0;

		/**
		 * Reports state of the focuser. Called during position change to query for progress in focusing.
		 *
		 * @return >= 0 if still focusing, -1 on error, -2 when focusing finished.
		 */
		virtual int isFocusing ();
		virtual int endFocusing ();

		double getFocusMin () { return target->getMin (); }
		double getFocusMax () { return target->getMax (); }

		void setFocusExtent (double foc_min, double foc_max);
		void updateOffsetsExtent ();

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
		double getTarget ()
		{
			return target->getValueDouble ();
		}

		// callback functions from focuser connection
		virtual int initValues ();
		virtual int idle ();

		/**
		 * Set focuser movement speed. Speed is specified in number of steps per second.
		 */
		void setSpeed (double _speed) { speed->setValueDouble (_speed); }
	private:
		time_t focusTimeout;
};

}
#endif
