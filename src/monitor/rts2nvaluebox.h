#ifndef __RTS2_NVALUEBOX__
#define __RTS2_NVALUEBOX__

#include "../utils/rts2value.h"

#include "rts2nwindow.h"

class Rts2NValueBox:public Rts2NWindow
{
private:
  Rts2Value * val;
public:
  Rts2NValueBox (WINDOW * master_window, int w, int h, Rts2Value * in_val);
};


class Rts2NValueBoxBool:public Rts2NValueBox
{
public:
  Rts2NValueBoxBool (WINDOW * master_window, int w, int h,
		     Rts2ValueBool * in_val);
};

#endif /* !__RTS2_NVALUEBOX__ */
