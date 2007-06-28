#ifndef __RTS2_SWAITFOR__
#define __RTS2_WAITFOR__

#include "rts2script.h"

class Rts2SWaitFor:public Rts2ScriptElement
{
private:
  std::string valueName;
  double tarval;
  double range;
public:
    Rts2SWaitFor (Rts2Script * in_script, char *valueName, double value,
		  double range);
  virtual void idle ();
};

#endif /* !__RTS2_WAITFOR__ */
