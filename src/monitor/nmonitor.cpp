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

#include "../writers/rts2image.h"
#include "../writers/rts2devcliimg.h"

#define LINE_SIZE	13
#define COL_SIZE	25

#define CLR_OK		1
#define CLR_TEXT	2
#define CLR_PRIORITY	3
#define CLR_WARNING	4
#define CLR_FAILURE	5
#define CLR_STATUS      6

#define CMD_BUF_LEN    100

#define EVENT_PRINT    RTS2_LOCAL_EVENT + 1

class Rts2CNMonConn:public Rts2ConnClient
{
private:
  int statusBegin;
  WINDOW *window;
  int printStatus ();
protected:
    virtual void print ();

public:
    Rts2CNMonConn (Rts2Block * in_master,
		   char *in_name):Rts2ConnClient (in_master, in_name)
  {
    window = NULL;
    statusBegin = 1;
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
  }

  WINDOW *getWindow ()
  {
    return window;
  }

  virtual int commandReturn (Rts2Command * command, int status)
  {
    print ();
    wrefresh (window);
    return 0;
  }

  virtual int setState (char *in_state_name, int in_value)
  {
    int ret;
    ret = Rts2ConnClient::setState (in_state_name, in_value);
    printStatus ();
    wrefresh (window);
    return ret;
  }

  void setStatusBegin (int in_status_begin)
  {
    statusBegin = in_status_begin;
  }
};

class Rts2NMTelescope:public Rts2DevClientTelescopeImage
{
private:
  Rts2CNMonConn * connection;
  void print (WINDOW * wnd);
public:
    Rts2NMTelescope (Rts2CNMonConn *
		     in_connection):Rts2DevClientTelescopeImage
    (in_connection)
  {
    in_connection->setStatusBegin (8);
    connection = in_connection;
  }
  virtual void postEvent (Rts2Event * event)
  {
    WINDOW *window;
    switch (event->getType ())
      {
      case EVENT_PRINT:
	window = connection->getWindow ();
	if (window)
	  print (window);
	break;
      }
    Rts2DevClientTelescopeImage::postEvent (event);
  }
};

void
Rts2NMTelescope::print (WINDOW * wnd)
{
  struct ln_hrz_posn altaz;
  struct ln_hms hms;
  double gst, lst;

  struct ln_equ_posn tel;
  struct ln_lnlat_posn obs;
  double st;

  getEqu (&tel);
  getObs (&obs);

  altaz.az = nan ("f");
  altaz.alt = nan ("f");

  gst = getValueDouble ("siderealtime") + getValueDouble ("longtitude");
  gst = ln_range_degrees (gst);

  st = gst / 15.0;

  ln_get_hrz_from_equ_sidereal_time (&tel, &obs, st, &altaz);

  mvwprintw (wnd, 1, 1, "Typ: %-10s", getValueChar ("type"));
  mvwprintw (wnd, 2, 1, "R+D/f: %07.3f%+06.3f/%c",
	     getValueDouble ("ra"), getValueDouble ("dec"),
	     getValueDouble ("flip") ? 'f' : 'n');
  mvwprintw (wnd, 3, 1, "Az/Al/D: %03.0f %+02.0f %s", altaz.az, altaz.alt,
	     ln_hrz_to_nswe (&altaz));

  mvwprintw (wnd, 4, 1, "x/y: %.0f %.0f", getValueDouble ("axis0_counts"),
	     getValueDouble ("axis1_counts"));
  mvwprintw (wnd, 5, 1, "Lon/Lat: %+03.3f %+03.3f",
	     getValueDouble ("longtitude"), getValueDouble ("latitude"));

  lst = getValueDouble ("siderealtime");
  ln_rad_to_hms (ln_deg_to_rad (lst), &hms);
  mvwprintw (wnd, 6, 1, "Lsid: %07.3f (%02i:%02i:%02.1f)",
	     getValueDouble ("siderealtime"), hms.hours, hms.minutes,
	     hms.seconds);

  ln_rad_to_hms (ln_deg_to_rad (gst), &hms);
  mvwprintw (wnd, 7, 1, "Gsid: %07.3f (%02i:%02i:%02.1f)", gst, hms.hours,
	     hms.minutes, hms.seconds);
}

class Rts2NMCamera:public Rts2DevClientCamera
{
private:
  Rts2CNMonConn * connection;
  void print (WINDOW * wnd);
public:
    Rts2NMCamera (Rts2CNMonConn *
		  in_connection):Rts2DevClientCamera (in_connection)
  {
    in_connection->setStatusBegin (8);
    connection = in_connection;
  }
  virtual void postEvent (Rts2Event * event)
  {
    switch (event->getType ())
      {
	WINDOW *window;
      case EVENT_PRINT:
	window = connection->getWindow ();
	if (window)
	  print (window);
	break;
      }
    Rts2DevClientCamera::postEvent (event);
  }
};

