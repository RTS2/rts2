#ifndef __RTS2_NCOMWIN__
#define __RTS2_NCOMWIN__

#include "rts2daemonwindow.h"

class Rts2NComWin:public Rts2NWindow
{
public:
  Rts2NComWin (WINDOW * master_window);
  virtual ~ Rts2NComWin (void);
  virtual int injectKey (int key);
  virtual void draw ();

  void commandReturn (Rts2Command * cmd, int cmd_status);
};

#endif /* !__RTS2_NCOMWIN__ */
