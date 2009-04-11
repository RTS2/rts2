#include "rts2ncomwin.h"

#include "nmonitor.h"

Rts2NComWin::Rts2NComWin ():Rts2NWindow (11, LINES - 24, COLS - 12, 3, 0)
{
	comwin = newpad (1, 300);
	statuspad = newpad (2, 300);
}


Rts2NComWin::~Rts2NComWin (void)
{
	delwin (comwin);
	delwin (statuspad);
}


keyRet Rts2NComWin::injectKey (int key)
{
	int
		x,
		y;
	switch (key)
	{
		case KEY_BACKSPACE:
			getyx (comwin, y, x);
			mvwdelch (comwin, y, x - 1);
			break;
		case KEY_ENTER:
		case K_ENTER:
			return RKEY_ENTER;
		default:
			waddch (comwin, key);
	}
	return Rts2NWindow::injectKey (key);
}


void
Rts2NComWin::draw ()
{
	Rts2NWindow::draw ();
	refresh ();
}


void
Rts2NComWin::refresh ()
{
	int x, y;
	int w, h;
	Rts2NWindow::refresh ();
	getbegyx (window, y, x);
	getmaxyx (window, h, w);
	if (pnoutrefresh (comwin, 0, 0, y, x, y + 1, x + w - 1) == ERR)
		errorMove ("pnoutrefresh ComWin", y, x, y + 1, x + w - 1);
	if (pnoutrefresh (statuspad, 0, 0, y + 1, x, y + h - 1, x + w - 1) == ERR)
		errorMove ("pnoutrefresh statuspad", y + 1, x, y + h - 1, x + w - 1);
}


bool
Rts2NComWin::setCursor ()
{
	int x, y;
	getyx (comwin, y, x);
	x += getX ();
	y += getY ();
	setsyx (y, x);
	return true;
}


void
Rts2NComWin::commandReturn (Rts2Command * cmd, int cmd_status)
{
	if (cmd_status == 0)
		wcolor_set (statuspad, CLR_OK, NULL);
	else
		wcolor_set (statuspad, CLR_FAILURE, NULL);
	mvwprintw (statuspad, 1, 0, "%s %+04i %s",
		cmd->getConnection ()->getName (), cmd_status, cmd->getText ());
	wcolor_set (statuspad, CLR_DEFAULT, NULL);
	int y, x;
	getyx (statuspad, y, x);
	for (; x < getWidth (); x++)
		mvwaddch (statuspad, y, x, ' ');
	wmove (comwin, 0, 0);
}
