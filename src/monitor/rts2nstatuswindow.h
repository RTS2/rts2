#ifndef __RTS2__NSTATUSWINDOW__
#define __RTS2__NSTATUSWINDOW__

#include "rts2daemonwindow.h"
#include "rts2ncomwin.h"
#include "../utils/rts2client.h"

class Rts2NStatusWindow:public Rts2NWindow
{
private:
  Rts2Client * master;
  Rts2NComWin *comWin;
public:
    Rts2NStatusWindow (Rts2NComWin * in_comWin, Rts2Client * in_master);
    virtual ~ Rts2NStatusWindow (void);
  virtual void draw ();
};

#endif /* !__RTS2__NSTATUSWINDOW__ */
