#ifndef __RTS2_NMSGWINDOW__
#define __RTS2_NMSGWINDOW__

#include "rts2daemonwindow.h"
#include "../utils/rts2message.h"

class Rts2NMsgWindow:public Rts2NWindow
{
public:
  Rts2NMsgWindow (WINDOW * master_window);
  virtual ~ Rts2NMsgWindow (void);
  virtual int injectKey (int key);
  virtual void draw ();
  friend Rts2NMsgWindow & operator << (Rts2NMsgWindow & msgwin,
				       Rts2Message & msg);
};

Rts2NMsgWindow & operator << (Rts2NMsgWindow & msgwin, Rts2Message & msg);

#endif /* !__RTS2_NMSGWINDOW__ */
