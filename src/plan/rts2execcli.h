#ifndef __RTS2_EXECCLI__
#define __RTS2_EXECCLI__

#include "rts2script.h"
#include "../writers/rts2devcliimg.h"
#include "../utils/rts2event.h"
#include "../utilsdb/target.h"

#define EVENT_SCRIPT_STARTED   RTS2_LOCAL_EVENT+50
#define EVENT_LAST_READOUT     RTS2_LOCAL_EVENT+51
#define EVENT_SCRIPT_ENDED     RTS2_LOCAL_EVENT+52

class Rts2DevClientCameraExec:public Rts2DevClientCameraImage
{
private:
  int sendLastReadout;
  Target *currentTarget;
  Rts2Script *script;
  void postLastReadout ();
public:
    Rts2DevClientCameraExec (Rts2Conn * in_connection);
    virtual ~ Rts2DevClientCameraExec (void);
  virtual void postEvent (Rts2Event * event);
  void nextCommand ();
  virtual Rts2Image *createImage (const struct timeval *expStart);
  virtual void processImage (Rts2Image * image);
  virtual void stateChanged (Rts2ServerState * state);
};

class Rts2DevClientTelescopeExec:public Rts2DevClientTelescopeImage
{
private:
  Target * currentTarget;
public:
  Rts2DevClientTelescopeExec (Rts2Conn * in_connection);
  virtual void postEvent (Rts2Event * event);
  virtual void stateChanged (Rts2ServerState * state);
};

#endif /*! __RTS2_EXECCLI__ */
