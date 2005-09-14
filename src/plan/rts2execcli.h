#ifndef __RTS2_EXECCLI__
#define __RTS2_EXECCLI__

#include "rts2script.h"
#include "rts2devscript.h"

#include "../writers/rts2devcliimg.h"
#include "../utils/rts2event.h"
#include "../utilsdb/target.h"

#define EVENT_SCRIPT_STARTED   RTS2_LOCAL_EVENT+50
#define EVENT_LAST_READOUT     RTS2_LOCAL_EVENT+51
#define EVENT_SCRIPT_ENDED     RTS2_LOCAL_EVENT+52
#define EVENT_MOVE_OK	       RTS2_LOCAL_EVENT+53
#define EVENT_MOVE_FAILED      RTS2_LOCAL_EVENT+54
// to get correct scriptCount..
#define EVENT_MOVE_QUESTION    RTS2_LOCAL_EVENT+55
#define EVENT_KILL_ALL         RTS2_LOCAL_EVENT+57

#define EVENT_SLEW_TO_TARGET   RTS2_LOCAL_EVENT+58

#define EVENT_TEL_SCRIPT_CHANGE  RTS2_LOCAL_EVENT+59
#define EVENT_TEL_SCRIPT_RESYNC  RTS2_LOCAL_EVENT+60

#define EVENT_ACQUSITION_END	RTS2_LOCAL_EVENT+61

class Rts2DevClientCameraExec:public Rts2DevClientCameraImage,
  public Rts2DevScript
{
protected:
  virtual void unblockWait ()
  {
    Rts2DevClientCameraImage::unblockWait ();
  }
  virtual void unsetWait ()
  {
    Rts2DevClientCameraImage::unsetWait ();
  }

  virtual void clearWait ()
  {
    Rts2DevClientCameraImage::clearWait ();
  }

  virtual int isWaitMove ()
  {
    return Rts2DevClientCameraImage::isWaitMove ();
  }

  virtual void setWaitMove ()
  {
    Rts2DevClientCameraImage::setWaitMove ();
  }

  virtual int getFailedCount ()
  {
    return Rts2DevClient::getFailedCount ();
  }

  virtual void clearFailedCount ()
  {
    Rts2DevClient::clearFailedCount ();
  }

  virtual void exposureStarted ();
  virtual void exposureEnd ();
  virtual void readoutEnd ();

  int imgCount;

  virtual void startTarget ();

  virtual int getNextCommand ();
public:
  Rts2DevClientCameraExec (Rts2Conn * in_connection);
  virtual ~ Rts2DevClientCameraExec (void);
  virtual void postEvent (Rts2Event * event);
  virtual void nextCommand ();
  virtual Rts2Image *createImage (const struct timeval *expStart);
  virtual void processImage (Rts2Image * image);
  virtual void exposureFailed (int status);

  virtual void filterOK ();
  virtual void filterFailed (int status);
};

class Rts2DevClientTelescopeExec:public Rts2DevClientTelescopeImage
{
private:
  Target * currentTarget;
  int blockMove;
  Rts2CommandChange *cmdChng;

  struct ln_equ_posn fixedOffset;

  int syncTarget ();
  void checkInterChange ();
protected:
    virtual void moveEnd ();
  virtual void searchEnd ();
public:
    Rts2DevClientTelescopeExec (Rts2Conn * in_connection);
  virtual void postEvent (Rts2Event * event);
  virtual void moveFailed (int status);
  virtual void searchFailed (int status);
};

class Rts2DevClientMirrorExec:public Rts2DevClientMirrorImage
{
protected:
  virtual void mirrorA ();
  virtual void mirrorB ();
public:
    Rts2DevClientMirrorExec (Rts2Conn * in_connection);
  virtual void postEvent (Rts2Event * event);

  virtual void moveFailed (int status);
};

#endif /*! __RTS2_EXECCLI__ */
