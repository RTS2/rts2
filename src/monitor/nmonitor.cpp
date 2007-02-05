#include <libnova/libnova.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <list>

#include <iostream>
#include <fstream>

#include "nmonitor.h"

#ifdef HAVE_XCURSES
char *XCursesProgramName = "rts2-mon";
#endif

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

int
Rts2NMonitor::processOption (int in_opt)
{
  switch (in_opt)
    {
    case 'c':
      colorsOff = true;
      break;
    default:
      return Rts2Client::processOption (in_opt);
    }
  return 0;
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
  while (1)
    {
      int input = getch ();
      if (input == ERR)
	break;
      processKey (input);
    }
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
  windowStack.push_back (msgBox);
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
  windowStack.pop_back ();
}

void
Rts2NMonitor::menuPerform (int code)
{
  switch (code)
    {
    case MENU_OFF:
      messageBox ("Are you sure to switch off?", SWITCH_OFF);
      break;
    case MENU_STANDBY:
      messageBox ("Are you sure to switch to standby?", SWITCH_STANDBY);
      break;
    case MENU_ON:
      messageBox ("Are you sure to switch to on?", SWITCH_ON);
      break;
    case MENU_ABOUT:

      break;
    case MENU_EXIT:
      endRunLoop ();
      break;
    }
}

void
Rts2NMonitor::leaveMenu ()
{
  menu->leave ();
  windowStack.pop_back ();
}

void
Rts2NMonitor::changeActive (Rts2NWindow * new_active)
{
  Rts2NWindow *activeWindow = *(--windowStack.end ());
  windowStack.pop_back ();
  activeWindow->leave ();
  windowStack.push_back (new_active);
  new_active->enter ();
}

int
Rts2NMonitor::addAddress (Rts2Address * in_addr)
{
  int ret;
  ret = Rts2Client::addAddress (in_addr);
  if (ret)
    return ret;
  return ret;
}

Rts2NMonitor::Rts2NMonitor (int in_argc, char **in_argv):
Rts2Client (in_argc, in_argv)
{
  masterLayout = NULL;
  daemonLayout = NULL;
  statusWindow = NULL;
  deviceList = NULL;
  daemonWindow = NULL;
  comWindow = NULL;
  menu = NULL;
  msgwindow = NULL;
  msgBox = NULL;
  cmd_col = 0;

  colorsOff = false;

  resizedRequest = false;

  addOption ('c', "color-off", 0, "don't use colors");
}

Rts2NMonitor::~Rts2NMonitor (void)
{
  endwin ();
  delete msgBox;
  delete statusWindow;

  delete masterLayout;
}

int
Rts2NMonitor::repaint ()
{
  curs_set (0);
  deviceList->draw ();
  daemonWindow->draw ();
  msgwindow->draw ();
  statusWindow->draw ();
  comWindow->draw ();
  menu->draw ();
  if (msgBox)
    msgBox->draw ();
  comWindow->setCursor ();
  curs_set (1);
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

  // init ncurses
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
  timeout (0);

  // create & init menu
  menu = new Rts2NMenu (cursesWin);
  Rts2NSubmenu *sub = new Rts2NSubmenu (cursesWin, "System");
  sub->createAction ("Off", MENU_OFF);
  sub->createAction ("Standby", MENU_STANDBY);
  sub->createAction ("On", MENU_ON);
  sub->createAction ("Exit", MENU_EXIT);
  menu->addSubmenu (sub);

  sub = new Rts2NSubmenu (cursesWin, "Help");
  sub->createAction ("About", MENU_ABOUT);
  menu->addSubmenu (sub);

  // start color mode
  start_color ();
  use_default_colors ();

  if (has_colors () && !colorsOff)
    {
      init_pair (CLR_DEFAULT, -1, -1);
      init_pair (CLR_OK, COLOR_GREEN, -1);
      init_pair (CLR_TEXT, COLOR_WHITE, -1);
      init_pair (CLR_PRIORITY, COLOR_CYAN, -1);
      init_pair (CLR_WARNING, COLOR_RED, -1);
      init_pair (CLR_FAILURE, COLOR_YELLOW, -1);
      init_pair (CLR_STATUS, COLOR_RED, COLOR_CYAN);
    }

  // init windows
  deviceList = new Rts2NDevListWindow (cursesWin, this);
  comWindow = new Rts2NComWin (cursesWin);
  msgwindow = new Rts2NMsgWindow (cursesWin);
  windowStack.push_back (deviceList);
  statusWindow = new Rts2NStatusWindow (cursesWin, this);
  daemonWindow = new Rts2NCentraldWindow (cursesWin, this);

  // init layout
  daemonLayout = new Rts2NLayoutBlock (daemonWindow, comWindow, false, 66);
  masterLayout = new Rts2NLayoutBlock (deviceList, daemonLayout, true, 10);
  masterLayout = new Rts2NLayoutBlock (masterLayout, msgwindow, false, 75);

  getCentraldConn ()->queCommand (new Rts2Command (this, "info"));
  setMessageMask (MESSAGE_MASK_ALL);

  resize ();

  return repaint ();
}

