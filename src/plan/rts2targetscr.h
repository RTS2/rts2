#ifndef __RTS2_TARGESCR__
#define __RTS2_TARGESCR__

#include "scriptexec.h"
#include "../utils/rts2target.h"

class Rts2ScriptExec;

/**
 * This target is used in Rts2ScriptExec to fill role of current
 * target, so executor logic will work properly.
 */
class Rts2TargetScr:public Rts2Target
{
private:
  Rts2ScriptExec * master;
public:
  Rts2TargetScr (Rts2ScriptExec * in_master);
  virtual ~ Rts2TargetScr (void);

  // target manipulation functions
  virtual int getScript (const char *device_name, char *buf);

  // return target position at given julian date
  virtual int getPosition (struct ln_equ_posn *pos, double JD);

  virtual int getObsTargetID ();

  virtual void acqusitionStart ();

  virtual int isAcquired ();
};

#endif /* !__RTS2_TARGESCR__ */
