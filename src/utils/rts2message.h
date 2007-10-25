#ifndef __RTS2_MESSAGE__
#define __RTS2_MESSAGE__

#include "message.h"

#include <sys/time.h>
#include <string>
#include <fstream>

class Rts2Message
{
	protected:
		struct timeval messageTime;
		std::string messageOName;
		messageType_t messageType;
		std::string messageString;

	public:
		Rts2Message (const struct timeval &in_messageTime,
			std::string in_messageOName, messageType_t in_messageType,
			std::string in_messageString);
		Rts2Message (const struct timeval &in_messageTime,
			std::string in_messageOName, int in_messageType,
			std::string in_messageString);
		Rts2Message (const char *in_messageOName, messageType_t in_messageType,
			const char *in_messageString);
		virtual ~ Rts2Message (void);

		std::string toConn ();
		std::string toString ();

		const char *getMessageOName ()
		{
			return messageOName.c_str ();
		}

		const char *getMessageString ()
		{
			return messageString.c_str ();
		}

		bool passMask (int in_mask)
		{
			return (((int) messageType) & in_mask);
		}

		bool isNotDebug ()
		{
			return (!(messageType & MESSAGE_DEBUG));
		}

		messageType_t getType ()
		{
			return messageType;
		}

		inline double getMessageTime ();
};

std::ofstream & operator << (std::ofstream & _of, Rts2Message & msg);
#endif							 /* ! __RTS2_MESSAGE__ */
