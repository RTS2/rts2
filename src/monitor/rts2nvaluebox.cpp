#include "rts2nvaluebox.h"

Rts2NValueBox::Rts2NValueBox (Rts2NWindow * top, int x, int y, int w, int h,
			      Rts2Value * in_val):
Rts2NSelWindow (top->getX () + x, top->getY () + y, w, h)
{
  topWindow = top;
  val = in_val;
}

Rts2NValueBoxBool::Rts2NValueBoxBool (Rts2NWindow * top,
				      Rts2ValueBool * in_val, int y):
Rts2NValueBox (top, 1, y, 10, 4, in_val)
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
