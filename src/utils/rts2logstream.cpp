#include "rts2app.h"
#include "rts2logstream.h"

void
Rts2LogStream::sendLog ()
{
  masterApp->sendMessage (messageType, ls.str ().c_str ());
}

Rts2LogStream & sendLog (Rts2LogStream & _ls)
{
  _ls.sendLog ();
  return _ls;
}
