#ifndef __RTS2_MESSAGEDB__
#define __RTS2_MESSAGEDB__

#include "../utils/rts2message.h"

// class to save/load event to DB

class Rts2MessageDB:public Rts2Message
{
	public:
		Rts2MessageDB (Rts2Message & msg):Rts2Message (msg)
		{
		}
		Rts2MessageDB (const struct timeval &in_messageTime,
			std::string in_messageOName, messageType_t in_messageType,
			std::string in_messageString);
		virtual ~ Rts2MessageDB (void);
		void insertDB ();
};
#endif							 /* ! __RTS2_MESSAGEDB__ */
