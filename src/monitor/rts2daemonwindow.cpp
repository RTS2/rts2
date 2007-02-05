#include "rts2daemonwindow.h"
#include "nmonitor.h"

#include <iostream>

Rts2NWindow::Rts2NWindow (WINDOW * master_window, int x, int y, int w, int h,
			  int border):
Rts2NLayout ()
{
  if (h <= 0)
    h = 0;
  if (w <= 0)
    w = 0;
  switch (border)
    {
    case 0:
      window = newwin (h, w, y, x);
      _haveBox = false;
      break;
    case 1:
      window = newwin (h, w, y, x);
      box (window, 0, 0);
      _haveBox = true;
      break;
    }
}

Rts2NWindow::~Rts2NWindow (void)
{
  delwin (window);
}

void
Rts2NWindow::draw ()
{
  werase (window);
  if (haveBox ())
    {
      box (window, 0, 0);
    }
}

int
Rts2NWindow::getX ()
{
  int x, y;
  getbegyx (window, y, x);
  return x;
}

int
Rts2NWindow::getY ()
{
  int x, y;
  getbegyx (window, y, x);
  return y;
}

int
Rts2NWindow::getCurX ()
{
  int x, y;
  getyx (window, y, x);
  return x + getX ();
}

int
Rts2NWindow::getCurY ()
{
  int x, y;
  getyx (window, y, x);
  return y + getY ();
}


int
Rts2NWindow::getWidth ()
{
  int w, h;
  getmaxyx (window, h, w);
  return w;
}

int
Rts2NWindow::getHeight ()
{
  int w, h;
  getmaxyx (window, h, w);
  return h;
}

void
Rts2NWindow::errorMove ()
{
  endwin ();
  std::cout << "Cannot move" << std::endl;
  exit (EXIT_FAILURE);
}

void
Rts2NWindow::move (int x, int y)
{
  int _x, _y;
  getbegyx (window, _y, _x);
  if (x == _x && y == _y)
    return;
  if (mvwin (window, y, x) == ERR)
    errorMove ();
}

void
Rts2NWindow::resize (int x, int y, int w, int h)
{
  wresize (window, h, w);
  move (x, y);
}

void
Rts2NWindow::grow (int max_w, int h_dif)
{
  int x, y, w, h;
  getbegyx (window, y, x);
  getmaxyx (window, h, w);
  if (max_w > w)
    w = max_w;
  resize (x, y, w, h + h_dif);
}

void
Rts2NWindow::refresh ()
{
  wnoutrefresh (window);
}

void
Rts2NWindow::enter ()
{
}

void
Rts2NWindow::leave ()
{
}

void
Rts2NWindow::setCursor ()
{
  setsyx (getCurY (), getCurX ());
}

void
Rts2NWindow::printState (Rts2Conn * conn)
{
  if (conn->getErrorState ())
    wcolor_set (getWriteWindow (), CLR_FAILURE, NULL);
  else if (conn->havePriority ())
    wcolor_set (getWriteWindow (), CLR_OK, NULL);
  wprintw (getWriteWindow (), "%s %s (%i) priority: %s\n", conn->getName (),
	   conn->getStateString ().c_str (), conn->getState (),
	   conn->havePriority ()? "yes" : "no");
  wcolor_set (getWriteWindow (), CLR_DEFAULT, NULL);
}

Rts2NSelWindow::Rts2NSelWindow (WINDOW * master_window, int x, int y, int w,
				int h, int border):
Rts2NWindow (master_window, x, y, w, h, border)
{
  selrow = 0;
  maxrow = 0;
  padoff_x = 0;
  padoff_y = 0;
  scrolpad = newpad (100, 300);
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
      selrow = maxrow - 1;
      break;
    case KEY_DOWN:
      if (selrow < (maxrow - 1))
	selrow++;
      else
	selrow = 0;
      break;
    case KEY_UP:
      if (selrow > 0)
	selrow--;
      else
	selrow = (maxrow - 1);
      break;
    }
  draw ();
  return -1;
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
  getbegyx (window, y, x);
  getmaxyx (window, h, w);
  if (haveBox ())
    pnoutrefresh (scrolpad, 0, 0, y + 1, x + 1, y + h - 2, x + w - 2);
  else
    pnoutrefresh (scrolpad, 0, 0, y, x, y + h - 1, x + w - 1);
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

void
Rts2NDevListWindow::draw ()
{
  Rts2NWindow::draw ();
  werase (scrolpad);
  maxrow = 0;
  for (connections_t::iterator iter = block->connectionBegin ();
       iter != block->connectionEnd (); iter++)
    {
      Rts2Conn *conn = *iter;
      const char *name = conn->getName ();
      if (*name == '\0')
	wprintw (scrolpad, "status\n");
      else
	wprintw (scrolpad, "%s\n", conn->getName ());
      maxrow++;
    }
  refresh ();
}

Rts2NDeviceWindow::Rts2NDeviceWindow (WINDOW * master_window, Rts2Conn * in_connection):Rts2NSelWindow
  (master_window, 10, 1, COLS - 10,
   LINES - 25)
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
      wprintw (getWriteWindow (), "%-20s|%30s\n", val->getName ().c_str (),
	       val->getValue ());
      maxrow++;
    }
}

void
Rts2NDeviceWindow::draw ()
{
  Rts2NWindow::draw ();
  werase (getWriteWindow ());
  maxrow = 1;
  printState (connection);
  drawValuesList ();
  refresh ();
}

void
Rts2NCentraldWindow::drawDevice (Rts2Conn * conn)
{
  printState (conn);
  maxrow++;
}

Rts2NCentraldWindow::Rts2NCentraldWindow (WINDOW * master_window, Rts2Client * in_client):Rts2NSelWindow
  (master_window, 10, 1, COLS - 10,
   LINES - 25)
{
  client = in_client;
  draw ();
}

Rts2NCentraldWindow::~Rts2NCentraldWindow (void)
{
}

void
Rts2NCentraldWindow::draw ()
{
  Rts2NSelWindow::draw ();
  werase (getWriteWindow ());
  maxrow = 0;
  for (connections_t::iterator iter = client->connectionBegin ();
       iter != client->connectionEnd (); iter++)
    {
      Rts2Conn *conn = *iter;
      drawDevice (conn);
    }
  refresh ();
}
