/*
 * NCurses layout engine
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
#include "daemonwindow.h"

#include <iostream>

using namespace rts2ncurses;

NSelWindow::NSelWindow (int x, int y, int w, int h, int border, int sw, int sh):NWindow (x, y, w, h, border)
{
	selrow = 0;
	maxrow = 0;
	padoff_x = 0;
	padoff_y = 0;
	scrolpad = newpad (sh, sw);
	lineOffset = 1;
}

NSelWindow::~NSelWindow (void)
{
	delwin (scrolpad);
}

keyRet NSelWindow::injectKey (int key)
{
	switch (key)
	{
		case KEY_HOME:
			selrow = 0;
			break;
		case KEY_END:
			selrow = maxrow - 1;
			break;
		case KEY_NPAGE:
			changeSelRow (getHeight ());
			break;
		case KEY_DOWN:
			changeSelRow (+1);
			break;
		case KEY_UP:
			changeSelRow (-1);
			break;
		case KEY_PPAGE:
			changeSelRow (-getHeight ());
			break;
		case KEY_LEFT:
			if (padoff_x > 0)
			{
				padoff_x--;
			}
			else
			{
				// beep ();
				// flash ();
			}
			break;
		case KEY_RIGHT:
			if (padoff_x < 300)
			{
				padoff_x++;
			}
			else
			{
				// beep ();
				// flash ();
			}
			break;
		default:
			return NWindow::injectKey (key);
	}
	return RKEY_HANDLED;
}

void NSelWindow::winrefresh ()
{
	int x, y;
	int w, h;
	getmaxyx (scrolpad, h, w);
	if (selrow >= maxrow || selrow < 0)
		selrow = maxrow - 1;
	if (maxrow > 0)
	{
		// Paint the cursor
		if (isActive ())
			mvwchgat (scrolpad, selrow, 0, w, A_REVERSE, CLR_PRIORITY, NULL); // no O_BLINK
		else
			mvwchgat (scrolpad, selrow, 0, w, A_REVERSE, 0, NULL);
	}
	// Update the padding
	if (selrow >= maxrow - 1)
		padoff_y = (maxrow - 1) - getWriteHeight () + 1 + lineOffset;
	else if (selrow >= padoff_y + getWriteHeight () - lineOffset)
		padoff_y = selrow - getWriteHeight () + 1 + lineOffset;
	else if (selrow < padoff_y)
		padoff_y = selrow;

	// Keep the last row at the bottom, if possible
	if (padoff_y > maxrow - getWriteHeight ())
		padoff_y = fmax(0, maxrow - getWriteHeight ());

	NWindow::winrefresh ();
	getbegyx (window, y, x);
	getmaxyx (window, h, w);

	if (haveBox ())
		pnoutrefresh (scrolpad, padoff_y, padoff_x, y + 1, x + 1, y + h - 2, x + w - 2);
	else
		pnoutrefresh (scrolpad, padoff_y, padoff_x, y, x, y + h - 1, x + w - 1);
}

void NSelWindow::changeSelRow (int change)
{
	if (selrow < 0)
		selrow = maxrow - 1;

	selrow += change;

	if (selrow >= maxrow)
		selrow = 0;
	else if (selrow < 0)
		selrow = maxrow - 1;
}

NDevListWindow::NDevListWindow (rts2core::Block * in_block, rts2core::connections_t *in_conns):NSelWindow (0, 1, 10, LINES - 20, 1, 50, 300)
{
	block = in_block;
	conns = in_conns;
}

NDevListWindow::~NDevListWindow (void)
{
}

void NDevListWindow::draw ()
{
	NWindow::draw ();
	werase (scrolpad);
	maxrow = 0;
	rts2core::connections_t::iterator iter;
	for (iter = block->getCentraldConns ()->begin (); iter != block->getCentraldConns ()->end (); iter++)
	{
		wprintw (scrolpad, "centrald\n");
		maxrow++;
	}
	for (iter = conns->begin (); iter != conns->end (); iter++)
	{
		if ((*iter)->getState () & DEVICE_ERROR_MASK)
			wcolor_set (scrolpad, CLR_FAILURE, NULL);
		else if ((*iter)->getState () & WEATHER_MASK)
			wcolor_set (scrolpad, CLR_WARNING, NULL);
		else
			wcolor_set (scrolpad, CLR_OK, NULL);
		wprintw (scrolpad, "%s\n", (*iter)->getName ());
		maxrow++;
	}

	wcolor_set (scrolpad, CLR_DEFAULT, NULL);

	for (rts2core::clients_t::iterator cli = block->getClients ()->begin (); cli != block->getClients ()->end (); cli++)
	{
		wprintw (scrolpad, "%s\n", (*cli)->getName ());
		maxrow++;
	}
	wprintw (scrolpad, "status");
	maxrow++;
	winrefresh ();
}

void NCentraldWindow::printState (rts2core::Connection * conn)
{
	if (conn->getErrorState ())
		wcolor_set (getWriteWindow (), CLR_FAILURE, NULL);
	else
		wcolor_set (getWriteWindow (), CLR_OK, NULL);
	wprintw (getWriteWindow (), "%s %s (%x)\n", conn->getName (),
		conn->getStateString ().c_str (), conn->getState ());
	wcolor_set (getWriteWindow (), CLR_DEFAULT, NULL);
}

void NCentraldWindow::drawDevice (rts2core::Connection * conn)
{
	printState (conn);
	maxrow++;
}

NCentraldWindow::NCentraldWindow (rts2core::Client * in_client):NSelWindow (10, 1, COLS - 10, LINES - 25, 1, 300, 300)
{
	client = in_client;
	draw ();
}

NCentraldWindow::~NCentraldWindow (void)
{
}

void NCentraldWindow::draw ()
{
	NSelWindow::draw ();
	werase (getWriteWindow ());
	maxrow = 0;
	rts2core::connections_t::iterator iter;
	for (iter = client->getCentraldConns ()->begin (); iter != client->getCentraldConns ()->end (); iter++)
	{
		drawDevice (*iter);
	}
	for (iter = client->getConnections ()->begin (); iter != client->getConnections ()->end (); iter++)
	{
		drawDevice (*iter);
	}
	winrefresh ();
}
