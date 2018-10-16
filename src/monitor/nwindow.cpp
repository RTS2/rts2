/*
 * Windowing class build on ncurses
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
#include "nwindow.h"

#include <iostream>

using namespace rts2ncurses;

NWindow::NWindow (int x, int y, int w, int h, bool border):Layout ()
{
	if (h <= 0)
		h = 0;
	if (w <= 0)
		w = 0;
	if (x < 0)
		x = 0;
	if (y < 0)
		y = 0;
	window = newwin (h, w, y, x);
	if (!window)
		errorMove ("newwin", y, x, h, w);
	_haveBox = border;
	active = false;
}

NWindow::~NWindow (void)
{
	delwin (window);
}

keyRet NWindow::injectKey (__attribute__ ((unused)) int key)
{
	return RKEY_NOT_HANDLED;
}

void NWindow::draw ()
{
	werase (window);
	if (haveBox ())
	{
		if (isActive ())
			wborder (window, A_REVERSE | ACS_VLINE, A_REVERSE | ACS_VLINE,
			A_REVERSE | ACS_HLINE, A_REVERSE | ACS_HLINE,
			A_REVERSE | ACS_ULCORNER,
			A_REVERSE | ACS_URCORNER,
			A_REVERSE | ACS_LLCORNER,
			A_REVERSE | ACS_LRCORNER);
		else
			box (window, 0, 0);
	}

	if (!title.empty ())
		mvwprintw (window, 0, 2, "%s", title.c_str ());
}

int NWindow::getX ()
{
	return getbegx (window);
}

int NWindow::getY ()
{
	return getbegy (window);
}

int NWindow::getCurX ()
{
	return getcurx (getWriteWindow ());
}

int NWindow::getCurY ()
{
	return getcury (getWriteWindow ());
}

int NWindow::getWindowX ()
{
	return getcurx (window);
}

int NWindow::getWindowY ()
{
	return getcury (window);
}

int NWindow::getWidth ()
{
	return getmaxx (window);
}

int NWindow::getHeight ()
{
	return getmaxy (window);
}

int NWindow::getWriteWidth ()
{
	int ret = getWidth ();
	if (haveBox ())
		return ret - 2;
	return ret;
}

int NWindow::getWriteHeight ()
{
	int ret = getHeight ();
	if (haveBox ())
		return ret - 2;
	return ret;
}

void NWindow::errorMove (const char *op, int y, int x, int h, int w)
{
	endwin ();
	std::cout << "Cannot perform ncurses operation " << op
		<< " y x h w "
		<< y << " "
		<< x << " "
		<< h << " "
		<< w << " " << "LINES COLS " << LINES << " " << COLS << std::endl;
	exit (EXIT_FAILURE);
}

void NWindow::winmove (int x, int y)
{
	int _x, _y;
	getbegyx (window, _y, _x);
	if (x == _x && y == _y)
		return;
	if (mvwin (window, y, x) == ERR)
		errorMove ("mvwin", y, x, -1, -1);
}

void NWindow::resize (int x, int y, int w, int h)
{
	if (wresize (window, h, w) == ERR)
		errorMove ("wresize", 0, 0, h, w);
	winmove (x, y);
}

void NWindow::grow (int max_w, int h_dif)
{
	int x, y, w, h;
	getbegyx (window, y, x);
	getmaxyx (window, h, w);
	if (max_w > w)
		w = max_w;
	resize (x, y, w, h + h_dif);
}

void NWindow::winrefresh ()
{
	wnoutrefresh (window);
}

void NWindow::enter ()
{
	active = true;
}

void NWindow::leave ()
{
	active = false;
}

bool NWindow::setCursor ()
{
	return false;
}
