/* 
 * Message events triggering infrastructure.
 * Copyright (C) 2010 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_MESSAGEEVENTS__
#define __RTS2_MESSAGEEVENTS__

#include "emailaction.h"
#include "message.h"

namespace rts2xmlrpc
{
/**
 * Class for message event handling.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class MessageEvent
{
	public:
		MessageEvent (XmlRpcd *_master, std::string _deviceName, int _type);
		virtual ~MessageEvent () {}

		bool isForMessage (rts2core::Message *message) { return deviceName == message->getMessageOName () && message->passMask (type); }

		/**
		 * Triggered when message with given type is received. Throws Errors on error.
		 */
		virtual void run (rts2core::Message *message) = 0;

	protected:
		XmlRpcd *master;
		ci_string deviceName;

	private:
		int type;
};

/**
 * Run command when message arrives.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class MessageCommand: public MessageEvent
{
	public:
		MessageCommand (XmlRpcd *_master, std::string _deviceName, int _type, std::string _commandName):MessageEvent (_master, _deviceName, _type)
		{
			commandName = _commandName;
		}

		virtual void run (rts2core::Message *msg);
	private:
		std::string commandName;
};

/**
 * Send email when message arrives.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class MessageEmail: public MessageEvent, public EmailAction
{
	public:
		MessageEmail (XmlRpcd *_master, std::string _deviceName, int _type):MessageEvent (_master, _deviceName, _type), EmailAction () {}

		virtual void run (rts2core::Message *message);
};

/**
 * Message triggers.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class MessageCommands:public std::list <MessageEvent *>
{
	public:
		MessageCommands () {}

		~MessageCommands ()
		{
			for (MessageCommands::iterator iter = begin (); iter != end (); iter++)
				delete (*iter);
		}
};

}

#endif /* !__RTS2_MESSAGEEVENTS__ */

