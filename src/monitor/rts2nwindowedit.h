#ifndef __RTS2_NWINDOWEDIT__
#define __RTS2_NWINDOWEDIT__

#include "rts2nwindow.h"

/**
 * This is window with single edit box to edit values
 */
class Rts2NWindowEdit:public Rts2NWindow
{
private:
  WINDOW * comwin;
  int ex, ey, ew, eh;
protected:
  /**
   * Returns true if key should we wadded to comwin, false if we don't support this key (e.g. numeric edit box don't want to get
   * alnum). Default is to return isalnum (key).
   */
    virtual bool passKey (int key);
public:
  /**
   * ex, ey, ew and eh are position of (single) edit box within this window.
   */
    Rts2NWindowEdit (int x, int y, int w, int h, int in_ex, int in_ey,
		     int in_ew, int in_eh, int border = 1);
    virtual ~ Rts2NWindowEdit (void);

  virtual keyRet injectKey (int key);
  virtual void refresh ();

  virtual bool setCursor ();

  virtual WINDOW *getWriteWindow ()
  {
    return comwin;
  }
};

/**
 * This is window with edit box for integers
 */
class Rts2NWindowEditIntegers:public Rts2NWindowEdit
{
protected:
  virtual bool passKey (int key);
public:
  Rts2NWindowEditIntegers (int x, int y, int w, int h, int in_ex, int in_ey,
			   int in_ew, int in_eh, int border = 1);
};

/**
 * This is window with edit box for digits..
 */
class Rts2NWindowEditDigits:public Rts2NWindowEdit
{
protected:
  virtual bool passKey (int key);
public:
    Rts2NWindowEditDigits (int x, int y, int w, int h, int in_ex, int in_ey,
			   int in_ew, int in_eh, int border = 1);
};

#endif /* !__RTS2_NWINDOWEDIT__ */
