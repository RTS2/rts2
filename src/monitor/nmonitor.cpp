#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <curses.h>
#include <libnova/libnova.h>
#include <getopt.h>
#include <panel.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <pthread.h>
#include <limits.h>

#include <iostream>
#include <fstream>

#include "../utils/rts2block.h"
#include "../utils/rts2client.h"
#include "../utils/rts2command.h"

#define LINE_SIZE	13
#define COL_SIZE	25

#define CLR_OK		1
#define CLR_TEXT	2
#define CLR_PRIORITY	3
#define CLR_WARNING	4
#define CLR_FAILURE	5
#define CLR_STATUS      6

#define CMD_BUF_LEN    100

class Rts2CNMonConn:public Rts2ConnClient
{
private:
  int nrow;
  WINDOW *window;
  int printStatus ();

public:
    Rts2CNMonConn (Rts2Block * in_master,
		   char *in_name):Rts2ConnClient (in_master, in_name)
  {
    window = NULL;
  }
  virtual ~ Rts2CNMonConn (void)
  {
    delwin (window);
  }
  int hasWindow ()
  {
    return window != NULL;
  }
  int setWindow (WINDOW * in_window)
  {
    if (hasWindow ())
      delwin (window);
    window = in_window;
    print ();
  }

  int print ();

  virtual int commandReturn (Rts2Command * command, int status)
  {
    print ();
    return 0;
  }
};

int
Rts2CNMonConn::printStatus ()
{
  for (int i = 0; i < MAX_STATE; i++)
    {
      Rts2ServerState *state = serverState[i];
      if (state)
	{
	  mvwprintw (window, nrow, 0, "%s : %i", state->name, state->value);
	  nrow++;
	}
    }
  return 0;
}

int
Rts2CNMonConn::print ()
{
  int ret;
  if (has_colors ())
    wcolor_set (window, CLR_OK, NULL);
  else
    wstandout (window);

  mvwprintw (window, 0, 0, "== %-10s ==", getName ());

  if (has_colors ())
    wcolor_set (window, CLR_TEXT, NULL);
  else
    wstandend (window);

  nrow = 1;
  if (!otherDevice)
    {
      mvwprintw (window, 1, 0, "  UNKNOW DEV");
    }
  else
    {
      // print values
      std::vector < Rts2Value * >::iterator val_iter;
      for (val_iter = otherDevice->values.begin ();
	   val_iter != otherDevice->values.end (); val_iter++, nrow++)
	{
	  Rts2Value *val = (*val_iter);
	  char *buf;
	  val->getValue (&buf);
	  mvwprintw (window, nrow, 0, " %s : %s", val->getName (), buf);
	  delete buf;
	}
    }
  ret = printStatus ();
  wrefresh (window);
  return ret;
}

/*******************************************************
 *
 * This class hold "root" window of display,
 * takes care about displaying it's connection etc..
 *
 ******************************************************/

class Rts2NMonitor:public Rts2Client
{
private:
  WINDOW * statusWindow;
  WINDOW *commandWindow;
    std::vector < Rts2CNMonConn * >clientConnections;
  int connCols;
  int connLines;
  int cmd_col;
  char cmd_buf[CMD_BUF_LEN];
protected:
  void relocatesWindows ();
  void processConnection (Rts2CNMonConn * conn);
  virtual Rts2ConnClient *createClientConnection (char *in_deviceName)
  {
    Rts2CNMonConn *conn;
      conn = new Rts2CNMonConn (this, in_deviceName);
      processConnection (conn);
      return conn;
  }

public:
    Rts2NMonitor (int argc, char **argv);
  virtual ~ Rts2NMonitor (void);

  int paintWindows ();
  int repaint ();

  virtual int init ();
  virtual int idle ();

  void processKey (int key);

  virtual int addAddress (Rts2Address * in_addr);

  int messageBox (char *message);
};

void
Rts2NMonitor::relocatesWindows ()
{
  int win_num;
  WINDOW *connWin;
  std::vector < Rts2CNMonConn * >::iterator conn_iter;
  Rts2CNMonConn *conn;
  win_num = clientConnections.size ();

  // allocate window for our connection
  for (conn_iter = clientConnections.begin ();
       conn_iter != clientConnections.end (); conn_iter++)
    {
      conn = (*conn_iter);
      if (!conn->hasWindow ())
	{
	  connWin =
	    newwin (LINES / 2, COLS / 4, 1 + LINES / 4 * (win_num / 4),
		    COLS / 4 * ((win_num - 1) % 4));
	  conn->setWindow (connWin);
	}
    }
}

void
Rts2NMonitor::processConnection (Rts2CNMonConn * conn)
{
  clientConnections.push_back (conn);
  relocatesWindows ();
  conn->queCommand (new Rts2Command (this, "base_info"));
  conn->queCommand (new Rts2Command (this, "info"));
}

Rts2NMonitor::Rts2NMonitor (int argc, char **argv):
Rts2Client (argc, argv)
{
  statusWindow = NULL;
  commandWindow = NULL;
  connCols = 0;
  connLines = 0;
  cmd_col = 0;
}

Rts2NMonitor::~Rts2NMonitor (void)
{
  delwin (statusWindow);
  delwin (commandWindow);

  erase ();
  refresh ();

  nocbreak ();
  echo ();
  endwin ();
}

int
Rts2NMonitor::paintWindows ()
{
  curs_set (0);
  // prepare main window..
  if (statusWindow)
    delwin (statusWindow);
  if (commandWindow)
    delwin (commandWindow);
  statusWindow = newwin (1, COLS, LINES - 1, 0);
  wbkgdset (statusWindow, A_BOLD | COLOR_PAIR (CLR_STATUS));
  commandWindow = newwin (1, COLS, 0, 0);
  wbkgdset (commandWindow, A_BOLD | COLOR_PAIR (CLR_STATUS));
  waddch (commandWindow, 'C');
  curs_set (1);
}

