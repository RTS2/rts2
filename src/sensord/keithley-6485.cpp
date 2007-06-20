#include "sensorgpib.h"

class Rts2DevSensorKeithley:public Rts2DevSensorGpib
{
private:
  int getGPIB (const char *buf, Rts2ValueString * val);

  int getGPIB (const char *buf, Rts2ValueDouble * val);

  int getGPIB (const char *buf, Rts2ValueBool * val);
  int setGPIB (const char *buf, Rts2ValueBool * val);

  Rts2ValueBool *azero;
  Rts2ValueDouble *current;
protected:
    virtual int setValue (Rts2Value * old_value, Rts2Value * new_value);

  virtual int initValues ();
public:
    Rts2DevSensorKeithley (int argc, char **argv);
    virtual ~ Rts2DevSensorKeithley (void);

  virtual int info ();
};

int
Rts2DevSensorKeithley::getGPIB (const char *buf, Rts2ValueString * val)
{
  int ret;
  char rb[200];
  ret = gpibWrite (buf);
  if (ret)
    return ret;
  ret = gpibRead (rb, 200);
  if (ret)
    return ret;
  for (char *rb_top = rb; *rb_top; rb_top++)
    {
      if (*rb_top == '\n')
	{
	  *rb_top = '\0';
	  break;
	}
    }
  val->setValueString (rb);
  return 0;
}

int
Rts2DevSensorKeithley::getGPIB (const char *buf, Rts2ValueDouble * val)
{
  int ret;
  char rb[200];
  double rval;
  ret = gpibWrite (buf);
  if (ret)
    return ret;
  ret = gpibRead (rb, 200);
  if (ret)
    return ret;
  ret = sscanf (rb, "%lf", &rval);
  if (ret != 1)
    return -1;
  val->setValueDouble (rval);
  return 0;
}

int
Rts2DevSensorKeithley::getGPIB (const char *buf, Rts2ValueBool * val)
{
  int ret;
  char rb[10];
  ret = gpibWrite (buf);
  if (ret)
    return ret;
  ret = gpibRead (rb, 10);
  if (ret)
    return ret;
  if (atoi (rb) == 1)
    val->setValueBool (true);
  else
    val->setValueBool (false);
  return 0;
}

int
Rts2DevSensorKeithley::setGPIB (const char *buf, Rts2ValueBool * val)
{
  char wr[strlen (buf) + 5];
  strcpy (wr, buf);
  if (val->getValueBool ())
    strcat (wr, " ON");
  else
    strcat (wr, " OFF");
  return gpibWrite (wr);
}

int
Rts2DevSensorKeithley::setValue (Rts2Value * old_value, Rts2Value * new_value)
{
  int ret;
  if (old_value == azero)
    {
      ret = setGPIB ("SYSTEM:AZERO", (Rts2ValueBool *) new_value);
      if (ret)
	return -2;
      return 0;
    }
  return Rts2DevSensorGpib::setValue (old_value, new_value);
}

Rts2DevSensorKeithley::Rts2DevSensorKeithley (int in_argc, char **in_argv):
Rts2DevSensorGpib (in_argc, in_argv)
{
  setPad (14);

  createValue (azero, "AZERO", "SYSTEM:AZERO value");
  createValue (current, "CURRENT", "Measured current");
}

Rts2DevSensorKeithley::~Rts2DevSensorKeithley (void)
{
}

int
Rts2DevSensorKeithley::initValues ()
{
  int ret;
  Rts2ValueString *model = new Rts2ValueString ("model");
  ret = getGPIB ("*IDN?", model);
  if (ret)
    return -1;
  addConstValue (model);
  return Rts2DevSensor::initValues ();
}

int
Rts2DevSensorKeithley::info ()
{
  int ret;
  ret = getGPIB ("SYSTEM:AZERO?", azero);
  if (ret)
    return ret;
  ret = gpibWrite ("CONF:CURR");
  if (ret)
    return ret;
  ret = getGPIB ("READ?", current);
  if (ret)
    return ret;
  // scale current
  current->setValueDouble (current->getValueDouble () * 10e+12);
  return Rts2DevSensorGpib::info ();
}

int
main (int argc, char **argv)
{
  Rts2DevSensorKeithley device = Rts2DevSensorKeithley (argc, argv);
  return device.run ();
}
