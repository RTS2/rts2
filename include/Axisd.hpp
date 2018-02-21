/* 
 * Axis controller.
 * Copyright (C) 2018 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_AXISD__
#define __RTS2_AXISD__

#include "device.h"

/**
 * Abstract axis. Has notion of center (=0mm) position and scale (counts/mm).
 */
namespace rts2axisd
{

/**
 * Class for single axis control.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Axis:public rts2core::Device
{
	public:
		Axis (int argc, char **argv, const char *sn = "AX1");
		virtual ~Axis (void);

	protected:
		int setValue (rts2core::Value * old_value, rts2core::Value * new_value);

		/**
		 * Start movement to new target value.
		 */
		virtual int moveTo (int tcounts) = 0;
		int mmToCounts (double mm);
		double countsToMm (int counts);

		int getTarget() { return target->getValueInteger(); }
	
	private:
		rts2core::ValueIntegerMinMax *target;
		rts2core::ValueDouble *centerPoint;
		rts2core::ValueDouble *scale;
};

};

#endif							 /* !__RTS2_AXISD__ */
