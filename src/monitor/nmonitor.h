#ifndef __RTS2_NMONITOR__
#define __RTS2_NMONITOR__

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
#include "rts2ncomwin.h"

// colors used in monitor
#define CLR_DEFAULT	1
#define CLR_OK		2
#define CLR_TEXT	3
#define CLR_PRIORITY	4
#define CLR_WARNING	5
#define CLR_FAILURE	6
#define CLR_STATUS      7

#define K_ENTER		13
#define K_ESC		27

#define CMD_BUF_LEN    100

#define MENU_OFF	1
#define MENU_STANDBY	2
#define MENU_ON		3
#define MENU_EXIT	4
#define MENU_ABOUT	5

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
  Rts2NComWin *comWindow;
  Rts2NMenu *menu;

  Rts2NMsgBox *msgBox;

  Rts2NStatusWindow *statusWindow;

    std::list < Rts2NWindow * >windowStack;

  int cmd_col;
  char cmd_buf[CMD_BUF_LEN];

  void executeCommand ();

  int repaint ();

  void refreshAddress ();

  messageAction msgAction;

  bool colorsOff;

  bool resizedRequest;

  void messageBox (const char *query, messageAction action);
  void messageBoxEnd ();
  void menuPerform (int code);
  void leaveMenu ();

protected:
    virtual int processOption (int in_opt);

  virtual void addSelectSocks (fd_set * read_set);
  virtual void selectSuccess (fd_set * read_set);

  virtual int addAddress (Rts2Address * in_addr);
public:
    Rts2NMonitor (int argc, char **argv);
    virtual ~ Rts2NMonitor (void);

  virtual int init ();
  virtual int idle ();

  virtual Rts2ConnClient *createClientConnection (char *in_deviceName);
  virtual int deleteConnection (Rts2Conn * conn);

  virtual int willConnect (Rts2Address * in_addr);

  virtual void message (Rts2Message & msg);

  int resize ();

  void processKey (int key);

  void commandReturn (Rts2Command * cmd, int cmd_status);

  void setResizeRequest ()
  {
    resizedRequest = true;
  }
};

class Rts2NMonConn:public Rts2ConnClient
{
private:
  Rts2NMonitor * master;
public:
  Rts2NMonConn (Rts2NMonitor * in_master,
		char *in_name):Rts2ConnClient (in_master, in_name)
  {
    master = in_master;
  }

  virtual int commandReturn (Rts2Command * cmd, int in_status)
  {
    master->commandReturn (cmd, in_status);
    return Rts2ConnClient::commandReturn (cmd, in_status);
  }
};

#endif /* !__RTS2_NMONITOR__ */
