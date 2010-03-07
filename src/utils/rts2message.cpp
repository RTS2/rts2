/* 
 * Configuration file read routines.
 * Copyright (C) 2006-2007 Petr Kubanek <petr@kubanek.net>
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

#include "rts2message.h"
#include "timestamp.h"

#include <sys/time.h>
#include <time.h>
#include <sstream>

Rts2Message::Rts2Message (const struct timeval &in_messageTime,
std::string in_messageOName,
messageType_t in_messageType,
std::string in_messageString)
{
	messageTime = in_messageTime;
	messageOName = in_messageOName;
	messageType = in_messageType;
	messageString = in_messageString;
}


Rts2Message::Rts2Message (const struct timeval &in_messageTime,
std::string in_messageOName,
int in_messageType, std::string in_messageString)
{
	messageTime = in_messageTime;
	messageOName = in_messageOName;
	switch (in_messageType)
	{
		case MESSAGE_ERROR:
			messageType = MESSAGE_ERROR;
			break;
		case MESSAGE_WARNING:
			messageType = MESSAGE_WARNING;
			break;
		case MESSAGE_INFO:
			messageType = MESSAGE_INFO;
			break;
		case MESSAGE_DEBUG:
			messageType = MESSAGE_DEBUG;
			break;
		default:
			messageType = MESSAGE_DEBUG;
	}
	messageString = in_messageString;
}


Rts2Message::Rts2Message (const char *in_messageOName,
messageType_t in_messageType,
const char *in_messageString)
{
	gettimeofday (&messageTime, NULL);
	messageOName = std::string (in_messageOName);
	messageType = in_messageType;
	messageString = std::string (in_messageString);
}


Rts2Message::~Rts2Message (void)
{
}


std::string Rts2Message::toConn ()
{
	std::ostringstream os;
	// replace \r\n
	std::string msg = messageString;
	size_t pos;
	for (pos = msg.find_first_of ("\0"); pos != std::string::npos; pos = msg.find_first_of ("\0", pos))
	{
		msg.replace (pos, 1, "\\0");
	}
	for (pos = msg.find_first_of ("\r"); pos != std::string::npos; pos = msg.find_first_of ("\r", pos))
	{
		msg.replace (pos, 1, "\\r");
	}
	for (pos = msg.find_first_of ("\n"); pos != std::string::npos; pos = msg.find_first_of ("\n", pos))
	{
		msg.replace (pos, 1, "\\n");
	}

	os << PROTO_MESSAGE
		<< " " << messageTime.tv_sec
		<< " " << messageTime.tv_usec
		<< " " << messageOName << " " << messageType << " " << msg;
	return os.str ();
}


std::string Rts2Message::toString ()
{
	std::ostringstream os;
	os <<
		Timestamp (&messageTime)
		<< " " << messageOName << " " << messageType << " " << messageString;
	return os.str ();
}


std::ostream & operator << (std::ostream & _of, Rts2Message & msg)
{
	_of << msg.toString () << std::endl;
	return _of;
}
