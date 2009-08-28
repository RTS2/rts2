#include "nmonitor.h"
#include "rts2nstatuswindow.h"

Rts2NStatusWindow::Rts2NStatusWindow (Rts2NComWin * in_comWin, Rts2Client * in_master):Rts2NWindow (0, LINES - 1, COLS, 1,
0)
{
	master = in_master;
	comWin = in_comWin;
}


Rts2NStatusWindow::~Rts2NStatusWindow (void)
{
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
	mvwhline (window, 0, 0, ' ', getWidth ());
	mvwprintw (window, 0, 0, "%s %x %i", master->getMasterStateString ().c_str (), master->getMasterState (),
		comWin->getCurX ());
	strftime (dateBuf, 20, "%Y-%m-%d %H:%M:%S", gmtime (&now));
	mvwprintw (window, 0, COLS - 19, "%19s", dateBuf);
	wcolor_set (window, CLR_DEFAULT, NULL);

	refresh ();
}
