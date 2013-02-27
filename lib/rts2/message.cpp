/* 
 * Configuration file read routines.
 * Copyright (C) 2006-2010 Petr Kubanek <petr@kubanek.net>
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

#include "message.h"
#include "block.h"

#include <sys/time.h>
#include <time.h>
#include <sstream>

using namespace rts2core;

Message::Message (const struct timeval &in_messageTime, std::string in_messageOName, messageType_t in_messageType, std::string in_messageString)
{
	messageTime = in_messageTime;
	messageOName = in_messageOName;
	messageType = in_messageType;
	messageString = in_messageString;
}

Message::Message (const char *in_messageOName, messageType_t in_messageType, const char *in_messageString)
{
	gettimeofday (&messageTime, NULL);
	messageOName = std::string (in_messageOName);
	messageType = in_messageType;
	messageString = std::string (in_messageString);
}

Message::Message (double in_messageTime, const char *in_messageOName, messageType_t in_messageType, const char *in_messageString)
{
#ifdef RTS2_HAVE_TRUNC
	messageTime.tv_sec = trunc (in_messageTime);
#else
	messageTime.tv_sec = (time_t) floor (in_messageTime);
#endif
	messageTime.tv_usec = USEC_SEC * (in_messageTime - messageTime.tv_sec);
	messageOName = std::string (in_messageOName);
	messageType = in_messageType;
	messageString = std::string (in_messageString);
}

Message::~Message ()
{
}

std::string Message::toConn ()
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

const std::string Message::getMessageString ()
{
	std::ostringstream os;
	switch (getID ())
	{
		case INFO_OBSERVATION_SLEW:
			os << expandString ("slew to observation #$0 of target #$1");
			break;
		case INFO_OBSERVATION_STARTED:
			os << expandString ("observation #$0 of target #$1 started");
			break;
		case INFO_OBSERVATION_END:
			os << expandString ("observation #$0 of target #$1 ended");
			break;
		case INFO_OBSERVATION_INTERRUPTED:
			os << expandString ("observation $0 interrupted");
			break;
		default:
			return messageString;
	}

	return os.str ();
}

const std::string Message::getMessageArg (int n)
{
	size_t ibeg = 0;
	if (n > 0)
	{
		while (n > 0)
		{
			ibeg = messageString.find (' ', ibeg);
			if (ibeg == std::string::npos)
				return std::string ();
			n--;
		}
		ibeg++;
	}
	return messageString.substr (ibeg, messageString.find (' ', ibeg + 1));
}

int Message::getMessageArgInt (int n)
{
	return atoi (getMessageArg (n).c_str ());
}

const std::string Message::expandString (const char *str)
{
	std::string ret;
	while (*str != '\0')
	{
		if (*str == '$')
		{
			int arg = 0;
			str++;
			while (*str != '\0' && isdigit (*str))
			{
				arg = 10 * arg + (*str - '0');
				str++;
			}
			ret += getMessageArg (arg);
		}
		else
		{
			ret += *str;
			str++;
		}
	}
	return ret;
}
