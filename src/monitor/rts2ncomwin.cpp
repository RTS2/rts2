#include "rts2ncomwin.h"

#include "nmonitor.h"

Rts2NComWin::Rts2NComWin (WINDOW * master_window):Rts2NWindow (master_window, 11, LINES - 24, COLS - 12, 5,
	     0)
{
}

Rts2NComWin::~Rts2NComWin (void)
{

}

int
Rts2NComWin::injectKey (int key)
{
  int x, y;
  switch (key)
    {
    case KEY_BACKSPACE:
      getyx (window, y, x);
      mvwdelch (window, y, x - 1);
      break;
    case KEY_ENTER:
    case K_ENTER:
      return 0;
    default:
      waddch (window, key);
    }
  return -1;
}

void
Rts2NComWin::draw ()
{
  Rts2NWindow::draw ();
  refresh ();
}
