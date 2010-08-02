#ifndef __RTS2_MESSAGE_T__
#define __RTS2_MESSAGE_T__

typedef int messageType_t;

#define MESSAGE_CRITICAL                0x0010
#define MESSAGE_ERROR                   0x0001
#define MESSAGE_WARNING                 0x0002
#define MESSAGE_INFO                    0x0004
#define MESSAGE_DEBUG                   0x0008

#define MESSAGE_REPORTIT                0x1000

#define CRITICAL_TELESCOPE_FAILURE      0x0110
#define CRITICAL_DOME_FAILURE           0x0210
#define CRITICAL_CAMERA_FAILURE         0x0310
#define CRITICAL_REQUIRED_FAILURE       0x0410

#define MESSAGE_MASK_ALL                0xFFFF

#endif							 /* ! __RTS2_MESSAGE_T__ */
