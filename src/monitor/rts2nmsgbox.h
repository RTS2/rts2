#ifndef __RTS2_NMSGBOX__
#define __RTS2_NMSGBOX__

#include "rts2daemonwindow.h"

class Rts2NMsgBox:public Rts2NWindow
{
private:
  const char *query;
  const char **buttons;
  int butnum;
public:
    Rts2NMsgBox (const char *in_query,
		 const char *in_buttons[], int in_butnum);
    virtual ~ Rts2NMsgBox (void);
  virtual int injectKey (int key);
  virtual void draw ();
  /**
   * -1 when exited with KEY_ESC, >=0 when exited with enter, it's
   *  index of selected button
   */
  int exitState;
};

#endif /* !__RTS2_NMSGBOX__ */
