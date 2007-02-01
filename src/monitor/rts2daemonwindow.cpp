#include "rts2daemonwindow.h"

#include <iostream>

Rts2NWindow::Rts2NWindow (WINDOW * master_window, int x, int y, int w, int h,
			  int border)
{
  switch (border)
    {
    case 0:
      boxwin = NULL;
      window = newwin (h, w, y, x);
      break;
    case 1:
      boxwin = newwin (h, w, y, x);
      box (boxwin, 0, 0);
      window = derwin (boxwin, h - 2, w - 2, 1, 1);
      break;
    }
}

Rts2NWindow::~Rts2NWindow (void)
{
  if (boxwin)
    delwin (boxwin);
  else
    delwin (window);
}

void
Rts2NWindow::draw ()
{
  if (boxwin)
    {
      box (boxwin, 0, 0);
    }
}

void
Rts2NWindow::refresh ()
{
  if (boxwin)
    {
      wnoutrefresh (boxwin);
    }
  else
    {
      wnoutrefresh (window);
    }
}

Rts2NSelWindow::Rts2NSelWindow (WINDOW * master_window, int x, int y, int w,
				int h, int border):
Rts2NWindow (master_window, x, y, w, h, border)
{
  selrow = 0;
  maxrow = 0;
  padoff_x = 0;
  padoff_y = 0;
//  scrolpad = newpad (100, 300);
  scrolpad = window;
}

Rts2NSelWindow::~Rts2NSelWindow (void)
{
}

int
Rts2NSelWindow::injectKey (int key)
{
  switch (key)
    {
    case KEY_HOME:
      selrow = 0;
      break;
    case KEY_END:
      selrow = maxrow;
      break;
    case KEY_DOWN:
      selrow++;
      break;
    case KEY_UP:
      if (selrow > 0)
	selrow--;
      break;
    }
  draw ();
  return 0;
}

void
Rts2NSelWindow::refresh ()
{
  int x, y;
  int w, h;
  if (selrow >= 0)
    {
      getmaxyx (scrolpad, h, w);
      mvwchgat (scrolpad, selrow, 0, w, A_REVERSE, 0, NULL);
    }
  Rts2NWindow::refresh ();
//  getbegyx (window, y, x);
//  getmaxyx (window, h, w);
//  pnoutrefresh (scrolpad, 0, 0, y, x, y+h-2, x+w-2);
}

Rts2NDevListWindow::Rts2NDevListWindow (WINDOW * master_window, Rts2Block * in_block):Rts2NSelWindow (master_window, 0, 1, 10,
		LINES -
		20)
{
  block = in_block;
}

Rts2NDevListWindow::~Rts2NDevListWindow (void)
{
}

int
Rts2NDevListWindow::injectKey (int key)
{
  return Rts2NSelWindow::injectKey (key);
}

void
Rts2NDevListWindow::draw ()
{
  Rts2NWindow::draw ();
  wclear (scrolpad);
  maxrow = 0;
  for (connections_t::iterator iter = block->connectionBegin ();
       iter != block->connectionEnd (); iter++)
    {
      Rts2Conn *conn = *iter;
      wprintw (scrolpad, "%s\n", conn->getName ());
      maxrow++;
    }
  refresh ();
}

Rts2NDeviceWindow::Rts2NDeviceWindow (WINDOW * master_window, Rts2Conn * in_connection):Rts2NSelWindow
  (master_window, 10, 1, COLS - 10,
   LINES - 20)
{
  connection = in_connection;
  draw ();
}

Rts2NDeviceWindow::~Rts2NDeviceWindow ()
{
}

void
Rts2NDeviceWindow::drawValuesList ()
{
  Rts2DevClient *client = connection->getOtherDevClient ();
  if (client)
    drawValuesList (client);
}

void
Rts2NDeviceWindow::drawValuesList (Rts2DevClient * client)
{
  for (std::vector < Rts2Value * >::iterator iter = client->valueBegin ();
       iter != client->valueEnd (); iter++)
    {
      Rts2Value *val = *iter;
      wprintw (scrolpad, "%-20s|%30s\n", val->getName ().c_str (),
	       val->getValue ());
      maxrow++;
    }
}

int
Rts2NDeviceWindow::injectKey (int key)
{
  return Rts2NSelWindow::injectKey (key);
}

void
Rts2NDeviceWindow::draw ()
{
  Rts2NWindow::draw ();
  werase (scrolpad);
  drawValuesList ();
  refresh ();
}

void
Rts2NCentraldWindow::drawDevice (Rts2Conn * conn)
{
  wprintw (window, "%s %s\n", conn->getName (),
	   conn->getStateString ().c_str ());
}

Rts2NCentraldWindow::Rts2NCentraldWindow (WINDOW * master_window, Rts2Client * in_client):Rts2NWindow
  (master_window, 10, 1, COLS - 10,
   LINES - 20)
{
  client = in_client;
  draw ();
}

Rts2NCentraldWindow::~Rts2NCentraldWindow (void)
{
}

int
Rts2NCentraldWindow::injectKey (int key)
{
  return 0;
}

void
Rts2NCentraldWindow::draw ()
{
  Rts2NWindow::draw ();
  werase (window);
  for (connections_t::iterator iter = client->connectionBegin ();
       iter != client->connectionEnd (); iter++)
    {
      Rts2Conn *conn = *iter;
      drawDevice (conn);
    }
  refresh ();
}
