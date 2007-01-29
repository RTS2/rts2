extern "C"
{
#include <cdk/cdk.h>
};

#include <libnova/libnova.h>
#include <getopt.h>
#include <panel.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <iostream>
#include <fstream>

#include "../utils/rts2block.h"
#include "../utils/rts2client.h"
#include "../utils/rts2command.h"

#include "../writers/rts2image.h"
#include "../writers/rts2devcliimg.h"

#ifdef HAVE_XCURSES
char *XCursesProgramName = "rts2-mon";
#endif

#define LINE_SIZE	13
#define COL_SIZE	25

#define CLR_OK		1
#define CLR_TEXT	2
#define CLR_PRIORITY	3
#define CLR_WARNING	4
#define CLR_FAILURE	5
#define CLR_STATUS      6

#define CMD_BUF_LEN    100

#define EVENT_PRINT		RTS2_LOCAL_EVENT + 1
#define EVENT_PRINT_SHORT	RTS2_LOCAL_EVENT + 2
#define EVENT_PRINT_FULL	RTS2_LOCAL_EVENT + 3
#define EVENT_PRINT_MESSAGES	RTS2_LOCAL_EVENT + 4

class Rts2CNMonConn:public Rts2ConnClient
{
public:
  Rts2CNMonConn (Rts2Block * in_master,
		 char *in_name):Rts2ConnClient (in_master, in_name)
  {
  }
  virtual ~ Rts2CNMonConn (void)
  {
  }

  virtual int commandReturn (Rts2Command * cmd, int in_status)
  {
    update_panels ();
    return 0;
  }

  virtual int command ()
  {
    // for immediate updates of values..
    int ret;
    ret = Rts2ConnClient::command ();
    update_panels ();
    return ret;
  }

  virtual int setState (int in_value)
  {
    int ret;
    ret = Rts2ConnClient::setState (in_value);
    update_panels ();
    return ret;
  }

  virtual void priorityChanged ()
  {
    Rts2ConnClient::priorityChanged ();
    update_panels ();
  }
};

class MonWindow:public Rts2DevClient
{
private:
  int printFormat;
  int statusBegin;
protected:
  int printStatus (WINDOW * window, Rts2Conn * in_connection);

  virtual void print (WINDOW * wnd)
  {
  }
  virtual void printOneLine (WINDOW * wnd)
  {
  }
public:
MonWindow (Rts2Conn * in_connection):Rts2DevClient (in_connection)
  {
    printFormat = EVENT_PRINT_FULL;
    statusBegin = 1;
  }

  void postEvent (Rts2Event * event);
  void printTimeDiff (WINDOW * window, int row, char *text, double diff);

  void setStatusBegin (int in_status_begin)
  {
    statusBegin = in_status_begin;
  }
};

int
MonWindow::printStatus (WINDOW * window, Rts2Conn * in_connection)
{
  Rts2ServerState *state = in_connection->getStateObject ();
  mvwprintw (window, statusBegin, 0, "Status: %-5i", state->getValue ());
  mvwprintw (window, statusBegin + 1, 0, "Priority: %s",
	     in_connection->havePriority ()? "yes" : "no");
  return 0;
}

void
MonWindow::postEvent (Rts2Event * event)
{
  PANEL *panel;
  switch (event->getType ())
    {
    case EVENT_PRINT:
      panel = (PANEL *) event->getArg ();
      switch (printFormat)
	{
	case EVENT_PRINT_SHORT:
	  show_panel (panel);
	  printOneLine (panel_window (panel));
	  break;
	case EVENT_PRINT_FULL:
	  show_panel (panel);
	  print (panel_window (panel));
	  break;
	case EVENT_PRINT_MESSAGES:
	  hide_panel (panel);
	  break;
	}
      break;
    case EVENT_PRINT_SHORT:
    case EVENT_PRINT_FULL:
    case EVENT_PRINT_MESSAGES:
      printFormat = event->getType ();
    }
}

