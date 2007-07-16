#ifndef __RTS2_EXECCLI__
#define __RTS2_EXECCLI__

#include "rts2script.h"
#include "rts2devscript.h"

#include "../writers/rts2devcliimg.h"
#include "../utils/rts2event.h"
#include "../utils/rts2target.h"

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

#define EVENT_TEL_START_GUIDING	RTS2_LOCAL_EVENT+62
#define EVENT_TEL_STOP_GUIDING  RTS2_LOCAL_EVENT+63

#define EVENT_QUE_IMAGE		RTS2_LOCAL_EVENT+64

#define EVENT_STOP_OBSERVATION  RTS2_LOCAL_EVENT+65

#define EVENT_CORRECTING_OK	RTS2_LOCAL_EVENT+66

#define EVENT_SCRIPT_RUNNING_QUESTION	RTS2_LOCAL_EVENT+67

class GuidingParams
{
public:
  char dir;
  double dist;
    GuidingParams (char in_dir, double in_dist)
  {
    dir = in_dir;
    dist = in_dist;
  }
};

class Rts2DevClientCameraExec:public Rts2DevClientCameraImage,
  public Rts2DevScript
{
private:
  bool queCurrentImage;
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

  virtual int queCommandFromScript (Rts2Command * com)
  {
    return queCommand (com);
  }

  virtual int getFailedCount ()
  {
    return Rts2DevClient::getFailedCount ();
  }

  virtual void clearFailedCount ()
  {
    Rts2DevClient::clearFailedCount ();
  }

  virtual void idle ()
  {
    Rts2DevScript::idle ();
    Rts2DevClientCameraImage::idle ();
  }

  virtual void exposureStarted ();
  virtual void exposureEnd ();
  virtual void readoutEnd ();

  int imgCount;

  virtual void startTarget ();

  virtual int getNextCommand ();

  virtual void clearBlockMove ();
public:
  Rts2DevClientCameraExec (Rts2Conn * in_connection);
  virtual ~ Rts2DevClientCameraExec (void);
  virtual void postEvent (Rts2Event * event);
  virtual void nextCommand ();
  void queImage (Rts2Image * image);
  virtual imageProceRes processImage (Rts2Image * image);
  virtual void exposureFailed (int status);

  virtual void filterOK ();
  virtual void filterFailed (int status);

  virtual void settingsOK ();
  virtual void settingsFailed (int status);
};

class Rts2DevClientTelescopeExec:public Rts2DevClientTelescopeImage
{
private:
  Rts2Target * currentTarget;
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

class Rts2DevClientMirrorExec:public Rts2DevClientMirror
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
