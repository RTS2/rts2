#ifndef __RTS2_DEVCLI_COP_TELD__
#define __RTS2_DEVCLI_COP_TELD__

#include "../utils/rts2devclient.h"

#define EVENT_COP_START_SYNC	RTS2_LOCAL_EVENT + 550
#define EVENT_COP_SYNCED	RTS2_LOCAL_EVENT + 551

class Rts2DevClientCupolaTeld:public Rts2DevClientCupola
{
protected:
  virtual void syncEnded ();
public:
    Rts2DevClientCupolaTeld (Rts2Conn * conn);
    virtual ~ Rts2DevClientCupolaTeld (void);
  virtual void syncFailed (int status);
  virtual void postEvent (Rts2Event * event);
};

#endif /* !__RTS2_DEVCLI_COP_TELD__ */
