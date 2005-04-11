#ifndef __RTS2_VALUE__
#define __RTS2_VALUE__

#include <string.h>

class Rts2Conn;

/***********************************
 *
 * Holds values send over TCP/IP.
 * 
 **********************************/

class Rts2Value
{
private:
  char *valueName;
public:
    Rts2Value (char *in_val_name);
  int isValue (char *in_val_name)
  {
    return !strcmp (in_val_name, valueName);
  }
  char *getName ()
  {
    return valueName;
  }
  virtual int setValue (Rts2Conn * connection)
  {
  }
  virtual void getValue (char *buf[])
  {
    *buf = "<unknow>";
  }
};

class Rts2ValueString:public Rts2Value
{
private:
  char *value;
public:
    Rts2ValueString (char *in_val_name);
    virtual ~ Rts2ValueString (void)
  {
    delete value;
  }
  virtual int setValue (Rts2Conn * connection);
  virtual void getValue (char *buf[]);
};

class Rts2ValueInteger:public Rts2Value
{
private:
  int value;
public:
    Rts2ValueInteger (char *in_val_name);
  virtual int setValue (Rts2Conn * connection);
  virtual void getValue (char *buf[]);
};

class Rts2ValueDouble:public Rts2Value
{
private:
  double value;
public:
    Rts2ValueDouble (char *in_val_name);
  virtual int setValue (Rts2Conn * connection);
  virtual void getValue (char *buf[]);
};


#endif /* !__RTS2_VALUE__ */
