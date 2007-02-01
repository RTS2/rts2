#ifndef __RTS2__NSTATUSWINDOW__
#define __RTS2__NSTATUSWINDOW__

#include "rts2daemonwindow.h"
#include "../utils/rts2client.h"

class Rts2NStatusWindow:public Rts2NWindow
{
private:
  Rts2Client * master;
public:
  Rts2NStatusWindow (WINDOW * master_window, Rts2Client * in_master);
  virtual ~ Rts2NStatusWindow (void);
  virtual int injectKey (int key);
  virtual void draw ();
};

#endif /* !__RTS2__NSTATUSWINDOW__ */
