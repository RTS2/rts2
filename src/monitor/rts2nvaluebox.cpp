#include "rts2nvaluebox.h"

Rts2NValueBox::Rts2NValueBox (WINDOW * master_window, int w, int h,
			      Rts2Value * in_val):
Rts2NWindow (master_window, 0, 0, w, h)
{
  val = in_val;
}

Rts2NValueBoxBool::Rts2NValueBoxBool (WINDOW * master_window, int w, int h,
				      Rts2ValueBool * in_val):
Rts2NValueBox (master_window, w, h, in_val)
{

}
