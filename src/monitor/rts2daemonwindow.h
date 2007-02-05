#ifndef __RTS2_DAEMONWINDOW__
#define __RTS2_DAEMONWINDOW__

#include <curses.h>

#include "rts2nlayout.h"

#include "../utils/rts2block.h"
#include "../utils/rts2conn.h"
#include "../utils/rts2client.h"
#include "../utils/rts2devclient.h"

class Rts2NWindow:public Rts2NLayout
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

  int getX ();
  int getY ();
  int getCurX ();
  int getCurY ();
  int getWidth ();
  int getHeight ();

  void getWinString (char *buf, int n)
  {
    mvwinnstr (window, 0, 0, buf, n);
  }

  virtual void clear ()
  {
    werase (window);
  }

  void move (int x, int y);
  virtual void resize (int x, int y, int w, int h);
  void grow (int max_w, int h_dif);

  virtual void refresh ();

  /**
   * Gets called when window get focus.
   */
  virtual void enter ();

  /**
   * Gets called when window lost focus.
   */
  virtual void leave ();

  /**
   * Set screen cursor to current window.
   */
  void setCursor ();

  /**
   * Returns window which is used to write text
   */
  virtual WINDOW *getWriteWindow ()
  {
    return window;
  }

  void printState (Rts2Conn * conn);
};

class Rts2NSelWindow:public Rts2NWindow
{
protected:
  int selrow;
  int maxrow;
  int padoff_x;
  int padoff_y;
  WINDOW *scrolpad;
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

  virtual void erase ()
  {
    werase (scrolpad);
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
