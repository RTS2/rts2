/*
 * RTS2 communication window
 * Copyright (C) 2003-2007,2010 Petr Kubanek <petr@kubanek.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "nmonitor.h"
#include "ncomwin.h"

using namespace rts2ncurses;

NComWin::NComWin ():NWindow (11, LINES - 24, COLS - 12, 3, 0)
{
	comwin = newpad (1, 300);
	statuspad = newpad (2, 300);
}

NComWin::~NComWin (void)
{
	delwin (comwin);
	delwin (statuspad);
}

keyRet NComWin::injectKey (int key)
{
	int x, y;
	switch (key)
	{
		case KEY_BACKSPACE:
		case 127:
			getyx (comwin, y, x);
			mvwdelch (comwin, y, x - 1);
			break;
		case KEY_CTRL ('P'):
			showHistory (1);
			break;
		case KEY_CTRL ('N'):
			showHistory (-1);
			break;
		case KEY_ENTER:
		case K_ENTER:
			return RKEY_ENTER;
		default:
			waddch (comwin, key);
	}
	return NWindow::injectKey (key);
}

void NComWin::showHistory (int dir)
{
	if (dir > 0 && historyPos < history.size ())
		historyPos ++;

	if (dir < 0 && historyPos > 0)
		historyPos --;

	werase (comwin);

	if (historyPos > 0)
		mvwprintw (comwin, 0, 0, history.at (historyPos - 1).c_str ());
}

void NComWin::draw ()
{
	NWindow::draw ();
	winrefresh ();
}
void NComWin::winrefresh ()
{
	int x, y;
	int w, h;
	NWindow::winrefresh ();
	getbegyx (window, y, x);
	getmaxyx (window, h, w);
	if (pnoutrefresh (comwin, 0, 0, y, x, y + 1, x + w - 1) == ERR)
		errorMove ("pnoutrefresh ComWin", y, x, y + 1, x + w - 1);
	if (pnoutrefresh (statuspad, 0, 0, y + 1, x, y + h - 1, x + w - 1) == ERR)
		errorMove ("pnoutrefresh statuspad", y + 1, x, y + h - 1, x + w - 1);
}

bool NComWin::setCursor ()
{
	int x, y;
	getyx (comwin, y, x);
	x += getX ();
	y += getY ();
	setsyx (y, x);
	return true;
}

void NComWin::commandReturn (rts2core::Command * cmd, int cmd_status)
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
