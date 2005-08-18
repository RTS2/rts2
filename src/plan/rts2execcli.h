#ifndef __RTS2_EXECCLI__
#define __RTS2_EXECCLI__

#include "rts2script.h"
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

class Rts2DevClientCameraExec:public Rts2DevClientCameraImage
{
private:
  Target * currentTarget;
  Rts2Script *script;
  Rts2Command *nextComd;
  char cmd_device[DEVICE_NAME_SIZE];

  enum
  { NO_WAIT, WAIT_SLAVE, WAIT_MASTER } waitAcq;

  void startTarget ();

  virtual void exposureStarted ();
  virtual void exposureEnd ();
  virtual void readoutEnd ();

  void deleteScript ();

  int blockMove;
  int getObserveStart;
  int imgCount;
  int nextPreparedCommand ();

public:
    Rts2DevClientCameraExec (Rts2Conn * in_connection);
    virtual ~ Rts2DevClientCameraExec (void);
  virtual void postEvent (Rts2Event * event);
  void nextCommand ();
  virtual Rts2Image *createImage (const struct timeval *expStart);
  virtual void processImage (Rts2Image * image);
  virtual void exposureFailed (int status);
};

class Rts2DevClientTelescopeExec:public Rts2DevClientTelescopeImage
{
private:
  Target * currentTarget;
  int blockMove;
  Rts2CommandChange *cmdChng;

  int syncTarget ();
  void checkInterChange ();
protected:
    virtual void moveEnd ();
public:
    Rts2DevClientTelescopeExec (Rts2Conn * in_connection);
  virtual void postEvent (Rts2Event * event);
  virtual void moveFailed (int status);
};

#endif /*! __RTS2_EXECCLI__ */