void
Rts2NMCamera::print (WINDOW * wnd)
{
  mvwprintw (wnd, 1, 1, "Typ: %-10s", getValueChar ("type"));
  mvwprintw (wnd, 2, 1, "Ser: %-10s", getValueChar ("serial"));
/*  if (info->chip_info)
    mvwprintw (wnd, 3, 1, "Siz: [%ix%i]", info->chip_info[0].width,
	       info->chip_info[0].height);
  else
    mvwprintw (wnd, 3, 1, "Siz: Unknow"); */
  mvwprintw (wnd, 4, 1, "S/A: %+05.1f %+05.1f oC",
	     getValueDouble ("temperature_setpoint"),
	     getValueDouble ("air_temperature"));
  mvwprintw (wnd, 5, 1, "CCD: %+05.1f oC",
	     getValueDouble ("ccd_temperature"));
  mvwprintw (wnd, 6, 1, "CPo: %04.1f %%",
	     getValueDouble ("cooling_power") / 10.0);
  mvwprintw (wnd, 7, 1, "Fan: %s", getValueDouble ("fan") ? "on " : "off");
}

class Rts2NMDome:public Rts2DevClientDome
{
private:
  Rts2CNMonConn * connection;
  void print (WINDOW * wnd);
public:
    Rts2NMDome (Rts2CNMonConn *
		in_connection):Rts2DevClientDome (in_connection)
  {
    in_connection->setStatusBegin (8);
    connection = in_connection;
  }
  virtual void postEvent (Rts2Event * event)
  {
    switch (event->getType ())
      {
	WINDOW *window;
      case EVENT_PRINT:
	window = connection->getWindow ();
	if (window)
	  print (window);
	break;
      }
    Rts2DevClientDome::postEvent (event);
  }
};

void
Rts2NMDome::print (WINDOW * wnd)
{
  int dome = getValueInteger ("dome");
  mvwprintw (wnd, 1, 1, "Mod: %s", getValueChar ("type"));
  mvwprintw (wnd, 2, 1, "Tem: %+2.2f oC", getValueChar ("temperature"));
  mvwprintw (wnd, 3, 1, "Hum: %2.2f %", getValueChar ("humidity"));
  mvwprintw (wnd, 4, 1, "Pow_tel: %i", getValueInteger ("power_telescope"));
  mvwprintw (wnd, 5, 1, "Pow_cam: %i", getValueInteger ("power_cameras"));
#define is_on(num)	((dome & (1 << num))? 'O' : 'f')
  mvwprintw (wnd, 6, 1, "Open sw: %c %c", is_on (0), is_on (1));
  mvwprintw (wnd, 7, 1, "Close s: %c %c", is_on (2), is_on (3));
#undef is_on
}

class Rts2NMExecutor:public Rts2DevClientExecutor
{
private:
  Rts2CNMonConn * connection;
  void print (WINDOW * wnd);
public:
    Rts2NMExecutor (Rts2CNMonConn *
		    in_connection):Rts2DevClientExecutor (in_connection)
  {
    in_connection->setStatusBegin (4);
    connection = in_connection;
  }
  virtual void postEvent (Rts2Event * event)
  {
    switch (event->getType ())
      {
	WINDOW *window;
      case EVENT_PRINT:
	window = connection->getWindow ();
	if (window)
	  print (window);
	break;
      }
    Rts2DevClientExecutor::postEvent (event);
  }
};

void
Rts2NMExecutor::print (WINDOW * wnd)
{
  mvwprintw (wnd, 1, 1, "Curr: %-5i", getValueInteger ("current"));
  mvwprintw (wnd, 2, 1, "Next: %-5i", getValueInteger ("next"));
  mvwprintw (wnd, 3, 1, "ObsI: %5i", getValueInteger ("obsid"));
}

class Rts2NMImgproc:public Rts2DevClientImgproc
{
private:
  Rts2CNMonConn * connection;
  void print (WINDOW * wnd);
public:
    Rts2NMImgproc (Rts2CNMonConn *
		   in_connection):Rts2DevClientImgproc (in_connection)
  {
    in_connection->setStatusBegin (2);
    connection = in_connection;
  }
  virtual void postEvent (Rts2Event * event)
  {
    switch (event->getType ())
      {
	WINDOW *window;
      case EVENT_PRINT:
	window = connection->getWindow ();
	if (window)
	  print (window);
	break;
      }
    Rts2DevClientImgproc::postEvent (event);
  }
};

