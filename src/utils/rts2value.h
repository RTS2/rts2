#ifndef __RTS2_VALUE__
#define __RTS2_VALUE__

#include <string.h>
#include <math.h>
#include <time.h>

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
protected:
  char buf[100];
public:
    Rts2Value (char *in_val_name);
  int isValue (const char *in_val_name)
  {
    return !strcmp (in_val_name, valueName);
  }
  char *getName ()
  {
    return valueName;
  }
  virtual int setValue (Rts2Conn * connection) = 0;
  virtual char *getValue (int width = 0, int precision = 0)
  {
    return "<unknow>";
  }
  virtual double getValueDouble ()
  {
    return nan ("f");
  }
  virtual int getValueInteger ()
  {
    return -1;
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
  virtual char *getValue (int width = 0, int precision = 0);
};

class Rts2ValueInteger:public Rts2Value
{
private:
  int value;
public:
    Rts2ValueInteger (char *in_val_name);
  virtual int setValue (Rts2Conn * connection);
  virtual char *getValue (int width = 0, int precision = 0);
  virtual double getValueDouble ()
  {
    return value;
  }
  virtual int getValueInteger ()
  {
    return value;
  }
};

class Rts2ValueTime:public Rts2Value
{
private:
  time_t value;
public:
  Rts2ValueTime (char *in_val_name);
  virtual int setValue (Rts2Conn * connection);
  virtual char *getValue (int width = 0, int precision = 0);
  virtual double getValueDouble ()
  {
    return value;
  }
  virtual int getValueInteger ()
  {
    return (int) value;
  }
};

class Rts2ValueDouble:public Rts2Value
{
private:
  double value;
public:
    Rts2ValueDouble (char *in_val_name);
  virtual int setValue (Rts2Conn * connection);
  virtual char *getValue (int width = 0, int precision = 0);
  virtual double getValueDouble ()
  {
    return value;
  }
  virtual int getValueInteger ()
  {
    return (int) value;
  }
};


#endif /* !__RTS2_VALUE__ */
