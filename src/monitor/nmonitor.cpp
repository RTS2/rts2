#include <libnova/libnova.h>
#include <getopt.h>
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

#include "rts2daemonwindow.h"
#include "rts2nmenu.h"
#include "rts2nmsgbox.h"
#include "rts2nmsgwindow.h"
#include "rts2nstatuswindow.h"

#include "nmonitor.h"

#ifdef HAVE_XCURSES
char *XCursesProgramName = "rts2-mon";
#endif

#define CMD_BUF_LEN    100

#define MENU_OFF	1
#define MENU_STANDBY	2
#define MENU_ON		3

enum messageAction
{ SWITCH_OFF, SWITCH_STANDBY, SWITCH_ON };

/*******************************************************
 *
 * This class hold "root" window of display,
 * takes care about displaying it's connection etc..
 *
 ******************************************************/

class Rts2NMonitor:public Rts2Client
{
private:
  WINDOW * cursesWin;
  Rts2NDevListWindow *deviceList;
  Rts2NWindow *daemonWindow;
  Rts2NMsgWindow *msgwindow;
  Rts2NMenu *menu;

  Rts2NMsgBox *msgBox;

  Rts2NStatusWindow *statusWindow;

  Rts2NWindow *activeWindow;
  Rts2NWindow *msgOldEntry;
  int cmd_col;
  char cmd_buf[CMD_BUF_LEN];

  void executeCommand ();
  void relocatesWindows ();

  int paintWindows ();
  int repaint ();

  void refreshAddress ();

  messageAction msgAction;

  void messageBox (const char *query, messageAction action);
  void messageBoxEnd ();
  void menuPerform (int code);
  void leaveMenu ();

protected:
    virtual void addSelectSocks (fd_set * read_set);
  virtual void selectSuccess (fd_set * read_set);

  virtual int addAddress (Rts2Address * in_addr);
public:
    Rts2NMonitor (int argc, char **argv);
    virtual ~ Rts2NMonitor (void);

  virtual int init ();
  virtual int idle ();

  virtual int deleteConnection (Rts2Conn * conn);

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
}

void
Rts2NMonitor::relocatesWindows ()
{
  resize ();
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
      processKey (input);
    }
}

void
Rts2NMonitor::refreshAddress ()
{
  deviceList->draw ();
}

void
Rts2NMonitor::messageBox (const char *query, messageAction action)
{
  const static char *buts[] = { "Yes", "No" };
  if (msgBox)
    return;
  msgAction = action;
  msgBox = new Rts2NMsgBox (cursesWin, query, buts, 2);
  msgBox->draw ();
  msgOldEntry = activeWindow;
  activeWindow = msgBox;
}

void
Rts2NMonitor::messageBoxEnd ()
{
  if (msgBox->exitState == 0)
    {
      switch (msgAction)
	{
	case SWITCH_OFF:
	  getCentraldConn ()->queCommand (new Rts2Command (this, "off"));
	  break;
	case SWITCH_STANDBY:
	  getCentraldConn ()->queCommand (new Rts2Command (this, "standby"));
	  break;
	case SWITCH_ON:
	  getCentraldConn ()->queCommand (new Rts2Command (this, "on"));
	  break;
	}
    }
  delete msgBox;
  msgBox = NULL;
  activeWindow = msgOldEntry;
}

void
Rts2NMonitor::menuPerform (int code)
{
  switch (code)
    {
    case MENU_OFF:
      break;
    case MENU_STANDBY:
      break;
    case MENU_ON:
      break;
    }
}

void
Rts2NMonitor::leaveMenu ()
{
  menu->leave ();
  activeWindow = msgOldEntry;
  msgOldEntry = NULL;
}

int
Rts2NMonitor::addAddress (Rts2Address * in_addr)
{
  int ret;
  ret = Rts2Client::addAddress (in_addr);
  if (ret)
    return ret;
  refreshAddress ();
  return ret;
}

Rts2NMonitor::Rts2NMonitor (int in_argc, char **in_argv):
Rts2Client (in_argc, in_argv)
{
  statusWindow = NULL;
  deviceList = NULL;
  daemonWindow = NULL;
  menu = NULL;
  msgwindow = NULL;
  msgBox = NULL;
  cmd_col = 0;
}

Rts2NMonitor::~Rts2NMonitor (void)
{
  endwin ();
  delete deviceList;
  delete daemonWindow;
  delete msgBox;
  delete msgwindow;
  delete statusWindow;
}

