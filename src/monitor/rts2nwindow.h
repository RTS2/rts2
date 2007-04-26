#ifndef __RTS2_NWINDOW__
#define __RTS2_NWINDOW__

#include <curses.h>

#include "rts2nlayout.h"

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
  int getWriteWidth ();
  int getWriteHeight ();

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