void
MonWindow::printTimeDiff (WINDOW * window, int row, char *text, double diff)
{
  char time_buf[50];
  struct ln_hms hms;
  time_buf[0] = '\0';
  double t = fabs (diff);
  if (t > 86400)
    {
      sprintf (time_buf, "%li days ", (long) (t / 86400));
      t = (long) t % 86400;
    }
  // that will delete sign..
  ln_deg_to_hms (360.0 * ((double) fabs (t) / 86400.0), &hms);
  sprintf (time_buf, "%s%c%i:%02i:%04.1f", time_buf, diff > 0 ? '+' : '-',
	   hms.hours, hms.minutes, hms.seconds);
  mvwprintw (window, row, 1, "%s:%s", text, time_buf);
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
  CDKSCREEN *cdkscreen;
  CDKALPHALIST *deviceList;
  CDKALPHALIST *valueList;
  void *activeEntry;
  CDKSWINDOW *msgwindow;
    std::vector < Rts2CNMonConn * >clientConnections;
  int cmd_col;
  char cmd_buf[CMD_BUF_LEN];

  void executeCommand ();
  void relocatesWindows ();
  void processConnection (Rts2CNMonConn * conn);

  int paintWindows ();
  int repaint ();

  int messageBox (char *message);

  void refreshCommandWindow ()
  {
    mvwprintw (commandWindow, 0, 0, "%-*s", COLS, cmd_buf);
    wrefresh (commandWindow);
    repaint ();
  }

  int printType;

  void drawValuesList ();
  void drawValuesList (Rts2DevClient * client);

protected:
  virtual Rts2ConnClient * createClientConnection (char *in_deviceName)
  {
    Rts2CNMonConn *conn;
    conn = new Rts2CNMonConn (this, in_deviceName);
    processConnection (conn);
    return conn;
  }

  virtual void addSelectSocks (fd_set * read_set);
  virtual void selectSuccess (fd_set * read_set);

  virtual int addAddress (Rts2Address * in_addr);
public:
  Rts2NMonitor (int argc, char **argv);
  virtual ~ Rts2NMonitor (void);

  virtual int init ();
  virtual int idle ();

  virtual void postEvent (Rts2Event * event);

  virtual int deleteConnection (Rts2Conn * conn);

  virtual Rts2DevClient *createOtherType (Rts2Conn * conn,
					  int other_device_type);

  virtual int willConnect (Rts2Address * in_addr);

  virtual void message (Rts2Message & msg);

  int resize ();

  void processKey (int key);
};

void
Rts2NMonitor::executeCommand ()
{
  char *cmd_sep;
  Rts2Conn *cmd_conn;
  // try to parse..
  cmd_sep = index (cmd_buf, '.');
  if (cmd_sep)
    {
      *cmd_sep = '\0';
      cmd_sep++;
      cmd_conn = getConnection (cmd_buf);
    }
  else
    {
      cmd_conn = getCentraldConn ();
      cmd_sep = cmd_buf;
    }
  cmd_conn->queCommand (new Rts2Command (this, cmd_sep));
  cmd_col = 0;
  cmd_buf[0] = '\0';
  refreshCommandWindow ();
}

void
Rts2NMonitor::relocatesWindows ()
{
  update_panels ();
}

void
Rts2NMonitor::processConnection (Rts2CNMonConn * conn)
{
  clientConnections.push_back (conn);
  relocatesWindows ();
}

void
Rts2NMonitor::drawValuesList ()
{
  Rts2Conn *conn = getConnection (deviceList->entryField->info);
  if (conn)
    {
      Rts2DevClient *client = conn->getOtherDevClient ();
      if (client)
	drawValuesList (client);
    }
}

void
Rts2NMonitor::drawValuesList (Rts2DevClient * client)
{
  int i = 0;
  char *new_list[client->valueSize ()];
  for (std::vector < Rts2Value * >::iterator iter = client->valueBegin ();
       iter != client->valueEnd (); iter++, i++)
    {
      Rts2Value *val = *iter;
      asprintf (&new_list[i], "%-20s|%30s", val->getName ().c_str (),
		val->getValue ());
    }
  setCDKAlphalistContents (valueList, (char **) new_list,
			   client->valueSize ());
  for (i = 0; i < client->valueSize (); i++)
    {
      free (new_list[i]);
    }
}

void
Rts2NMonitor::addSelectSocks (fd_set * read_set)
{
  // add stdin for ncurses input
  FD_SET (1, read_set);
  Rts2Client::addSelectSocks (read_set);
}

