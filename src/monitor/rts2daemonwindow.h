#ifndef __RTS2_DAEMONWINDOW__
#define __RTS2_DAEMONWINDOW__

#include <curses.h>
#include <panel.h>

#include "../utils/rts2block.h"
#include "../utils/rts2conn.h"
#include "../utils/rts2client.h"
#include "../utils/rts2devclient.h"

class Rts2NWindow
{
protected:
  WINDOW * boxwin;
  WINDOW *window;
public:
    Rts2NWindow (WINDOW * master_window, int x, int y, int w, int h,
		 int border = 1);
    virtual ~ Rts2NWindow (void);
  virtual int injectKey (int key) = 0;
  virtual void draw ();
  virtual void refresh ();
};

class Rts2NSelWindow:public Rts2NWindow
{
protected:
  int selrow;
public:
    Rts2NSelWindow (WINDOW * master_window, int x, int y, int w, int h,
		    int border = 1);
    virtual ~ Rts2NSelWindow (void);
  virtual int injectKey (int key);
  virtual void refresh ();
  int getSelRow ()
  {
    return selrow;
  }
};

class Rts2NDevListWindow:public Rts2NSelWindow
{
private:
  Rts2Block * block;
public:
  Rts2NDevListWindow (WINDOW * master_window, Rts2Block * in_block);
  virtual ~ Rts2NDevListWindow (void);
  virtual int injectKey (int key);
  virtual void draw ();
};

class Rts2NDeviceWindow:public Rts2NSelWindow
{
private:
  WINDOW * valueList;
  Rts2Conn *connection;
  void drawValuesList ();
  void drawValuesList (Rts2DevClient * client);
public:
    Rts2NDeviceWindow (WINDOW * master_window, Rts2Conn * in_connection);
    virtual ~ Rts2NDeviceWindow (void);
  virtual int injectKey (int key);
  virtual void draw ();
};

class Rts2NCentraldWindow:public Rts2NWindow
{
private:
  Rts2Client * client;
  void drawDevice (Rts2Conn * conn);
public:
    Rts2NCentraldWindow (WINDOW * master, Rts2Client * in_client);
    virtual ~ Rts2NCentraldWindow (void);
  virtual int injectKey (int key);
  virtual void draw ();
};

#endif /*! __RTS2_DAEMONWINDOW__ */
