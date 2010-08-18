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
#include "timelog.h"

#include <vector>

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
		MessageDB (double in_messageTime, const char *in_messageOName, messageType_t in_messageType, const char *in_messageString);
		virtual ~ MessageDB (void);
		void insertDB ();
};

/**
 * Set of messages retrieved from database for given period.
 *
 * @author Petr KUbanek <petr@kubanek.net>
 */
class MessageSet:public std::vector <MessageDB>, public TimeLog
{
	public:
		MessageSet () {}
		/**
		 * Retrieve messages within given period.
		 *
		 * @param from       start of the period
		 * @param to         end of the period
		 * @param type_mask  type mask to filter messages
		 *
		 * @throw SqlError
		 */
		void load (double from, double to, int type_mask);

		virtual void rewind () { timeLogIter = begin (); }

		virtual double getNextCtime ();
		virtual void printUntil (double time, std::ostream &os);
	private:
		std::vector <MessageDB>::iterator timeLogIter;
};

}

#endif							 /* ! __RTS2_MESSAGEDB__ */
