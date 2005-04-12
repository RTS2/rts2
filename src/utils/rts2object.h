#ifndef __RTS2_OBJECT__
#define __RTS2_OBJECT__

#include "rts2event.h"

/*******************************
 * 
 * Basic RTS2 class.
 *
 * It's solely important method is postEvent, which solves issues
 * related with conn<->block and other communications.
 *
 ******************************/

class Rts2Object
{
public:
  Rts2Object ()
  {
  }
  virtual ~ Rts2Object (void)
  {
  }
  virtual void postEvent (Rts2Event * event)
  {
    // discard event..
    delete event;
  }
};

#endif /*! __RTS2_OBJECT__ */
