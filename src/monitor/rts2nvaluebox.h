#ifndef __RTS2_NVALUEBOX__
#define __RTS2_NVALUEBOX__

#include "../utils/rts2value.h"

#include "rts2nwindow.h"

/**
 * Holds edit box for value.
 */
class Rts2NValueBox:public Rts2NWindow
{
private:
  Rts2Value * val;
public:
  Rts2NValueBox (WINDOW * master_window, int x, int y, int w, int h,
		 Rts2Value * in_val);
  virtual int injectKey (int key) = 0;
};

/**
 * Holds edit box for boolean value.
 */
class Rts2NValueBoxBool:public Rts2NValueBox
{
public:
  Rts2NValueBoxBool (WINDOW * master_window, Rts2ValueBool * in_val, int y);
  virtual int injectKey (int key);
  virtual void draw ();
};

#endif /* !__RTS2_NVALUEBOX__ */
