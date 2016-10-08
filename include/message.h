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

#ifndef __RTS2_MESSAGE__
#define __RTS2_MESSAGE__

#include <status.h>

#include <sys/time.h>

#ifdef RTS2_HAVE_STDINT_H
#include <stdint.h>
#endif

#include <string>
#include <fstream>

#include "timestamp.h"

typedef uint32_t messageType_t;

#define MESSAGE_LEVEL_MASK              0x00001F

#define MESSAGE_ERROR                   0x000001
#define MESSAGE_WARNING                 0x000002
#define MESSAGE_INFO                    0x000004
#define MESSAGE_DEBUG                   0x000008

#define MESSAGE_CRITICAL                0x000010

#define MESSAGE_REPORTIT                0x100000

#define MESSAGE_ID_MASK                 0x0FFF00

#define CRITICAL_TELESCOPE_FAILURE      0x000110
#define CRITICAL_DOME_FAILURE           0x000210
#define CRITICAL_CAMERA_FAILURE         0x000310
#define CRITICAL_REQUIRED_FAILURE       0x000410

// INFO_OBSERVATION_xx messages contains as arguments observation ID and target ID
#define INFO_OBSERVATION_SLEW           0x000500
#define INFO_OBSERVATION_STARTED        0x000600
#define INFO_OBSERVATION_END            0x000700
#define INFO_OBSERVATION_INTERRUPTED    0x000800
#define INFO_OBSERVATION_LOOP           0x000900

// INFO_MOUNT_xx messages logs mount movements
#define INFO_MOUNT_SLEW_START           0x001000
#define INFO_MOUNT_SLEW_ALTAZ           0x001100
#define INFO_MOUNT_SLEW_END             0x001200
#define DEBUG_MOUNT_TRACKING_LOG        0x001300

#define MESSAGE_MASK_ALL                0xFFFFFF

namespace rts2core
{

/**
 * Holds message, which can be passed through the system. Message contains
 * timestamp when it was generated, flags describing its severity, and message
 * text, which is free text.
 *
 *
 * @ingroup RTS2Block
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Message
{
	public:
		Message (const struct timeval &in_messageTime, std::string in_messageOName, messageType_t in_messageType, std::string in_messageString);

		Message (const char *in_messageOName, messageType_t in_messageType, const char *in_messageString);
		Message (double in_messageTime, const char *in_messageOName, messageType_t in_messageType, const char *in_messageString);

		virtual ~ Message (void);

		std::string toConn ();

		const char *getMessageOName () { return messageOName.c_str (); }

		const std::string getMessageString ();

		/**
		 * Returns n-th message argument, assuming arguments are separated with space.
		 *
		 * @param n argument index - 0 based
		 * @param f format - ' ', h (hours, for RA), d (degrees)
		 */
		const std::string getMessageArg (int n, char f);

		/**
		 * Returns n-th message argument as integer.
		 *
		 * @see Message::getMessageArg(int)
		 */
		int getMessageArgInt (int n);

		bool passMask (int in_mask) { return (((int) messageType) & in_mask); }

		/**
		 * Check if message is debug message.
		 *
		 * @return True if message is not debugging messages, and hence
		 * have to be stored in more permament location.
		 */
		bool isNotDebug () { return (!(messageType & MESSAGE_DEBUG)); }

		/**
		 * Returns message flags.
		 *
		 * @return Message flags.
		 */
		messageType_t getType () { return messageType; }

		messageType_t getLevel () { return messageType & MESSAGE_LEVEL_MASK; }

		/**
		 * Returns message number.
		 */
		messageType_t getID () { return messageType & MESSAGE_ID_MASK; }

		/**
		 * Return message type as string.
		 */
		const char* getTypeString ()
		{
			switch (getType ())
			{
				case MESSAGE_ERROR:
					return "error";
				case MESSAGE_WARNING:
					return "warning";
				case MESSAGE_INFO:
					return "info";
				case MESSAGE_DEBUG:
					return "debug";
				default:
					return "unknown";
			}
		}

		double getMessageTime () { return messageTime.tv_sec + (double) messageTime.tv_usec / USEC_SEC;	}

		time_t getMessageTimeSec () { return messageTime.tv_sec; }

		int getMessageTimeUSec () { return messageTime.tv_usec; }

		friend std::ostream & operator << (std::ostream & _of, Message & msg)
		{
			_of << Timestamp (&msg.messageTime) << " " << msg.messageOName << " " << msg.messageType << " " << msg.getMessageString ();
			return _of;
		}

		const std::string expandString (const char *str);

	protected:
		struct timeval messageTime;
		std::string messageOName;
		messageType_t messageType;
		std::string messageString;
};

}

#endif							 /* ! __RTS2_MESSAGE__ */
