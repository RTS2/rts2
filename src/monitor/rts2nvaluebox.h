#ifndef __RTS2_NVALUEBOX__
#define __RTS2_NVALUEBOX__

#include "../utils/rts2value.h"

#include "rts2nwindow.h"
#include "rts2nwindowedit.h"
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
  virtual keyRet injectKey (int key) = 0;
  virtual void draw () = 0;
  virtual void sendValue (Rts2Conn * connection) = 0;
  virtual bool setCursor () = 0;
};

/**
 * Holds edit box for boolean value.
 */
class Rts2NValueBoxBool:public Rts2NValueBox, public Rts2NSelWindow
{
public:
  Rts2NValueBoxBool (Rts2NWindow * top, Rts2ValueBool * in_val, int x, int y);
  virtual keyRet injectKey (int key);
  virtual void draw ();
  virtual void sendValue (Rts2Conn * connection);
  virtual bool setCursor ();
};

/**
 * Holds edit box for integer value.
 */
class Rts2NValueBoxInteger:public Rts2NValueBox,
  public Rts2NWindowEditIntegers
{
public:
  Rts2NValueBoxInteger (Rts2NWindow * top, Rts2ValueInteger * in_val, int x,
			int y);

  virtual keyRet injectKey (int key);
  virtual void draw ();
  virtual void sendValue (Rts2Conn * connection);
  virtual bool setCursor ();
};

/**
 * Holds edit box for float value.
 */
class Rts2NValueBoxFloat:public Rts2NValueBox, public Rts2NWindowEditDigits
{
public:
  Rts2NValueBoxFloat (Rts2NWindow * top, Rts2ValueFloat * in_val, int x,
		      int y);

  virtual keyRet injectKey (int key);
  virtual void draw ();
  virtual void sendValue (Rts2Conn * connection);
  virtual bool setCursor ();
};

/**
 * Holds edit box for double value.
 */
class Rts2NValueBoxDouble:public Rts2NValueBox, public Rts2NWindowEditDigits
{
public:
  Rts2NValueBoxDouble (Rts2NWindow * top, Rts2ValueDouble * in_val, int x,
		       int y);

  virtual keyRet injectKey (int key);
  virtual void draw ();
  virtual void sendValue (Rts2Conn * connection);
  virtual bool setCursor ();
};

/**
 * Holds edit box for selection value.
 */
class Rts2NValueBoxSelection:public Rts2NValueBox, public Rts2NSelWindow
{
public:
  Rts2NValueBoxSelection (Rts2NWindow * top, Rts2ValueSelection * in_val,
			  int x, int y);
  virtual keyRet injectKey (int key);
  virtual void draw ();
  virtual void sendValue (Rts2Conn * connection);
  virtual bool setCursor ();
};

#endif /* !__RTS2_NVALUEBOX__ */
