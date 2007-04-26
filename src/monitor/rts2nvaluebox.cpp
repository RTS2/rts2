#include "rts2nvaluebox.h"

Rts2NValueBox::Rts2NValueBox (WINDOW * master_window, int x, int y, int w,
			      int h, Rts2Value * in_val):
Rts2NWindow (master_window, x, y, w, h)
{
  val = in_val;
}

Rts2NValueBoxBool::Rts2NValueBoxBool (WINDOW * master_window,
				      Rts2ValueBool * in_val, int y):
Rts2NValueBox (master_window, 1, y, 10, 4, in_val)
{

}

int
Rts2NValueBoxBool::injectKey (int key)
{
  return -1;
}

void
Rts2NValueBoxBool::draw ()
{
  Rts2NValueBox::draw ();
  mvwprintw (getWriteWindow (), 1, 1, "true");
  mvwprintw (getWriteWindow (), 2, 1, "false");
  refresh ();
}
