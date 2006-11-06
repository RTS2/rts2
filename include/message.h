#ifndef __RTS2_MESSAGE_T__
#define __RTS2_MESSAGE_T__

typedef enum
{
  MESSAGE_ERROR = 0x01,
  MESSAGE_WARNING = 0x02,
  MESSAGE_INFO = 0x04,
  MESSAGE_DEBUG = 0x08
} messageType_t;

#endif /* ! __RTS2_MESSAGE_T__ */
