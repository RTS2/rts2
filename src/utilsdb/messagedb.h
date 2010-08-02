/*
 * Class for database messages manipulation.
 * Copyright (C) 2007-2010 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_MESSAGEDB__
#define __RTS2_MESSAGEDB__

#include "../utils/rts2message.h"

namespace rts2db
{

/**
 * Class for message database manipulation.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class MessageDB:public Rts2Message
{
	public:
		MessageDB (Rts2Message & msg):Rts2Message (msg) {}
		MessageDB (const struct timeval &in_messageTime, std::string in_messageOName, messageType_t in_messageType, std::string in_messageString);
		virtual ~ MessageDB (void);
		void insertDB ();
};

}

#endif							 /* ! __RTS2_MESSAGEDB__ */
