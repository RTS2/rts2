#ifndef __RTS2_LOGSTREAM__
#define __RTS2_LOGSTREAM__

#include "rts2device.h"

#include <sstream>

class Rts2Device;

class Rts2LogStream
{
private:
  Rts2Device * masterDevice;
  messageType_t messageType;
    std::ostringstream ls;
public:

    Rts2LogStream (Rts2Device * in_master, messageType_t in_type)
  {
    masterDevice = in_master;
    messageType = in_type;
  }

  Rts2LogStream (Rts2LogStream & in_logStream)
  {
    masterDevice = in_logStream.masterDevice;
    messageType = in_logStream.messageType;
  }

  Rts2LogStream & operator << (Rts2LogStream & (*func) (Rts2LogStream &))
  {
    return func (*this);
  }

  template < typename _charT > Rts2LogStream & operator << (_charT value)
  {
    ls << value;
    return *this;
  }

  inline void sendLog ();
};

Rts2LogStream & sendLog (Rts2LogStream & _ls);

#endif /* ! __RTS2_LOGSTREAM__ */