int
Rts2NMonitor::repaint ()
{
  char dateBuf[40];
  time_t now;
  time (&now);

  wcolor_set (statusWindow, CLR_STATUS, NULL);
  mvwprintw (statusWindow, 0, 0, "** Status: %i ** ", getMasterState ());
  wcolor_set (statusWindow, CLR_TEXT, NULL);
  strftime (dateBuf, 40, "%c", gmtime (&now));
  mvwprintw (statusWindow, 0, COLS - 40, "%40s", dateBuf);
  refresh ();
  wrefresh (statusWindow);
  wrefresh (commandWindow);
  return 0;
}

int
Rts2NMonitor::init ()
{
  int ret;
  ret = Rts2Client::init ();
  if (ret)
    return ret;

  if (!initscr ())
    {
      std::cerr << "ncurses not supported (initscr call failed)!" << std::
	endl;
      return -1;
    }

  connCols = COLS / COL_SIZE;
  connLines = (LINES - 2) / LINE_SIZE;

  cbreak ();
  keypad (stdscr, TRUE);
  noecho ();

  start_color ();

  if (has_colors ())
    {
      init_pair (CLR_OK, COLOR_GREEN, 0);
      init_pair (CLR_TEXT, COLOR_WHITE, 0);
      init_pair (CLR_PRIORITY, COLOR_CYAN, 0);
      init_pair (CLR_WARNING, COLOR_RED, 0);
      init_pair (CLR_FAILURE, COLOR_YELLOW, 0);
      init_pair (CLR_STATUS, COLOR_RED, COLOR_CYAN);
    }
  getCentraldConn ()->queCommand (new Rts2Command (this, "info"));
  paintWindows ();
  return repaint ();
}

int
Rts2NMonitor::idle ()
{
  repaint ();
  setTimeout (USEC_SEC);
  return Rts2Client::idle ();
}

void
Rts2NMonitor::processKey (int key)
{
  switch (key)
    {
    case KEY_F (2):
      if (messageBox ("Are you sure to switch off (y/n)?") == 'y')
	getCentraldConn ()->queCommand (new Rts2Command (this, "off"));
      break;
    case KEY_F (3):
      if (messageBox ("Are you sure to switch to standby (y/n)?") == 'y')
	getCentraldConn ()->queCommand (new Rts2Command (this, "standby"));
      break;
    case KEY_F (4):
      if (messageBox ("Are you sure to switch to on (y/n)?") == 'y')
	getCentraldConn ()->queCommand (new Rts2Command (this, "on"));
      break;
    case KEY_F (5):
      queAll ("info");
      break;
    case KEY_F (10):
      endRunLoop ();
      break;
    case KEY_BACKSPACE:
      if (cmd_col > 0)
	{
	  cmd_col--;
	  wmove (commandWindow, 0, cmd_col + 1);
	  wdelch (commandWindow);
	  wrefresh (commandWindow);
	  break;
	}
      beep ();
      break;
    case KEY_ENTER:
    case '\n':

      break;
    default:
      if (cmd_col >= CMD_BUF_LEN)
	{
	  beep ();
	  break;
	}
      cmd_buf[cmd_col++] = key;
      wechochar (commandWindow, key);
      break;
    }
}

int
Rts2NMonitor::addAddress (Rts2Address * in_addr)
{
  int ret;
  ret = Rts2Client::addAddress (in_addr);
  if (ret)
    return ret;
  // add that device to our list - create connection for it
  Rts2Conn *conn;
  conn = getOpenConnection (in_addr->getName ());
  if (!conn)
    {
      conn = Rts2Client::createClientConnection (in_addr);
      addConnection (conn);
    }
  repaint ();
  return ret;
}

int
Rts2NMonitor::messageBox (char *message)
{
  int key = 0;
  WINDOW *wnd = newwin (5, COLS / 2, LINES / 2, COLS / 2 - COLS / 4);
  PANEL *msg_pan = new_panel (wnd);
  wcolor_set (wnd, CLR_WARNING, NULL);
  box (wnd, 0, 0);
  wcolor_set (wnd, CLR_TEXT, NULL);
  mvwprintw (wnd, 2, 2 + (COLS / 2 - strlen (message)) / 2, message);
  wrefresh (wnd);
  key = wgetch (wnd);
  del_panel (msg_pan);
  delwin (wnd);
  update_panels ();
  doupdate ();
  return key;
}

Rts2NMonitor *monitor = NULL;

// signal and keyboard handlers..

void
sigExit (int sig)
{
  delete monitor;
  exit (0);
}

void *
inputThread (void *arg)
{
  int key;
  while (1)
    {
      key = getch ();
      monitor->processKey (key);
    }
}

int
main (int argc, char **argv)
{
  int ret;
  pthread_t keyThread;
  pthread_attr_t keyThreadAttr;

  monitor = new Rts2NMonitor (argc, argv);

  signal (SIGINT, sigExit);
  signal (SIGTERM, sigExit);

  ret = monitor->init ();
  if (ret)
    {
      delete monitor;
      exit (0);
    }
  pthread_attr_init (&keyThreadAttr);
  pthread_attr_setstacksize (&keyThreadAttr, PTHREAD_STACK_MIN * 6);
  pthread_create (&keyThread, &keyThreadAttr, inputThread, NULL);
  monitor->run ();

  delete (monitor);

  return 0;
}
