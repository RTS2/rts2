/* 
 * State changes triggering infrastructure. 
 * Copyright (C) 2009 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2__STATEEVENTS__
#define __RTS2__STATEEVENTS__

#include <string>
#include <list>

namespace rts2xmlrpc
{

/**
 * Class for mapping between device names, state bit masks and 
 * command to be executed.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class StateChangeCommand
{
	private:
		std::string deviceName;
		int changeMask;
		int newStateValue;
		std::string commandName;

	public:
		StateChangeCommand (std::string _deviceName, int _changeMask, int _newStateValue, std::string _commandName)
		{
			deviceName = _deviceName;
			changeMask = _changeMask;
			newStateValue = _newStateValue;
			commandName = _commandName;
		}

		/**
		 * Returns command associated with this state change.
		 */
		std::string getCommand ()
		{
			return commandName;
		}

		/**
		 * Returns true if command should be executed on state change between two states.
		 *
		 * @param oldState Old device state.
		 * @param newState New device state.
		 *
		 * @return True if command should be executed.
		 */
		bool executeOnStateChange (int oldState, int newState)
		{
			if (changeMask < 0)
				return (newStateValue < 0 || newState == newStateValue);
			return (oldState & changeMask) != (newState & changeMask)
				&& (newStateValue < 0 || ((newState & changeMask) == newStateValue));
		}

		/**
		 * Returns true if this entry belongs to given device.
		 */
		bool isForDevice (std::string _deviceName, int _deviceType)
		{
			return deviceName == _deviceName;
		}
};


/**
 * Holds a list of StateChangeCommands.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class StateCommands:public std::list <StateChangeCommand>
{
	public:
		StateCommands ()
		{
		}

		~StateCommands ()
		{
		}
};

}

#endif /* !__RTS2__STATEEVENTS__ */
