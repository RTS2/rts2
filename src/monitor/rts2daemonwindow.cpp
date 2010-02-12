#include "nmonitor.h"
#include "rts2daemonwindow.h"

#include <iostream>

Rts2NSelWindow::Rts2NSelWindow (int x, int y, int w, int h, int border, int sw, int sh):Rts2NWindow (x, y, w, h, border)
{
	selrow = 0;
	maxrow = 0;
	padoff_x = 0;
	padoff_y = 0;
	scrolpad = newpad (sh, sw);
	lineOffset = 1;
}

Rts2NSelWindow::~Rts2NSelWindow (void)
{
	delwin (scrolpad);
}

keyRet Rts2NSelWindow::injectKey (int key)
{
	switch (key)
	{
		case KEY_HOME:
			selrow = 0;
			break;
		case KEY_END:
			selrow = -1;
			break;
		case KEY_DOWN:
			if (selrow == -1)
				selrow = 0;
			else if (selrow < (maxrow - 1))
				selrow++;
			else
				selrow = 0;
			break;
		case KEY_UP:
			if (selrow == -1)
				selrow = (maxrow - 2);
			else if (selrow > 0)
				selrow--;
			else
				selrow = (maxrow - 1);
			break;
		case KEY_LEFT:
			if (padoff_x > 0)
			{
				padoff_x--;
			}
			else
			{
				beep ();
				flash ();
			}
			break;
		case KEY_RIGHT:
			if (padoff_x < 300)
			{
				padoff_x++;
			}
			else
			{
				beep ();
				flash ();
			}
			break;
		default:
			return Rts2NWindow::injectKey (key);
	}
	return RKEY_HANDLED;
}

void Rts2NSelWindow::refresh ()
{
	int x, y;
	int w, h;
	getmaxyx (scrolpad, h, w);
	if (selrow >= maxrow)
		selrow = maxrow - 1;
	if (maxrow > 0)
	{
		if (selrow >= 0)
		{
			if (isActive ())
				mvwchgat (scrolpad, selrow, 0, w, A_REVERSE | A_BLINK, CLR_PRIORITY, NULL);
			else
				mvwchgat (scrolpad, selrow, 0, w, A_REVERSE, 0, NULL);
		}
		else if (selrow == -1)
		{
			mvwchgat (scrolpad, maxrow - 1, 0, w, A_REVERSE, 0, NULL);
		}
	}
	Rts2NWindow::refresh ();
	getbegyx (window, y, x);
	getmaxyx (window, h, w);
	// normalize selrow
	if (selrow == -1)
		padoff_y = (maxrow - 1) - getWriteHeight () + 1 + lineOffset;
	else if ((selrow - padoff_y + lineOffset) >= getWriteHeight ())
		padoff_y = selrow - getWriteHeight () + 1 + lineOffset;
	else if ((selrow - padoff_y) < 0)
		padoff_y = selrow;
	if (haveBox ())
		pnoutrefresh (scrolpad, padoff_y, padoff_x, y + 1, x + 1, y + h - 2, x + w - 2);
	else
		pnoutrefresh (scrolpad, padoff_y, padoff_x, y, x, y + h - 1, x + w - 1);
}

Rts2NDevListWindow::Rts2NDevListWindow (Rts2Block * in_block):Rts2NSelWindow (0, 1, 10, LINES - 20, 1, 50, 300)
{
	block = in_block;
}

Rts2NDevListWindow::~Rts2NDevListWindow (void)
{
}

void Rts2NDevListWindow::draw ()
{
	Rts2NWindow::draw ();
	werase (scrolpad);
	maxrow = 0;
	connections_t::iterator iter;
	for (iter = block->getCentraldConns ()->begin (); iter != block->getCentraldConns ()->end (); iter++)
	{
		wprintw (scrolpad, "centrald\n");
		maxrow++;
	}
	for (iter = block->getConnections ()->begin (); iter != block->getConnections ()->end (); iter++)
	{
		Rts2Conn *conn = *iter;
		wprintw (scrolpad, "%s\n", conn->getName ());
		maxrow++;
	}
	wprintw (scrolpad, "status");
	maxrow++;
	refresh ();
}

void Rts2NCentraldWindow::printState (Rts2Conn * conn)
{
	if (conn->getErrorState ())
		wcolor_set (getWriteWindow (), CLR_FAILURE, NULL);
	wprintw (getWriteWindow (), "%s %s (%x)\n", conn->getName (),
		conn->getStateString ().c_str (), conn->getState ());
	wcolor_set (getWriteWindow (), CLR_DEFAULT, NULL);
}

void Rts2NCentraldWindow::drawDevice (Rts2Conn * conn)
{
	printState (conn);
	maxrow++;
}

Rts2NCentraldWindow::Rts2NCentraldWindow (Rts2Client * in_client):Rts2NSelWindow (10, 1, COLS - 10, LINES - 25, 1, 300, 300)
{
	client = in_client;
	draw ();
}

Rts2NCentraldWindow::~Rts2NCentraldWindow (void)
{
}

void Rts2NCentraldWindow::draw ()
{
	Rts2NSelWindow::draw ();
	werase (getWriteWindow ());
	maxrow = 0;
	connections_t::iterator iter;
	for (iter = client->getCentraldConns ()->begin (); iter != client->getCentraldConns ()->end (); iter++)
	{
		drawDevice (*iter);
	}
	for (iter = client->getConnections ()->begin (); iter != client->getConnections ()->end (); iter++)
	{
		drawDevice (*iter);
	}
	refresh ();
}
