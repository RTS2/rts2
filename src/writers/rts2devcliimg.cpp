#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "rts2devcliimg.h"

Rts2DevClientCameraImage::Rts2DevClientCameraImage (Rts2Conn * in_connection)
{

}

void
Rts2DevClientCameraImage::postEvent (Rts2Event * event)
{
  switch (event->getType ())
    {
    case EVENT_SET_TARGET:
      break;
    }
}

Rts2DevClientTelescopeImage::Rts2DevClientTelescopeImage (Rts2Conn *
							  in_connection)
{

}

void
Rts2DevClientTelescopeImage::postEvent (Rts2Event * event)
{

}

Rts2DevClientDomeImage::Rts2DevClientDomeImage (Rts2Conn * in_connection);

void
Rts2DevClientDomeImage::postEvent (Rts2Event * event)
{

}

Rts2DevClientFocusImage::Rts2DevClientFocusImage (Rts2Conn * in_connection)
{

}

void
Rts2DevClientFocusImage::postEvent (Rts2Event * event)
{

}
