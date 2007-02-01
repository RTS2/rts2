#ifndef __RTS2_NMSGBOX__
#define __RTS2_NMSGBOX__

#include "rts2daemonwindow.h"

class Rts2NMsgBox:public Rts2NWindow
{
public:
  Rts2NMsgBox (WINDOW * master_window);
  virtual ~ Rts2NMsgBox (void);
  virtual int injectKey (int key);
  virtual void draw ();
  enum
  { MSG_YES, MSG_NO } exitState;
};

#endif /* !__RTS2_NMSGBOX__ */