void
Rts2NMImgproc::print (WINDOW * wnd)
{
  mvwprintw (wnd, 1, 1, "Que : %-5i", getValueInteger ("que_size"));
}

class Rts2NMGrb:public Rts2DevClientGrb
{
private:
  Rts2CNMonConn * connection;
  void print (WINDOW * wnd);
public:
    Rts2NMGrb (Rts2CNMonConn * in_connection):Rts2DevClientGrb (in_connection)
  {
    in_connection->setStatusBegin (5);
    connection = in_connection;
  }
  virtual void postEvent (Rts2Event * event)
  {
    switch (event->getType ())
      {
	WINDOW *window;
      case EVENT_PRINT:
	window = connection->getWindow ();
	if (window)
	  print (window);
	break;
      }
    Rts2DevClientGrb::postEvent (event);
  }
};

void
Rts2NMGrb::print (WINDOW * wnd)
{
  mvwprintw (wnd, 1, 1, "L Pac: %-5i", getValueInteger ("last_packet"));
  mvwprintw (wnd, 2, 1, "Delta: %-5i", getValueDouble ("delta"));
  mvwprintw (wnd, 3, 1, "L Tar: %5", getValueChar ("last_target"));
  mvwprintw (wnd, 3, 1, "LTime: %5f", getValueDouble ("last_target_time"));
}

// here begins nmonitor common part

int
Rts2CNMonConn::printStatus ()
{
  int nrow;
  nrow = statusBegin;
  for (int i = 0; i < MAX_STATE; i++)
    {
      Rts2ServerState *state = serverState[i];
      if (state)
	{
	  mvwprintw (window, nrow, 0, "%10s: %i", state->name, state->value);
	  nrow++;
	}
    }
  return 0;
}

void
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

  if (!otherDevice)
    {
      mvwprintw (window, 1, 0, "  UNKNOW DEV");
    }
  else
    {
      // print values
      otherDevice->postEvent (new Rts2Event (EVENT_PRINT));
    }
  ret = printStatus ();
  wrefresh (window);
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

protected:
    virtual Rts2ConnClient * createClientConnection (char *in_deviceName)
  {
    Rts2CNMonConn *conn;
    conn = new Rts2CNMonConn (this, in_deviceName);
    processConnection (conn);
    return conn;
  }

public:
  Rts2NMonitor (int argc, char **argv);
  virtual ~ Rts2NMonitor (void);

  virtual int init ();
  virtual int idle ();

  virtual Rts2DevClient *createOtherType (Rts2Conn * conn,
					  int other_device_type);

  virtual int addAddress (Rts2Address * in_addr);

  int resize ()
  {
    return repaint ();
  }

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
	    newwin (LINES / 2, COLS / 4, 1 + LINES / 4 * ((win_num - 1) / 4),
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
  connCols = -1;
  connLines = 0;
  cmd_col = 0;
}

Rts2NMonitor::~Rts2NMonitor (void)
{
  if (connCols >= 0)
    {
      delwin (statusWindow);
      delwin (commandWindow);

      erase ();
      refresh ();

      nocbreak ();
      echo ();
      endwin ();
    }
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

Rts2DevClient *
Rts2NMonitor::createOtherType (Rts2Conn * conn, int other_device_type)
{
  switch (other_device_type)
    {
    case DEVICE_TYPE_MOUNT:
      return new Rts2NMTelescope ((Rts2CNMonConn *) conn);
    case DEVICE_TYPE_CCD:
      return new Rts2NMCamera ((Rts2CNMonConn *) conn);
    case DEVICE_TYPE_DOME:
      return new Rts2NMDome ((Rts2CNMonConn *) conn);
    case DEVICE_TYPE_EXECUTOR:
      return new Rts2NMExecutor ((Rts2CNMonConn *) conn);
    case DEVICE_TYPE_IMGPROC:
      return new Rts2NMImgproc ((Rts2CNMonConn *) conn);
    case DEVICE_TYPE_GRB:
      return new Rts2NMGrb ((Rts2CNMonConn *) conn);
    default:
      return Rts2Client::createOtherType (conn, other_device_type);
    }
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

sighandler_t old_Winch;

void
sigWinch (int sig)
{
//  monitor->resize ();
  if (old_Winch)
    old_Winch (sig);
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
  old_Winch = signal (SIGWINCH, sigWinch);

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
