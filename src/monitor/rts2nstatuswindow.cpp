#include "rts2nstatuswindow.h"
#include "nmonitor.h"

Rts2NStatusWindow::Rts2NStatusWindow (WINDOW * master_window, Rts2Client * in_master):Rts2NWindow (master_window, 0, LINES - 1, COLS, 1,
	     0)
{
  master = in_master;
}

Rts2NStatusWindow::~Rts2NStatusWindow (void)
{
}

int
Rts2NStatusWindow::injectKey (int key)
{
  return 0;
}

void
Rts2NStatusWindow::draw ()
{
  char dateBuf[21];
  time_t now;
  time (&now);

  Rts2NWindow::draw ();

  wcolor_set (window, CLR_STATUS, NULL);
  mvwprintw (window, 0, 0, "%s", master->getMasterStateString ().c_str ());
  wcolor_set (window, CLR_TEXT, NULL);
  strftime (dateBuf, 20, "%Y-%m-%d %H:%M:%S", gmtime (&now));
  mvwprintw (window, 0, COLS - 19, "%19s", dateBuf);

  refresh ();
}
