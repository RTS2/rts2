#ifndef __RTS2_DEVCLIENT_IMG__
#define __RTS2_DEVCLIENT_IMG__

#include "../utils/rts2object.h"
#include "../utils/rts2block.h"
#include "../utils/rts2value.h"
#include "../utils/rts2devclient.h"

#include "rts2image.h"

#define EVENT_SET_TARGET
#define EVENT_WRITE_TO_IMAGE

/**************************************
 *
 * Defines client descendants capable to stream themselves
 * to an Rts2Image.
 * 
 *************************************/

class Rts2DevClientCameraImage:public Rts2DevClientCamera
{
protected:
  // we have to allocate that field as soon as we get the knowledge of
  // camera chip numbers..
  Rts2Image * images;
public:
  Rts2DevClientCameraImage (Rts2Conn * in_connection);
  virtual void postEvent (Rts2Event * event);
};

class Rts2DevClientTelescopeImage:public Rts2DevClientTelescope
{
public:
  Rts2DevClientTelescopeImage (Rts2Conn * in_connection);
  virtual void postEvent (Rts2Event * event);
};

class Rts2DevClientDomeImage:public Rts2DevClientDome
{
public:
  Rts2DevClientDomeImage (Rts2Conn * in_connection);
  virtual void postEvent (Rts2Event * event);
};

class Rts2DevClientFocusImage:public Rts2DevClientFocus
{
public:
  Rts2DevClientFocusImage (Rts2Conn * in_connection);
  virtual void postEvent (Rts2Event * event);
};

#endif /* !__RTS2_DEVCLIENT_IMG__ */
