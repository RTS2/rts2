#include "rts2block.h"
#include "rts2message.h"

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
  os << PROTO_MESSAGE
    << " " << messageTime.tv_sec
    << " " << messageTime.tv_usec
    << " " << messageOName << " " << messageType << " " << messageString;
  return os.str ();
}
