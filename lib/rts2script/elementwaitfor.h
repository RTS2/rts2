/*
 * Wait for some event or for a number of seconds.
 * Copyright (C) 2007-2009 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_SWAITFOR__
#define __RTS2_WAITFOR__

#include "rts2script/script.h"

namespace rts2script
{

/**
 * Used for waitfor command. Idle call returns NEXT_COMMAND_KEEP when target
 * value is not reached, and NEXT_COMMAND_NEXT when target value is reached.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class ElementWaitFor:public Element
{
	public:
		ElementWaitFor (Script * _script, const char *new_device, char *_valueName, double _value, double _range);
		virtual int defnextCommand (rts2core::DevClient * client, rts2core::Command ** new_command, char new_device[DEVICE_NAME_SIZE]);
		virtual int idle ();

		virtual void printScript (std::ostream &os) { os << COMMAND_WAITFOR << " " << valueName << " " << tarval << " " << range; }
	private:
		std::string valueName;
		double tarval;
		double range;
};

/**
 * Used for sleep command. Return NEXT_COMMAND_KEEP in idle call while command
 * should be keeped, and NEXT_COMMAND_NEXT when timeout expires.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class ElementSleep:public Element
{
	public:
		ElementSleep (Script * _script, double _sec):Element (_script) { sec = _sec; }
		virtual int defnextCommand (rts2core::DevClient * client, rts2core::Command ** new_command, char new_device[DEVICE_NAME_SIZE]);
		virtual int idle ();

		virtual void printScript (std::ostream &os) { os << COMMAND_SLEEP " " << sec; }
	private:
		double sec;
};

/**
 * Wait for camera to go into idle state.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class ElementWaitForIdle:public Element
{
	public:
		ElementWaitForIdle (Script *_script):Element (_script) {}
		virtual int defnextCommand (rts2core::DevClient * client, rts2core::Command ** new_command, char new_device[DEVICE_NAME_SIZE]);

		virtual void printScript (std::ostream &os) { os << COMMAND_WAIT_FOR_IDLE; }
};

}
#endif							 /* !__RTS2_WAITFOR__ */
