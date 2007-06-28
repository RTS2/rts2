#ifndef __RTS2_DEVCLIENT_IMG__
#define __RTS2_DEVCLIENT_IMG__

#include "../utils/rts2object.h"
#include "../utils/rts2devclient.h"
#include "../utils/rts2dataconn.h"
#include "../utils/rts2command.h"

#include "rts2image.h"
#include "cameraimage.h"

#include <libnova/libnova.h>

typedef enum
{ IMAGE_DO_BASIC_PROCESSING, IMAGE_KEEP_COPY } imageProceRes;

/**
 * Defines client descendants capable to stream themselves
 * to an Rts2Image.
 * 
 */
class Rts2DevClientCameraImage:public Rts2DevClientCamera
{
private:
  void writeFilter ();
  bool isExposing;

protected:
    bool getIsExposing ()
  {
    return isExposing;
  }
  void setIsExposing (bool in_isExposing)
  {
    isExposing = in_isExposing;
  }
  // we have to allocate that field as soon as we get the knowledge of
  // camera chip numbers..
  CameraImages images;

  CameraImages::iterator getTopIter ()
  {
    if (images.begin () == images.end ())
      return images.begin ();
    return --images.end ();
  }

  CameraImage *getTop ()
  {
    if (images.begin () == images.end ())
      return NULL;
    return *(--images.end ());
  }

  Rts2Image *getTopImage ()
  {
    if (images.begin () == images.end ())
      return NULL;
    return (*(--images.end ()))->image;
  }

  void clearImages ()
  {
    for (CameraImages::iterator iter = images.begin (); iter != images.end ();
	 iter++)
      {
	delete (*iter);
      }
    images.clear ();
  }

  Rts2Image *setImage (Rts2Image * old_img, Rts2Image * new_image);

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
  char instrume[70];
  char telescop[70];
  char origin[70];

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
  virtual void beforeProcess (Rts2Image * image);
  /**
   * This function carries image processing.  Based on the return value, image
   * will be deleted when new image is taken, or deleting of the image will
   * become responsibility of process which forked with this call.
   *
   * @return IMAGE_DO_BASIC_PROCESSING when image still should be handled by
   * connection, or IMAGE_KEEP_COPY if processing instance will delete image.
   */
  virtual imageProceRes processImage (Rts2Image * image);

  CameraImages::iterator processCameraImage (CameraImages::iterator & cis);

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

class Rts2DevClientFocusImage:public Rts2DevClientFocus
{
public:
  Rts2DevClientFocusImage (Rts2Conn * in_connection);
  virtual void postEvent (Rts2Event * event);
};

class Rts2DevClientWriteImage:public Rts2DevClient
{
public:
  Rts2DevClientWriteImage (Rts2Conn * in_connection);
  virtual void postEvent (Rts2Event * event);

  virtual void infoOK ();
  virtual void infoFailed ();
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
