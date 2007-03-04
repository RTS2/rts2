#ifndef __RTS2_VALUELIST__
#define __RTS2_VALUELIST__

#include <vector>

#include "rts2value.h"

/**
 * Represent set of Rts2Values. It's used to store values which shall
 * be reseted when new script starts etc..
 */
class Rts2ValueList:public
  std::vector <
Rts2Value * >
{
public:
  Rts2ValueList ()
  {
  }
  ~
  Rts2ValueList (void)
  {
    for (Rts2ValueList::iterator iter = begin (); iter != end (); iter++)
      delete *
	iter;
  }
};

#endif /* !__RTS2_VALUELIST__ */
