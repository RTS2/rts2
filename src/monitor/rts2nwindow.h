#ifndef __RTS2_NWINDOW__
#define __RTS2_NWINDOW__

#include <curses.h>

#include "rts2nlayout.h"

typedef enum
{ RKEY_ENTER, RKEY_HANDLED, RKEY_NOT_HANDLED } keyRet;

/**
 * Basic class for NCurser windows.
 */
class Rts2NWindow:public Rts2NLayout
{
private:
  bool _haveBox;
protected:
  WINDOW * window;
  void errorMove (const char *op, int y, int x, int h, int w);
public:
    Rts2NWindow (int x, int y, int w, int h, int border = 1);
    virtual ~ Rts2NWindow (void);
  /**
   * Handles key pressed event. Return values has following meaning:
   * RKEY_ENTER - enter key was pressed and was not handled by the window
   * RKEY_HANDLED - key was handled by the window, futher processing should not happen
   * RKEY_NOT_HANDLED - key was not handled, futher processing/default fall-back call is necessary
   */
  virtual keyRet injectKey (int key);
  virtual void draw ();

  int getX ();
  int getY ();
  int getCurX ();
  int getCurY ();
  int getWidth ();
  int getHeight ();
  int getWriteWidth ();
  int getWriteHeight ();

  void setX (int x)
  {
    move (x, getY ());
  }

  void setY (int y)
  {
    move (getX (), y);
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
  virtual void setCursor ();

  /**
   * Returns window which is used to write text
   */
  virtual WINDOW *getWriteWindow ()
  {
    return window;
  }

  bool haveBox ()
  {
    return _haveBox;
  }
};

#endif /* !__RTS2_NWINDOW__ */
