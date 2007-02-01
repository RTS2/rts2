#include "rts2nmsgbox.h"

Rts2NMsgBox::Rts2NMsgBox (WINDOW * master_window):Rts2NWindow (master_window, COLS / 2 - 3, LINES / 2 - 15, 30,
	     5)
{
  exitState = MSG_NO;
}

Rts2NMsgBox::~Rts2NMsgBox (void)
{
}

int
Rts2NMsgBox::injectKey (int key)
{
  return 0;
}

void
Rts2NMsgBox::draw ()
{
  Rts2NWindow::draw ();
  refresh ();
}
