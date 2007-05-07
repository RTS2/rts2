#ifndef __RTS2_NWINDOWEDIT__
#define __RTS2_NWINDOWEDIT__

#include "rts2nwindow.h"

/**
 * This is window with single edit box to edit values
 */
class Rts2NWindowEdit:public Rts2NWindow
{
private:
  WINDOW * comwin;
  int ex, ey, ew, eh;
public:
  /**
   * ex, ey, ew and eh are position of (single) edit box within this window.
   */
    Rts2NWindowEdit (int x, int y, int w, int h, int in_ex, int in_ey,
		     int in_ew, int in_eh, int border = 1);
    virtual ~ Rts2NWindowEdit (void);

  virtual keyRet injectKey (int key);
  virtual void refresh ();

  virtual WINDOW *getWriteWindow ()
  {
    return comwin;
  }
};

#endif /* !__RTS2_NWINDOWEDIT__ */
