#include "rts2swaitfor.h"

Rts2SWaitFor::Rts2SWaitFor (Rts2Script * in_script, char *in_valueName,
			    double in_tarval, double in_range):
Rts2ScriptElement (in_script)
{
  valueName = std::string (in_valueName);
  tarval = in_tarval;
  range = in_range;
  setIdleTimeout (1);
}

void
Rts2SWaitFor::idle ()
{

}
