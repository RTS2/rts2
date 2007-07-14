#ifndef __RTS2_VALUE__
#define __RTS2_VALUE__

#include <string.h>
#include <string>
#include <math.h>
#include <time.h>
#include <vector>
#include <iostream>

#define RTS2_VALUE_STRING	0x00000001
#define RTS2_VALUE_INTEGER	0x00000002
#define RTS2_VALUE_TIME		0x00000003
#define RTS2_VALUE_DOUBLE	0x00000004
#define RTS2_VALUE_FLOAT	0x00000005
#define RTS2_VALUE_BOOL		0x00000006
#define RTS2_VALUE_SELECTION	0x00000007

#define RTS2_VALUE_DOUBLE_STAT	0x00000014
#define RTS2_VALUE_DOUBLE_MMAX  0x00000024

#define RTS2_VALUE_MASK		0x000000ff

#define RTS2_VALUE_FITS		0x00000100
#define RTS2_VWHEN_MASK		0x0000f000

#define RTS2_VWHEN_BEFORE_EXP	0x00000000
#define RTS2_VWHEN_AFTER_START	0x00001000
#define RTS2_VWHEN_BEFORE_END	0x00002000
#define RTS2_VWHEN_AFTER_END	0x00003000

#define RTS2_TYPE_MASK		0x00ff0000
#define RTS2_DT_RA		0x00010000
#define RTS2_DT_DEC		0x00020000
#define RTS2_DT_DEGREES		0x00030000
#define RTS2_DT_DEG_DIST	0x00040000

#define RTS2_VALUE_INFOTIME	"infotime"

// script value, when we will display it, we might look for scriptPosition and scriptLen, which will show current script position
#define RTS2_DT_SCRIPT		0x00100000

// BOP mask is taken from status.h, and occupied highest byte (0xff000000)

#include <status.h>

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
  char buf[200];
  int32_t rts2Type;

  void setValueFlags (int32_t flags)
  {
    rts2Type |= (RTS2_TYPE_MASK | RTS2_VWHEN_MASK) & flags;
  }

  /**
   * Set value display type - part covered by RTS2_TYPE_MASK
   * 
   * \param displayType - display type, one from RTS2_DT_xxxx
   * constant.
   */
  void setValueDisplayType (int32_t displayType)
  {
    rts2Type |= (RTS2_TYPE_MASK & displayType);
  }

  /**
   * Send values meta infomration. @see sendMetaInfo adds to it actual value.
   */
  virtual int sendTypeMetaInfo (Rts2Conn * connection);
public:
  Rts2Value (std::string in_val_name);
  Rts2Value (std::string in_val_name, std::string in_description,
	     bool writeToFits = true, int32_t flags = 0);
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
  /**
   * Set value from string.
   * return -1 when value cannot be set from string.
   */
  virtual int setValueString (const char *in_value) = 0;
  virtual int doOpValue (char op, Rts2Value * old_value);
  virtual const char *getValue () = 0;
  virtual const char *getDisplayValue ()
  {
    return getValue ();
  }
  int32_t getValueWriteFlags ()
  {
    return rts2Type & RTS2_VWHEN_MASK;
  }
  int32_t getValueDisplayType ()
  {
    return rts2Type & RTS2_TYPE_MASK;
  }
  int32_t getBopMask ()
  {
    return rts2Type & BOP_MASK;
  }
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
    return description;
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
  virtual void setFromValue (Rts2Value * newValue) = 0;
};

class Rts2ValueString:public Rts2Value
{
private:
  char *value;
public:
    Rts2ValueString (std::string in_val_name);
    Rts2ValueString (std::string in_val_name, std::string in_description,
		     bool writeToFits = true, int32_t flags = 0);
    virtual ~ Rts2ValueString (void)
  {
    delete[]value;
  }
  virtual int setValue (Rts2Conn * connection);
  virtual int setValueString (const char *in_value);
  virtual const char *getValue ();
  virtual int send (Rts2Conn * connection);
  virtual void setFromValue (Rts2Value * newValue);
};

class Rts2ValueInteger:public Rts2Value
{
private:
  int value;
public:
    Rts2ValueInteger (std::string in_val_name);
    Rts2ValueInteger (std::string in_val_name, std::string in_description,
		      bool writeToFits = true, int32_t flags = 0);
  virtual int setValue (Rts2Conn * connection);
  virtual int setValueString (const char *in_value);
  virtual int doOpValue (char op, Rts2Value * old_value);
  /**
   * Returns -1 on error
   *
   */
  virtual int setValueInteger (int in_value)
  {
    value = in_value;
    return 0;
  }
  virtual const char *getValue ();
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
  virtual void setFromValue (Rts2Value * newValue);
};

