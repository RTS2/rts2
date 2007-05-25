#include "rts2nwindowedit.h"
#include "nmonitor.h"

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

bool
Rts2NWindowEdit::passKey (int key)
{
  return isalnum (key);
}

keyRet
Rts2NWindowEdit::injectKey (int key)
{
  int x, y;
  switch (key)
    {
    case KEY_BACKSPACE:
      getyx (comwin, y, x);
      mvwdelch (comwin, y, x - 1);
      return RKEY_HANDLED;
    case KEY_EXIT:
    case K_ESC:
      return RKEY_ESC;
    case KEY_ENTER:
    case K_ENTER:
      return RKEY_ENTER;
    case KEY_LEFT:
      x = getCurX ();
      if (x > 0)
	wmove (getWriteWindow (), getCurY (), x - 1);
      break;
    case KEY_RIGHT:
      x = getCurX ();
      if (x < getWidth () - 3)
	wmove (getWriteWindow (), getCurY (), x + 1);
      break;
    default:
      if (isalnum (key))
	{
	  if (passKey (key))
	    {
	      waddch (getWriteWindow (), key);
	    }
	  // alfanum key, and it does not passed..
	  else
	    {
	      beep ();
	      flash ();
	    }
	  return RKEY_HANDLED;
	}
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

bool
Rts2NWindowEdit::setCursor ()
{
  int x, y;
  getbegyx (getWriteWindow (), y, x);
  x += getCurX ();
  y += getCurY ();
  setsyx (y, x);
  return true;
}

Rts2NWindowEditIntegers::Rts2NWindowEditIntegers (int x, int y, int w, int h, int in_ex, int in_ey, int in_ew, int in_eh, int border):
Rts2NWindowEdit (x, y, w, h, in_ex, in_ey, in_ew, in_eh, border)
{
}


bool
Rts2NWindowEditIntegers::passKey (int key)
{
  if (isdigit (key) || key == '+' || key == '-')
    return true;
  return false;
}

Rts2NWindowEditDigits::Rts2NWindowEditDigits (int x, int y, int w, int h, int in_ex, int in_ey, int in_ew, int in_eh, int border):
Rts2NWindowEdit (x, y, w, h, in_ex, in_ey, in_ew, in_eh, border)
{
}

bool
Rts2NWindowEditDigits::passKey (int key)
{
  if (isdigit (key) || key == '.' || key == ',' || key == '+' || key == '-')
    return true;
  return false;
}
