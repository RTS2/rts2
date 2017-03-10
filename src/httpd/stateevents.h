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

#include "emailaction.h"
#include "connection.h"

namespace rts2xmlrpc
{

/**
 * Abstract class for state actions, parent of all StateChange* classes.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class StateChange
{
	public:
		StateChange (std::string _deviceName, int _changeMask, int _newStateValue)
		{
			deviceName = _deviceName;
			changeMask = _changeMask;
			newStateValue = _newStateValue;
		}

		virtual ~StateChange () {}

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

		virtual void run (HttpD *_master, rts2core::Rts2Connection *_conn, double validTime) = 0;

	protected:
		int getChangeMask () { return changeMask; };

	private:
		std::string deviceName;
		int changeMask;
		int newStateValue;
};


/**
 * Class which records state change to database (or standard output).
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class StateChangeRecord: public StateChange
{
	private:
		int dbValueId;
	public:
		StateChangeRecord (std::string _deviceName, int _changeMask, int _newStateValue):StateChange (_deviceName, _changeMask, _newStateValue)
		{
			dbValueId = -1;
		}

		virtual void run (HttpD *_master, rts2core::Rts2Connection *_conn, double validTime);
};


/**
 * Class for mapping between device names, state bit masks and 
 * command to be executed.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class StateChangeCommand: public StateChange
{
	public:
		StateChangeCommand (std::string _deviceName, int _changeMask, int _newStateValue, std::string _commandName):StateChange (_deviceName, _changeMask, _newStateValue)
		{
			commandName = _commandName;
		}

		virtual void run (HttpD *_master, rts2core::Rts2Connection *_conn, double validTime);

	private:
		std::string deviceName;
		int changeMask;
		int newStateValue;
		std::string commandName;
};

/**
 * Send email to given recepient.
 */
class StateChangeEmail: public StateChange, public EmailAction
{
	public:
		StateChangeEmail (std::string _deviceName, int _changeMask, int _newStateValue):StateChange (_deviceName, _changeMask, _newStateValue), EmailAction ()
		{
		}

		virtual void run (HttpD *_master, rts2core::Rts2Connection *_conn, double validTime)
		{
			EmailAction::run (_master);
		}
};

/**
 * Holds a list of StateChangeCommands.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class StateCommands:public std::list <StateChange *>
{
	public:
		StateCommands ()
		{
		}

		~StateCommands ()
		{
			for (StateCommands::iterator iter = begin (); iter != end (); iter++)
				delete (*iter);
		}
};

}

#endif /* !__RTS2__STATEEVENTS__ */
