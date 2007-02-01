#include "rts2nmsgwindow.h"

Rts2NMsgWindow::Rts2NMsgWindow (WINDOW * master_window):Rts2NWindow (master_window, 0, LINES - 19, COLS,
	     18)
{
  scrollok (window, TRUE);
}

Rts2NMsgWindow::~Rts2NMsgWindow (void)
{
}

int
Rts2NMsgWindow::injectKey (int key)
{
  return 0;
}

void
Rts2NMsgWindow::draw ()
{
  Rts2NWindow::draw ();
  refresh ();
}


Rts2NMsgWindow & operator << (Rts2NMsgWindow & msgwin, Rts2Message & msg)
{
/*  char *buf;
  char *fmt;
  switch (msg.getType ())
    {
    case MESSAGE_ERROR:
      fmt = "<L></6></B>";
      break;
    case MESSAGE_WARNING:
      fmt = "<L></5>";
      break;
    case MESSAGE_INFO:
      fmt = "<L></4>";
      break;
    case MESSAGE_DEBUG:
      fmt = "<L></3>";
      break;
    }
  return msgwin; */
  wprintw (msgwin.window, "%s\n", msg.toString ().c_str ());
  msgwin.draw ();
  return msgwin;
}
