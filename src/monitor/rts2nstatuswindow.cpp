#include "rts2nstatuswindow.h"
#include "nmonitor.h"

Rts2NStatusWindow::Rts2NStatusWindow (WINDOW * master_window, Rts2NComWin * in_comWin, Rts2Client * in_master):Rts2NWindow (master_window, 0, LINES - 1, COLS, 1,
	     0)
{
  master = in_master;
  comWin = in_comWin;
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
  werase (window);

  wcolor_set (window, CLR_STATUS, NULL);
  mvwprintw (window, 0, 0, "%s %i", master->getMasterStateString ().c_str (),
	     comWin->getCurX ());
  strftime (dateBuf, 20, "%Y-%m-%d %H:%M:%S", gmtime (&now));
  mvwprintw (window, 0, COLS - 19, "%19s", dateBuf);
  wcolor_set (window, CLR_DEFAULT, NULL);

  refresh ();
}