int
Rts2NMonitor::idle ()
{
  repaint ();
  setTimeout (USEC_SEC);
  return Rts2Client::idle ();
}

Rts2ConnClient *
Rts2NMonitor::createClientConnection (char *in_deviceName)
{
  return new Rts2NMonConn (this, in_deviceName);
}

void
Rts2NMonitor::message (Rts2Message & msg)
{
  *msgwindow << msg;
}

void
Rts2NMonitor::resize ()
{
  menu->resize (0, 0, COLS, 1);
  statusWindow->resize (0, LINES - 1, COLS, 1);
  masterLayout->resize (0, 1, COLS, LINES - 2);
}

void
Rts2NMonitor::processKey (int key)
{
  Rts2NWindow *activeWindow = *(--windowStack.end ());
  int ret = -1;
  switch (key)
    {
    case '\t':
    case KEY_STAB:
      if (activeWindow == deviceList)
	{
	  changeActive (daemonWindow);
	}
      else if (activeWindow == daemonWindow)
	{
	  changeActive (msgwindow);
	}
      else if (activeWindow == msgwindow)
	{
	  changeActive (deviceList);
	}
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
      windowStack.push_back (menu);
      menu->enter ();
      break;
    case KEY_F (10):
      endRunLoop ();
      break;
    case KEY_RESIZE:
      resize ();
      break;
      // default for active window
    case KEY_UP:
    case KEY_DOWN:
    case KEY_LEFT:
    case KEY_RIGHT:
    case KEY_HOME:
    case KEY_END:
    case KEY_ENTER:
    case K_ENTER:
    case KEY_EXIT:
    case K_ESC:
      ret = activeWindow->injectKey (key);
      break;
    default:
      ret = comWindow->injectKey (key);
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
	      daemonLayout->setLayoutA (daemonWindow);
	    }
	  else
	    {
	      delete daemonWindow;
	      daemonWindow = new Rts2NDeviceWindow (cursesWin, conn);
	      daemonLayout->setLayoutA (daemonWindow);
	    }
	  resize ();
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
  else if (ret == 0 || key == KEY_ENTER || key == K_ENTER)
    {
      char command[comWindow->getCurX () + 1];
      Rts2Conn *conn = connectionAt (deviceList->getSelRow ());
      comWindow->getWinString (command, (comWindow->getCurX () - 1));
      command[comWindow->getCurX () + 1] = '\0';
      conn->queCommand (new Rts2Command (this, command));
      comWindow->clear ();
      mvwprintw (comWindow->getWriteWindow (), 1, 0, "%s", command);
      wmove (comWindow->getWriteWindow (), 0, 0);
    }
}

void
Rts2NMonitor::commandReturn (Rts2Command * cmd, int cmd_status)
{
  comWindow->commandReturn (cmd, cmd_status);
}

int
Rts2NMonitor::willConnect (Rts2Address * in_addr)
{
  return 1;
}

Rts2NMonitor *monitor = NULL;

int
main (int argc, char **argv)
{
  int ret;

  monitor = new Rts2NMonitor (argc, argv);

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