class Rts2ValueDouble:public Rts2Value
{
protected:
  double value;
public:
    Rts2ValueDouble (std::string in_val_name);
    Rts2ValueDouble (std::string in_val_name, std::string in_description,
		     bool writeToFits = true, int32_t flags = 0);
  virtual int setValue (Rts2Conn * connection);
  virtual int setValueString (const char *in_value);
  virtual int doOpValue (char op, Rts2Value * old_value);
  void setValueDouble (double in_value)
  {
    value = in_value;
  }
  virtual const char *getValue ();
  virtual const char *getDisplayValue ();
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
    if (isnan (value))
      return -1;
    return (int) value;
  }
  virtual void setFromValue (Rts2Value * newValue);
};

class Rts2ValueTime:public Rts2ValueDouble
{
public:
  Rts2ValueTime (std::string in_val_name);
  Rts2ValueTime (std::string in_val_name, std::string in_description,
		 bool writeToFits = true, int32_t flags = 0);
  void setValueTime (time_t in_value)
  {
    Rts2ValueDouble::setValueDouble (in_value);
  }
  virtual const char *getDisplayValue ();
};

class Rts2ValueFloat:public Rts2Value
{
private:
  float value;
public:
    Rts2ValueFloat (std::string in_val_name);
    Rts2ValueFloat (std::string in_val_name, std::string in_description,
		    bool writeToFits = true, int32_t flags = 0);
  virtual int setValue (Rts2Conn * connection);
  virtual int setValueString (const char *in_value);
  virtual int doOpValue (char op, Rts2Value * old_value);
  void setValueDouble (double in_value)
  {
    value = (float) in_value;
  }
  void setValueFloat (float in_value)
  {
    value = in_value;
  }
  virtual const char *getValue ();
  virtual const char *getDisplayValue ();
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
    if (isnan (value))
      return -1;
    return (int) value;
  }
  virtual void setFromValue (Rts2Value * newValue);
};

class Rts2ValueBool:public Rts2ValueInteger
{
  // value - 0 means unknow, 1 is false, 2 is true
public:
  Rts2ValueBool (std::string in_val_name);
  Rts2ValueBool (std::string in_val_name, std::string in_description,
		 bool writeToFits = true, int32_t flags = 0);
  virtual int setValueString (const char *in_value);

  void setValueBool (bool in_bool)
  {
    setValueInteger (in_bool ? 2 : 1);
  }

  bool getValueBool ()
  {
    return getValueInteger () == 2 ? true : false;
  }
};

/**
 * This value adds as meta-information allowed content in strings.
 * It can be used for named selection list (think of enums..).
 */
class Rts2ValueSelection:public Rts2ValueInteger
{
private:
  // holds variables bag..
  std::vector < std::string > valNames;

protected:
  virtual int sendTypeMetaInfo (Rts2Conn * connection);

public:
    Rts2ValueSelection (std::string in_val_name);
    Rts2ValueSelection (std::string in_val_name, std::string in_description,
			bool writeToFits = false, int32_t flags = 0);

  virtual int setValue (Rts2Conn * connection);
  virtual int setValueString (const char *in_value);

  int getSelIndex (std::string in_val);

  virtual const char *getDisplayValue ()
  {
    return getSelVal ().c_str ();
  }

  virtual int setValueInteger (int in_value)
  {
    if (in_value < 0 || (size_t) in_value >= valNames.size ())
      return -1;
    return Rts2ValueInteger::setValueInteger (in_value);
  }

  int setSelIndex (char *selVal)
  {
    int i = getSelIndex (std::string (selVal));
    if (i < 0)
      return -1;
    setValueInteger (i);
    return 0;
  }
  void copySel (Rts2ValueSelection * sel);

  void addSelVal (char *sel_name)
  {
    addSelVal (std::string (sel_name));
  }

  void addSelVal (std::string sel_name)
  {
    valNames.push_back (sel_name);
  }

  void addSelVals (const char **vals);

  std::string getSelVal ()
  {
    return getSelVal (getValueInteger ());
  }

  std::string getSelVal (int val)
  {
    if (val < 0 || (unsigned int) val >= valNames.size ())
      return std::string ("UNK");
    return valNames[val];
  }

  std::vector < std::string >::iterator selBegin ()
  {
    return valNames.begin ();
  }

  std::vector < std::string >::iterator selEnd ()
  {
    return valNames.end ();
  }

  int selSize ()
  {
    return valNames.size ();
  }

  void duplicateSelVals (Rts2ValueSelection * otherValue);
};

#endif /* !__RTS2_VALUE__ */
