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
#include "libnova_cpp.h"

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
			os << expandString ("slew to observation #$1 of target #$2");
			break;
		case INFO_OBSERVATION_STARTED:
			os << expandString ("observation #$1 of target #$2 started");
			break;
		case INFO_OBSERVATION_END:
			os << expandString ("observation #$1 of target #$2 ended");
			break;
		case INFO_OBSERVATION_INTERRUPTED:
			os << expandString ("observation #$1 of target #$2 interrupted");
			break;
		case INFO_OBSERVATION_LOOP:
			os << expandString ("starting new loop of observation #$1 on target #$2");
			break;
		case INFO_MOUNT_SLEW_START:
			os << expandString ("moving from $H1 $D2 (altaz $D5 $D6) to $H3 $D4 (altaz $D7 $D8)");
			break;
		case INFO_MOUNT_SLEW_END:
			os << expandString ("moved to $H1 $D2 requested $H3 $D4 target $H5 $D6");
			break;
		case DEBUG_MOUNT_TRACKING_LOG:
			os << expandString ("target $1 $2 precession $3 $4 nutation $5 $6 aberation $7 $8 refraction $9 model $10 $11 etar $12 $13 htar $14 $15");
			break;
		case DEBUG_MOUNT_TRACKING_SHORT_LOG:
			os << expandString ("target $1 $2 model $3 $4 etar $5 $6 htar $7 $8");
			break;
		case INFO_ROTATOR_OFFSET:
			os << expandString ("setting derotator offset to $1, PA offset to $2");
		default:
			return messageString;
	}

	return os.str ();
}

const std::string Message::getMessageArg (int n, char f)
{
	size_t ibeg = 0;
	if (n > 1)
	{
		n--;
		while (n > 0)
		{
			ibeg = messageString.find (' ', ibeg);
			if (ibeg == std::string::npos)
				return std::string ();
			n--;
			while (messageString[ibeg] == ' ')
				ibeg++;
		}
	}
	size_t iend = messageString.find (' ', ibeg + 1);

	std::string ret;

	if (iend == std::string::npos)
		ret = messageString.substr (ibeg);
	else
		ret = messageString.substr (ibeg, iend - ibeg);

	std::ostringstream os;

	switch (f)
	{
		case 'H':
			os << LibnovaRa (atof (ret.c_str ()));
			break;
		case 'D':
			os << LibnovaDeg (atof (ret.c_str ()));
			break;
		case 'A':
			os << LibnovaAA (atof (ret.c_str ()));
			break;
		default:
			return ret;
	}
	return os.str ();
}

int Message::getMessageArgInt (int n)
{
	return atoi (getMessageArg (n, ' ').c_str ());
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
			char format = '\0';

			while (*str != '\0' && !isdigit (*str))
			{
				format = *str;
				str++;
			}
			while (isdigit (*str))
			{
				arg = 10 * arg + (*str - '0');
				str++;
			}
			ret += getMessageArg (arg, format);
		}
		else
		{
			ret += *str;
			str++;
		}
	}
	return ret;
}
