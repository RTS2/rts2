#include "rts2nwindowedit.h"

#include <ctype.h>

#define MIN(x,y) ((x < y) ? x : y)

Rts2NWindowEdit::Rts2NWindowEdit (int x, int y, int w, int h, int in_ex,
				  int in_ey, int in_ew, int in_eh,
				  int border):
Rts2NWindow (x, y, w, h, border)
{
  ex = in_ex;
  ey = in_ey;
  eh = in_eh;
  ew = in_ew;
  comwin = newpad (in_eh, in_ew);
}

Rts2NWindowEdit::~Rts2NWindowEdit (void)
{
  delwin (comwin);
}

keyRet Rts2NWindowEdit::injectKey (int key)
{
  int
    x,
    y;
  if (isalnum (key))
    {
      waddch (getWriteWindow (), key);
      return RKEY_HANDLED;
    }
  switch (key)
    {
    case KEY_BACKSPACE:
      getyx (comwin, y, x);
      mvwdelch (comwin, y, x - 1);
      return RKEY_HANDLED;
    case KEY_LEFT:
      break;
    case KEY_RIGHT:
      break;
    default:
      return Rts2NWindow::injectKey (key);
    }
  return RKEY_HANDLED;
}

void
Rts2NWindowEdit::refresh ()
{
  int x, y;
  int w, h;
  Rts2NWindow::refresh ();
  getbegyx (window, y, x);
  getmaxyx (window, h, w);
  if (pnoutrefresh
      (getWriteWindow (), 0, 0, y + ey, x + ex, MIN (y + ey + eh, y + h - 1),
       MIN (x + ex + ew, x + w - 2)) == ERR)
    errorMove ("pnoutrefresh comwin", y + ey, x + ey,
	       MIN (y + ey + eh, y + h - 1), MIN (x + ex + ew, x + w - 2));
}
