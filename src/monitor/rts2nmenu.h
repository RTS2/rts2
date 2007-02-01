#ifndef __RTS2_NMENU__
#define __RTS2_NMENU__

#include "rts2daemonwindow.h"

class Rts2NMenu:public Rts2NWindow
{
public:
  Rts2NMenu (WINDOW * master_window);
  virtual ~ Rts2NMenu (void);
  virtual int injectKey (int key);
  virtual void draw ();
};

#endif /* !__RTS2_NMENU__ */
