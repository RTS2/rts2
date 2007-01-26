#ifndef __RTS2_VALUE__
#define __RTS2_VALUE__

#include <string.h>
#include <string>
#include <math.h>
#include <time.h>

#define RTS2_VALUE_STRING	0x01
#define RTS2_VALUE_INTEGER	0x02
#define RTS2_VALUE_TIME		0x03
#define RTS2_VALUE_DOUBLE	0x04
#define RTS2_VALUE_FLOAT	0x05

#define RTS2_VALUE_MASK		0xff

#define RTS2_VALUE_FITS		0x100

class Rts2Conn;

/***********************************
 *
 * Holds values send over TCP/IP.
 * 
 **********************************/

class Rts2Value
{
private:
  std::string valueName;
  std::string description;
protected:
  char buf[100];
  int rts2Type;
public:
    Rts2Value (char *in_val_name);
    Rts2Value (char *in_val_name, std::string in_description,
	       bool writeToFits = true);
    virtual ~ Rts2Value (void)
  {
  }
  int isValue (const char *in_val_name)
  {
    return !strcmp (in_val_name, valueName.c_str ());
  }
  std::string getName ()
  {
    return valueName;
  }
  virtual int setValue (Rts2Conn * connection) = 0;
  virtual char *getValue () = 0;
  virtual double getValueDouble ()
  {
    return nan ("f");
  }
  virtual float getValueFloat ()
  {
    return nan ("f");
  }
  virtual int getValueInteger ()
  {
    return -1;
  }

  std::string getDescription ()
  {
    return description.c_str ();
  }

  int getValueType ()
  {
    return rts2Type & RTS2_VALUE_MASK;
  }

  void setWriteToFits ()
  {
    rts2Type |= RTS2_VALUE_FITS;
  }

  bool getWriteToFits ()
  {
    return (rts2Type & RTS2_VALUE_FITS);
  }

  // send value metainformations, including description
  int sendMetaInfo (Rts2Conn * connection);

  // send value over given connection
  virtual int send (Rts2Conn * connection);
  int sendInfo (Rts2Conn * connection)
  {
    return send (connection);
  }
};

class Rts2ValueString:public Rts2Value
{
private:
  char *value;
public:
    Rts2ValueString (char *in_val_name);
    Rts2ValueString (char *in_val_name, std::string in_description,
		     bool writeToFits = true);
    virtual ~ Rts2ValueString (void)
  {
    delete value;
  }
  virtual int setValue (Rts2Conn * connection);
  virtual char *getValue ();
  void setValueString (char *in_value);
};

class Rts2ValueInteger:public Rts2Value
{
private:
  int value;
public:
    Rts2ValueInteger (char *in_val_name);
    Rts2ValueInteger (char *in_val_name, std::string in_description,
		      bool writeToFits = true);
  virtual int setValue (Rts2Conn * connection);
  void setValueInteger (int in_value)
  {
    value = in_value;
  }
  virtual char *getValue ();
  virtual double getValueDouble ()
  {
    return value;
  }
  virtual float getValueFloat ()
  {
    return value;
  }
  virtual int getValueInteger ()
  {
    return value;
  }
  int inc ()
  {
    return value++;
  }
};

class Rts2ValueTime:public Rts2Value
{
private:
  time_t value;
public:
  Rts2ValueTime (char *in_val_name);
  Rts2ValueTime (char *in_val_name, std::string in_description,
		 bool writeToFits = true);
  virtual int setValue (Rts2Conn * connection);
  virtual char *getValue ();
  virtual double getValueDouble ()
  {
    return value;
  }
  virtual float getValueFloat ()
  {
    return value;
  }
  virtual int getValueInteger ()
  {
    return (int) value;
  }
  virtual void setValueInteger (int in_value)
  {
    value = in_value;
  }
  void setValueTime (time_t in_value)
  {
    value = in_value;
  }
};

class Rts2ValueDouble:public Rts2Value
{
private:
  double value;
public:
    Rts2ValueDouble (char *in_val_name);
    Rts2ValueDouble (char *in_val_name, std::string in_description,
		     bool writeToFits = true);
  virtual int setValue (Rts2Conn * connection);
  void setValueDouble (double in_value)
  {
    value = in_value;
  }
  virtual char *getValue ();
  virtual double getValueDouble ()
  {
    return value;
  }
  virtual float getValueFloat ()
  {
    return value;
  }
  virtual int getValueInteger ()
  {
    return (int) value;
  }
};

class Rts2ValueFloat:public Rts2Value
{
private:
  float value;
public:
    Rts2ValueFloat (char *in_val_name);
    Rts2ValueFloat (char *in_val_name, std::string in_description,
		    bool writeToFits = true);
  virtual int setValue (Rts2Conn * connection);
  void setValueDouble (double in_value)
  {
    value = (float) in_value;
  }
  void setValueFloat (float in_value)
  {
    value = in_value;
  }
  virtual char *getValue ();
  virtual double getValueDouble ()
  {
    return value;
  }
  virtual float getValueFloat ()
  {
    return value;
  }
  virtual int getValueInteger ()
  {
    return (int) value;
  }
};

#endif /* !__RTS2_VALUE__ */
