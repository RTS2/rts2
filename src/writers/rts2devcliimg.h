#ifndef __RTS2_DEVCLIENT_IMG__
#define __RTS2_DEVCLIENT_IMG__

#include "../utils/rts2object.h"
#include "../utils/rts2devclient.h"
#include "../utils/rts2dataconn.h"
#include "../utils/rts2command.h"

#include "rts2image.h"

#include <libnova/libnova.h>

/**************************************
 *
 * Defines client descendants capable to stream themselves
 * to an Rts2Image.
 * 
 *************************************/

class Rts2DevClientCameraImage:public Rts2DevClientCamera
{
private:
  void writeFilter ();
protected:
  int isExposing;
  // we have to allocate that field as soon as we get the knowledge of
  // camera chip numbers..
  Rts2Image *images;
  int chipNumbers;
  int activeTargetId;
  int saveImage;

  // some camera characteristics..
  double xplate;
  double yplate;
  double xoa;
  double yoa;
  double ter_xoa;
  double ter_yoa;
  double config_rotang;
  int flip;
  char filter[200];

  float exposureTime;
  exposureType exposureT;
  int exposureChip;
  int exposureCount;		// -1 means exposure forewer

  virtual void queExposure ();
  virtual void exposureStarted ();
  virtual void exposureEnd ();
  virtual void readoutEnd ();
public:
    Rts2DevClientCameraImage (Rts2Conn * in_connection);
    virtual ~ Rts2DevClientCameraImage (void);
  virtual void postEvent (Rts2Event * event);
  virtual void dataReceived (Rts2ClientTCPDataConn * dataConn);
  virtual Rts2Image *createImage (const struct timeval *expStart);
  virtual void processImage (Rts2Image * image);
  virtual void exposureFailed (int status);

  void setSaveImage (int in_saveImage)
  {
    saveImage = in_saveImage;
  }
};

class Rts2DevClientTelescopeImage:public Rts2DevClientTelescope
{
public:
  Rts2DevClientTelescopeImage (Rts2Conn * in_connection);
  virtual void postEvent (Rts2Event * event);
  void getEqu (struct ln_equ_posn *tel);
  void getEquTel (struct ln_equ_posn *tel);
  void getEquTar (struct ln_equ_posn *tar);
  void getAltAz (struct ln_hrz_posn *hrz);
  void getObs (struct ln_lnlat_posn *obs);
  double getLocalSiderealDeg ();
  double getDistance (struct ln_equ_posn *in_pos);
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

class Rts2DevClientMirrorImage:public Rts2DevClientMirror
{
public:
  Rts2DevClientMirrorImage (Rts2Conn * in_connection);
  virtual void postEvent (Rts2Event * event);
};

class Rts2CommandQueImage:public Rts2Command
{
public:
  Rts2CommandQueImage (Rts2Block * in_owner, Rts2Image * image);
};

class Rts2CommandQueObs:public Rts2Command
{
public:
  Rts2CommandQueObs (Rts2Block * in_owner, int in_obsId);
};

#endif /* !__RTS2_DEVCLIENT_IMG__ */
