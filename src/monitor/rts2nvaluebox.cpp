#include "rts2nvaluebox.h"
#include "nmonitor.h"

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
  maxrow = 2;
  setLineOffset (0);
}

int
Rts2NValueBoxBool::injectKey (int key)
{
  switch (key)
    {
    case KEY_ENTER:
    case K_ENTER:
      return 0;
      break;
    }
  return Rts2NSelWindow::injectKey (key);
}

void
Rts2NValueBoxBool::draw ()
{
  Rts2NValueBox::draw ();
  werase (getWriteWindow ());
  mvwprintw (getWriteWindow (), 0, 1, "true");
  mvwprintw (getWriteWindow (), 1, 1, "false");
  refresh ();
}

void
Rts2NValueBoxBool::sendValue (Rts2Conn * connection)
{
  if (!connection->getOtherDevClient ())
    return;
  connection->
    queCommand (new
		Rts2CommandChangeValue (connection->getOtherDevClient (),
					getValue ()->getName (), '=',
					getSelRow () == 0));
}
