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
  WINDOW * statusWindow;
  CDKSCREEN *cdkscreen;
  CDKALPHALIST *deviceList;
  CDKALPHALIST *valueList;
  CDKMENU *menu;

  CDKBUTTONBOX *msgBox;

  void *activeEntry;
  void *msgOldEntry;
  CDKSWINDOW *msgwindow;
  int cmd_col;
  char cmd_buf[CMD_BUF_LEN];

  void executeCommand ();
  void relocatesWindows ();

  int paintWindows ();
  int repaint ();

  int printType;

  void drawValuesList ();
  void drawValuesList (Rts2DevClient * client);

  void refreshAddress ();

  messageAction msgAction;

  void messageBox (char *query, messageAction action);
  void messageBoxEnd ();

protected:
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
  refreshCDKScreen (cdkscreen);
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
      processKey (input);
    }
}

void
Rts2NMonitor::refreshAddress ()
{
  int i = 0;
  const char *new_list[connectionSize ()];
  for (connections_t::iterator iter = connectionBegin ();
       iter != connectionEnd (); iter++, i++)
    {
      Rts2Conn *conn = *iter;
      new_list[i] = conn->getName ();
    }
  setCDKAlphalistContents (deviceList, (char **) new_list, connectionSize ());
}

void
Rts2NMonitor::messageBox (char *query, messageAction action)
{
  char *buttons[] = { "Yes", "No" };
  if (msgBox)
    return;
  msgAction = action;
  msgBox = newCDKButtonbox (cdkscreen, CENTER, CENTER, 5, LINES / 2, query,
			    1, 2, buttons, 2, A_REVERSE, TRUE, TRUE);
  drawCDKAlphalist (deviceList, TRUE);
  drawCDKAlphalist (valueList, TRUE);
  drawCDKButtonbox (msgBox, TRUE);
  msgOldEntry = activeEntry;
  activeEntry = msgBox;
}

void
Rts2NMonitor::messageBoxEnd ()
{
  if (msgBox->exitType == vNORMAL && msgBox->currentButton == 0)
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
  destroyCDKButtonbox (msgBox);
  msgBox = NULL;
  activeEntry = msgOldEntry;
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
  cdkscreen = NULL;
  deviceList = NULL;
  valueList = NULL;
  menu = NULL;
  msgwindow = NULL;
  msgBox = NULL;
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
  statusWindow = newwin (1, COLS, LINES - 1, 0);
  wbkgdset (statusWindow, A_BOLD | COLOR_PAIR (CLR_STATUS));
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
  drawCDKSwindow (msgwindow, TRUE);
  return 0;
}

int
Rts2NMonitor::init ()
{
  int ret;
  char *menulist[MAX_MENU_ITEMS][MAX_SUB_ITEMS];
  int menuloc[] = { LEFT, RIGHT };
  int submenusize[] = { 2, 2 };
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
    newCDKAlphalist (cdkscreen, 0, 1, LINES - 22, 20, "<C></B/24>Device list",
		     NULL, NULL, 0, '_', A_REVERSE, TRUE, FALSE);

  valueList =
    newCDKAlphalist (cdkscreen, 20, 1, LINES - 22, COLS - 20,
		     "<C></B/24>Value list", NULL, NULL, 0, '_', A_REVERSE,
		     TRUE, FALSE);

  menulist[0][0] = "</B>File<!B>";
  menulist[1][0] = "</B>Edit<!B>";
  menulist[2][0] = "</B>Help<!B>";
  menulist[0][1] = "</B>Save<!B>";
  menulist[1][1] = "</B>Cut<!B> ";
  menulist[2][1] = "</B>On Edit <!B>";
  menulist[0][2] = "</B>Exit<!B>";
  menulist[1][2] = "</B>Copy<!B>";
  menulist[2][2] = "</B>On File <!B>";
  menulist[1][3] = "</B>Paste<!B>";
  menulist[2][3] = "</B>About...<!B>";

  menu =
    newCDKMenu (cdkscreen, menulist, 2, submenusize, menuloc, TOP,
		A_UNDERLINE, A_REVERSE);

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
  drawCDKMenu (menu);
  drawCDKSwindow (msgwindow, TRUE);

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
  int ret;
  ret = Rts2Client::deleteConnection (conn);
  refreshAddress ();
  return ret;
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
  int ret;
  switch (key)
    {
    case KEY_TAB:
      if (activeEntry == deviceList->entryField)
	activeEntry = valueList->entryField;
      else if (activeEntry == valueList->entryField)
	activeEntry = msgwindow;
      else
	activeEntry = deviceList->entryField;
      break;
    case KEY_F (2):
      messageBox ("</3>Are you sure to switch off?", SWITCH_OFF);
      break;
    case KEY_F (3):
      messageBox ("</3>Are you sure to switch to standby?", SWITCH_STANDBY);
      break;
    case KEY_F (4):
      messageBox ("</3>Are you sure to switch to on?", SWITCH_ON);
      break;
    case KEY_F (5):
      queAll ("info");
      break;
    case KEY_F (8):
      refreshCDKScreen (cdkscreen);
      break;
    case KEY_F (9):
      msgOldEntry = activeEntry;
      activeEntry = menu;
      break;
    case KEY_F (10):
      endRunLoop ();
      break;
      // some input-sensitive commands
    default:
      if (activeEntry == msgwindow)
	{
	  injectCDKSwindow (msgwindow, key);
	}
      else if (activeEntry == menu)
	{
	  injectCDKMenu (menu, key);
	  if (menu->exitType == vNORMAL)
	    {
	      activeEntry = msgOldEntry;
	    }
	  else if (menu->exitType == vESCAPE_HIT)
	    {
	      activeEntry = msgOldEntry;
	    }
	}
      else if (activeEntry == msgBox)
	{
	  ret = injectCDKButtonbox (msgBox, key);
	  if (ret != -1)
	    {
	      messageBoxEnd ();
	    }
	}
      else
	{
	  injectCDKEntry ((CDKENTRY *) activeEntry, key);
	}
    }
  // draw device values
  if (activeEntry == deviceList->entryField && deviceList->entryField->info)
    {
      drawValuesList ();
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
