#ifndef __RTS2_CENTRALSTATE__
#define __RTS2_CENTRALSTATE__

#include <ostream>

#include "rts2serverstate.h"

class Rts2CentralState:public Rts2ServerState
{
private:

public:
  Rts2CentralState (int in_state):Rts2ServerState ()
  {
    setValue (in_state);
  }

  const char *getStringShort ();
  std::string getString ();

  friend std::ostream & operator << (std::ostream & _os,
				     Rts2CentralState c_state);
};

std::ostream & operator << (std::ostream & _os, Rts2CentralState c_state);

#endif /* !__RTS2_CENTRALSTATE__ */
