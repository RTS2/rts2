#include "rts2nvaluebox.h"
#include "nmonitor.h"

Rts2NValueBox::Rts2NValueBox (Rts2NWindow * top, Rts2Value * in_val)
{
  topWindow = top;
  val = in_val;
}

Rts2NValueBox::~Rts2NValueBox (void)
{

}

Rts2NValueBoxBool::Rts2NValueBoxBool (Rts2NWindow * top,
				      Rts2ValueBool * in_val, int x, int y):
Rts2NValueBox (top, in_val),
Rts2NSelWindow (top->getX () + x, top->getY () + y, 10, 4)
{
  maxrow = 2;
  setLineOffset (0);
  if (!in_val->getValueBool ())
    setSelRow (1);
}

keyRet Rts2NValueBoxBool::injectKey (int key)
{
  switch (key)
    {
    case KEY_ENTER:
    case K_ENTER:
      return RKEY_ENTER;
      break;
    }
  return Rts2NSelWindow::injectKey (key);
}

void
Rts2NValueBoxBool::draw ()
{
  Rts2NSelWindow::draw ();
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

Rts2NValueBoxDouble::Rts2NValueBoxDouble (Rts2NWindow * top, Rts2ValueDouble * in_val, int x, int y):
Rts2NValueBox (top, in_val),
Rts2NWindow (top->getX () + x, top->getY () + y, 20, 3)
{
  comwin = newpad (1, 300);
  wprintw (comwin, "%f", in_val->getValueDouble ());
}

Rts2NValueBoxDouble::~Rts2NValueBoxDouble (void)
{
  delwin (comwin);
}

keyRet Rts2NValueBoxDouble::injectKey (int key)
{
  int
    x,
    y;
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
    case KEY_ENTER:
    case K_ENTER:
      return RKEY_ENTER;
      break;
    }
  if (isdigit (key) || key == '.' || key == ',' || key == '+' || key == '-')
    {
      waddch (comwin, key);
      return RKEY_HANDLED;
    }
  return Rts2NWindow::injectKey (key);
}

void
Rts2NValueBoxDouble::draw ()
{
  Rts2NWindow::draw ();
  refresh ();
}

void
Rts2NValueBoxDouble::refresh ()
{
  int x, y;
  int w, h;
  Rts2NWindow::refresh ();
  getbegyx (window, y, x);
  getmaxyx (window, h, w);
  if (pnoutrefresh (comwin, 0, 0, y + 1, x + 1, y + 1, x + w - 2) == ERR)
    errorMove ("pnoutrefresh comwin", y, x, y + 1, x + w - 1);
}

void
Rts2NValueBoxDouble::sendValue (Rts2Conn * connection)
{
  if (!connection->getOtherDevClient ())
    return;
  char buf[200];
  char *endptr;
  mvwinnstr (comwin, 0, 0, buf, 200);
  double tval = strtod (buf, &endptr);
  if (*endptr != '\0' && *endptr != ' ')
    {
      // log error;
      return;
    }
  connection->
    queCommand (new
		Rts2CommandChangeValue (connection->getOtherDevClient (),
					getValue ()->getName (), '=', tval));
}
