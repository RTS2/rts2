#ifndef __RTS2_DEVCLIPHOT__
#define __RTS2_DEVCLIPHOT__

#include "rts2devscript.h"

class Rts2DevClientPhotExec:public Rts2DevClientPhot, public Rts2DevScript
{
protected:
  virtual void unblockWait ()
  {
    Rts2DevClientPhot::unblockWait ();
  }
  virtual void unsetWait ()
  {
    Rts2DevClientPhot::unsetWait ();
  }

  virtual void clearWait ()
  {
    Rts2DevClientPhot::clearWait ();
  }

  virtual int isWaitMove ()
  {
    return Rts2DevClientPhot::isWaitMove ();
  }

  virtual void setWaitMove ()
  {
    return Rts2DevClientPhot::setWaitMove ();
  }

  virtual void integrationEnd ();
  virtual void addCount (int count, float exp, int is_ov);

  virtual int getNextCommand ();
public:
  Rts2DevClientPhotExec (Rts2Conn * in_connection);
  virtual ~ Rts2DevClientPhotExec (void);
  virtual void postEvent (Rts2Event * event);
  virtual void integrationFailed (int status);
  virtual void nextCommand ();
};

#endif /* !__RTS2_DEVCLIPHOT__ */
