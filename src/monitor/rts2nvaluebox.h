#ifndef __RTS2_NVALUEBOX__
#define __RTS2_NVALUEBOX__

#include "../utils/rts2value.h"

#include "rts2nwindow.h"
#include "rts2daemonwindow.h"

/**
 * Holds edit box for value.
 */
class Rts2NValueBox
{
private:
  Rts2NWindow * topWindow;
  Rts2Value *val;
protected:
    Rts2Value * getValue ()
  {
    return val;
  }
public:
    Rts2NValueBox (Rts2NWindow * top, Rts2Value * in_val);
  virtual ~ Rts2NValueBox (void);
  virtual int injectKey (int key) = 0;
  virtual void draw () = 0;
  virtual void sendValue (Rts2Conn * connection) = 0;
};

/**
 * Holds edit box for boolean value.
 */
class Rts2NValueBoxBool:public Rts2NValueBox, public Rts2NSelWindow
{
public:
  Rts2NValueBoxBool (Rts2NWindow * top, Rts2ValueBool * in_val, int x, int y);
  virtual int injectKey (int key);
  virtual void draw ();
  virtual void sendValue (Rts2Conn * connection);
};

/**
 * Holds edit box for boolean value.
 */
class Rts2NValueBoxDouble:public Rts2NValueBox, public Rts2NWindow
{
public:
  Rts2NValueBoxDouble (Rts2NWindow * top, Rts2ValueDouble * in_val, int x,
		       int y);
  virtual int injectKey (int key);
  virtual void draw ();
  virtual void sendValue (Rts2Conn * connection);
};

#endif /* !__RTS2_NVALUEBOX__ */