void
Rts2NMonitor::selectSuccess (fd_set * read_set)
{
  Rts2Client::selectSuccess (read_set);
  if (FD_ISSET (1, read_set))
    {
      chtype input = getch ();
      switch (input)
	{
	case KEY_STAB:
	case '\t':
	  if (activeEntry == deviceList->entryField)
	    activeEntry = valueList->entryField;
	  else if (activeEntry == valueList->entryField)
	    activeEntry = msgwindow;
	  else
	    activeEntry = deviceList->entryField;
	  break;
	default:
	  if (activeEntry == msgwindow)
	    injectCDKSwindow (msgwindow, input);
	  else
	    injectCDKEntry ((CDKENTRY *) activeEntry, input);
	}
      // draw device values
      if (activeEntry == deviceList->entryField
	  && deviceList->entryField->info)
	{
	  drawValuesList ();
	}
    }
}

int
Rts2NMonitor::addAddress (Rts2Address * in_addr)
{
  int i = 0;
  int ret;
  ret = Rts2Client::addAddress (in_addr);
  if (ret)
    return ret;
  const char *new_list[connectionSize ()];
  for (connections_t::iterator iter = connectionBegin ();
       iter != connectionEnd (); iter++, i++)
    {
      Rts2Conn *conn = *iter;
      new_list[i] = conn->getName ();
    }
  setCDKAlphalistContents (deviceList, (char **) new_list, connectionSize ());
  return 0;
}

Rts2NMonitor::Rts2NMonitor (int in_argc, char **in_argv):
Rts2Client (in_argc, in_argv)
{
  statusWindow = NULL;
  commandWindow = NULL;
  cdkscreen = NULL;
  deviceList = NULL;
  valueList = NULL;
  msgwindow = NULL;
  cmd_col = 0;
  printType = EVENT_PRINT_FULL;
}

Rts2NMonitor::~Rts2NMonitor (void)
{
  endCDK ();
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
  return 0;
}

int
Rts2NMonitor::repaint ()
{
  char dateBuf[40];
  time_t now;
  time (&now);

  wcolor_set (statusWindow, CLR_STATUS, NULL);

  mvwprintw (statusWindow, 0, 0, "** Status: %s ** %i ",
	     getMasterStateString ().c_str (),
	     deviceList->scrollField->currentItem);
  wcolor_set (statusWindow, CLR_TEXT, NULL);
  strftime (dateBuf, 40, "%Y-%m-%dT%H:%M:%S", gmtime (&now));
  mvwprintw (statusWindow, 0, COLS - 40, "%40s", dateBuf);
//  refresh ();
  wrefresh (statusWindow);
  wrefresh (commandWindow);
  drawCDKSwindow (msgwindow, TRUE);
  return 0;
}

