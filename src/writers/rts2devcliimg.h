#ifndef __RTS2_DEVCLIENT_IMG__
#define __RTS2_DEVCLIENT_IMG__

#include "../utils/rts2object.h"
#include "../utils/rts2devclient.h"
#include "../utils/rts2dataconn.h"
#include "../utils/rts2command.h"

#include "rts2image.h"

/**************************************
 *
 * Defines client descendants capable to stream themselves
 * to an Rts2Image.
 * 
 *************************************/

class Rts2DevClientCameraImage:public Rts2DevClientCamera
{
private:
  int isExposing;
protected:
  // we have to allocate that field as soon as we get the knowledge of
  // camera chip numbers..
    Rts2Image * images;
  int chipNumbers;
  int activeTargetId;
  int saveImage;

  float exposureTime;
  exposureType exposureT;
  int exposureChip;
  int exposureEnabled;

  void queExposure ();
public:
    Rts2DevClientCameraImage (Rts2Conn * in_connection);
  virtual void postEvent (Rts2Event * event);
  virtual void dataReceived (Rts2ClientTCPDataConn * dataConn);
  virtual Rts2Image *createImage (const struct timeval *expStart);
  virtual void processImage (Rts2Image * image);
  virtual void stateChanged (Rts2ServerState * state);
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

class Rts2CommandQueImage:public Rts2Command
{
public:
  Rts2CommandQueImage (Rts2Block * in_owner, Rts2Image * image);
};

#endif /* !__RTS2_DEVCLIENT_IMG__ */