int
Rts2NMonitor::paintWindows ()
{
  curs_set (0);
  doupdate ();
  curs_set (1);
  return 0;
}

int
Rts2NMonitor::repaint ()
{

  deviceList->draw ();
  daemonWindow->draw ();
  msgwindow->draw ();
  statusWindow->draw ();
  menu->draw ();
  if (msgBox)
    msgBox->draw ();
  doupdate ();
  return 0;
}

int
Rts2NMonitor::init ()
{
  int ret;
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

  cbreak ();
  noecho ();
  nonl ();
  intrflush (stdscr, FALSE);
  keypad (stdscr, TRUE);

  deviceList = new Rts2NDevListWindow (cursesWin, this);

  menu = new Rts2NMenu (cursesWin);
  Rts2NSubmenu *sub = new Rts2NSubmenu (cursesWin, "System");
  sub->createAction ("Off", MENU_OFF);
  sub->createAction ("Standby", MENU_STANDBY);
  sub->createAction ("On", MENU_ON);
  sub->createAction ("Exit", 4);
  menu->addSubmenu (sub);

  sub = new Rts2NSubmenu (cursesWin, "Help");
  sub->createAction ("About", 5);
  menu->addSubmenu (sub);


  msgwindow = new Rts2NMsgWindow (cursesWin);

  activeWindow = deviceList;

  statusWindow = new Rts2NStatusWindow (cursesWin, this);

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

  daemonWindow = new Rts2NCentraldWindow (cursesWin, this);

  return repaint ();
}

int
Rts2NMonitor::idle ()
{
  repaint ();
  setTimeout (USEC_SEC);
  return Rts2Client::idle ();
}

int
Rts2NMonitor::deleteConnection (Rts2Conn * conn)
{
  int ret;
  ret = Rts2Client::deleteConnection (conn);
  refreshAddress ();
  return ret;
}

void
Rts2NMonitor::message (Rts2Message & msg)
{
  *msgwindow << msg;
}

int
Rts2NMonitor::resize ()
{
  erase ();
  endwin ();
  initscr ();
  relocatesWindows ();
  paintWindows ();
  return repaint ();
}

void
Rts2NMonitor::processKey (int key)
{
  int ret = -1;
  switch (key)
    {
    case '\t':
    case KEY_STAB:
      activeWindow->leave ();
      if (activeWindow == deviceList)
	activeWindow = daemonWindow;
      else if (activeWindow == daemonWindow)
	activeWindow = msgwindow;
      else
	activeWindow = deviceList;
      activeWindow->enter ();
      break;
    case KEY_F (2):
      messageBox ("Are you sure to switch off?", SWITCH_OFF);
      break;
    case KEY_F (3):
      messageBox ("Are you sure to switch to standby?", SWITCH_STANDBY);
      break;
    case KEY_F (4):
      messageBox ("Are you sure to switch to on?", SWITCH_ON);
      break;
    case KEY_F (5):
      queAll ("info");
      break;
    case KEY_F (8):
      doupdate ();
      break;
    case KEY_F (9):
      msgOldEntry = activeWindow;
      activeWindow = menu;
      activeWindow->enter ();
      deviceList->draw ();
      if (daemonWindow)
	daemonWindow->draw ();
      menu->draw ();
      break;
    case KEY_F (10):
      endRunLoop ();
      break;
      // some input-sensitive commands
    default:
      ret = activeWindow->injectKey (key);
    }
  // draw device values
  if (activeWindow == deviceList)
    {
      Rts2Conn *conn = connectionAt (deviceList->getSelRow ());
      if (conn)
	{
	  if (conn->getName () == std::string (""))
	    {
	      delete daemonWindow;
	      daemonWindow = new Rts2NCentraldWindow (cursesWin, this);
	    }
	  else
	    {
	      delete daemonWindow;
	      daemonWindow = new Rts2NDeviceWindow (cursesWin, conn);
	    }
	}
    }
  // handle msg box
  if (activeWindow == msgBox && ret == 0)
    {
      messageBoxEnd ();
    }
  else if (activeWindow == menu && ret == 0)
    {
      Rts2NAction *action;
      action = menu->getSelAction ();
      if (action)
	menuPerform (action->getCode ());
      else
	leaveMenu ();
    }
}

int
Rts2NMonitor::willConnect (Rts2Address * in_addr)
{
  return 1;
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