int
Rts2NMonitor::init ()
{
  int ret;
  WINDOW *cursesWin;
  ret = Rts2Client::init ();
  if (ret)
    return ret;

  cursesWin = initscr ();
  if (!cursesWin)
    {
      std::cerr << "ncurses not supported (initscr call failed)!" << std::
	endl;
      return -1;
    }
  sighandler_t oldCore = signal (SIGSEGV, SIG_IGN);
  cdkscreen = initCDKScreen (cursesWin);
  signal (SIGSEGV, oldCore);

  initCDKColor ();

  deviceList =
    newCDKAlphalist (cdkscreen, 0, 0, LINES - 21, 20, "<C></B/24>Device list",
		     NULL, NULL, 0, '_', A_REVERSE, TRUE, FALSE);

  valueList =
    newCDKAlphalist (cdkscreen, 20, 0, LINES - 21, COLS - 20,
		     "<C></B/24>Value list", NULL, NULL, 0, '_', A_REVERSE,
		     TRUE, FALSE);

  msgwindow =
    newCDKSwindow (cdkscreen, 0, LINES - 18, 18, COLS, "Messages", 1000, TRUE,
		   FALSE);

  activeEntry = deviceList->entryField;

  cbreak ();
  keypad (stdscr, TRUE);
  noecho ();

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
  setMessageMask (MESSAGE_MASK_ALL);
  paintWindows ();

  drawCDKAlphalist (deviceList, TRUE);
  drawCDKAlphalist (valueList, TRUE);
  drawCDKSwindow (msgwindow, msgwindow->box);

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
Rts2NMonitor::postEvent (Rts2Event * event)
{
  switch (event->getType ())
    {
    case EVENT_PRINT_FULL:
    case EVENT_PRINT_SHORT:
    case EVENT_PRINT_MESSAGES:
      printType = event->getType ();
      break;
    }
  Rts2Client::postEvent (event);
  // ::postEvent deletes event
}

int
Rts2NMonitor::deleteConnection (Rts2Conn * conn)
{
  // try to find us among clientConnections
  std::vector < Rts2CNMonConn * >::iterator conn_iter;
  Rts2CNMonConn *tmp_conn;

  // allocate window for our connection
  for (conn_iter = clientConnections.begin ();
       conn_iter != clientConnections.end (); conn_iter++)
    {
      tmp_conn = (*conn_iter);
      if (tmp_conn == conn)
	{
	  clientConnections.erase (conn_iter);
	  break;
	}
    }
  return Rts2Client::deleteConnection (conn);
}

Rts2DevClient *
Rts2NMonitor::createOtherType (Rts2Conn * conn, int other_device_type)
{
  return new MonWindow (conn);
}

void
Rts2NMonitor::message (Rts2Message & msg)
{
  char *buf;
  char *fmt;
  switch (msg.getType ())
    {
    case MESSAGE_ERROR:
      fmt = "<L></6></B>";
      break;
    case MESSAGE_WARNING:
      fmt = "<L></5>";
      break;
    case MESSAGE_INFO:
      fmt = "<L></4>";
      break;
    case MESSAGE_DEBUG:
      fmt = "<L></3>";
      break;
    }
  asprintf (&buf, "%s%s", fmt, msg.toString ().c_str ());
  addCDKSwindow (msgwindow, buf, BOTTOM);
  free (buf);
}

int
Rts2NMonitor::resize ()
{
  erase ();
  endwin ();
  initscr ();
  relocatesWindows ();
  paintWindows ();
  postEvent (new Rts2Event (EVENT_PRINT));
  return repaint ();
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
    case KEY_F (8):
      switch (printType)
	{
	case EVENT_PRINT_MESSAGES:
	  postEvent (new Rts2Event (EVENT_PRINT_FULL));
	  break;
	default:
	  postEvent (new Rts2Event (EVENT_PRINT_MESSAGES));
	  break;
	}
      update_panels ();
      break;
    case KEY_F (9):
      switch (printType)
	{
	case EVENT_PRINT_FULL:
	  postEvent (new Rts2Event (EVENT_PRINT_SHORT));
	  break;
	default:
	  postEvent (new Rts2Event (EVENT_PRINT_SHORT));
	  break;
	}
      update_panels ();
      break;
    case KEY_F (10):
      endRunLoop ();
      break;
    case KEY_BACKSPACE:
      if (cmd_col > 0)
	{
	  cmd_col--;
	  cmd_buf[cmd_col] = '\0';
	  refreshCommandWindow ();
	  break;
	}
      beep ();
      break;
    case KEY_ENTER:
    case '\n':
      executeCommand ();
      break;
    default:
      if (cmd_col >= CMD_BUF_LEN)
	{
	  beep ();
	  break;
	}
      cmd_buf[cmd_col++] = key;
      cmd_buf[cmd_col] = '\0';
      refreshCommandWindow ();
      break;
    }
}

int
Rts2NMonitor::willConnect (Rts2Address * in_addr)
{
  return 1;
}

int
Rts2NMonitor::messageBox (char *msg)
{
  int key = 0;
  WINDOW *wnd = newwin (5, COLS / 2, LINES / 2, COLS / 2 - COLS / 4);
  PANEL *msg_pan = new_panel (wnd);
  wcolor_set (wnd, CLR_WARNING, NULL);
  box (wnd, 0, 0);
  wcolor_set (wnd, CLR_TEXT, NULL);
  mvwprintw (wnd, 2, 2 + (COLS / 2 - strlen (msg)) / 2, msg);
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

sighandler_t old_Winch;

void
sigWinch (int sig)
{
  if (old_Winch)
    old_Winch (sig);
  monitor->resize ();
}

int
main (int argc, char **argv)
{
  int ret;

  monitor = new Rts2NMonitor (argc, argv);

  old_Winch = signal (SIGWINCH, sigWinch);

  ret = monitor->init ();
  if (ret)
    {
      delete monitor;
      exit (0);
    }
  monitor->run ();

  delete (monitor);

  erase ();
  refresh ();

  nocbreak ();
  echo ();
  endwin ();

  return 0;
}
