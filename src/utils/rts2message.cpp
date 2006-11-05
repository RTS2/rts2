#include "rts2block.h"
#include "rts2message.h"

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

Rts2Message::~Rts2Message (void)
{
}

std::string Rts2Message::toConn ()
{
  std::ostringstream os;
  os << PROTO_MESSAGE
    << " " << (messageTime.tv_sec + messageTime.tv_usec / USEC_SEC)
    << " " << messageOName << " " << messageType << " " << messageString;
  return os.str ();
}
