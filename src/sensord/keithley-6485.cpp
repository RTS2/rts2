#include "sensord.h"

#include <gpib/ib.h>

class Rts2DevSensorKeithley:public Rts2DevSensor
{
private:
  int minor;
  int pad;

  int gpib_dev;

  int writeGPIB (const char *buf);
  int readGPIB (char *buf, int blen);

  int getGPIB (const char *buf, Rts2ValueString * val);

  int getGPIB (const char *buf, Rts2ValueBool * val);
  int setGPIB (const char *buf, Rts2ValueBool * val);

  Rts2ValueBool *azero;
protected:
    virtual int setValue (Rts2Value * old_value, Rts2Value * new_value);
  virtual int processOption (int in_opt);
public:
    Rts2DevSensorKeithley (int argc, char **argv);
    virtual ~ Rts2DevSensorKeithley (void);

  virtual int initValues ();

  virtual int init ();
  virtual int info ();
};

int
Rts2DevSensorKeithley::writeGPIB (const char *buf)
{
  int ret;
  ret = ibwrt (gpib_dev, buf, strlen (buf));
#ifdef DEBUG_EXTRA
  logStream (MESSAGE_DEBUG) << "dev " << gpib_dev << " write " << buf <<
    " ret " << ret << sendLog;
#endif
  if (ret & ERR)
    {
      logStream (MESSAGE_ERROR) << "error writing " << buf << " " << ret <<
	sendLog;
      return -1;
    }
  return 0;
}

int
Rts2DevSensorKeithley::readGPIB (char *buf, int blen)
{
  int ret;
  buf[0] = '\0';
  ret = ibrd (gpib_dev, buf, blen);
  if (ret & ERR)
    {
      logStream (MESSAGE_ERROR) << "error reading " << buf << " " << ret <<
	sendLog;
      return -1;
    }
#ifdef DEBUG_EXTRA
  logStream (MESSAGE_DEBUG) << "dev " << gpib_dev << " read '" << buf <<
    "' ret " << ret << sendLog;
#endif
  return 0;
}

int
Rts2DevSensorKeithley::getGPIB (const char *buf, Rts2ValueString * val)
{
  int ret;
  char rb[200];
  ret = writeGPIB (buf);
  if (ret)
    return ret;
  ret = readGPIB (rb, 200);
  if (ret)
    return ret;
  val->setValueString (rb);
  return 0;
}

int
Rts2DevSensorKeithley::getGPIB (const char *buf, Rts2ValueBool * val)
{
  int ret;
  char rb[10];
  ret = writeGPIB (buf);
  if (ret)
    return ret;
  ret = readGPIB (rb, 10);
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
  return writeGPIB (wr);
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
  return Rts2DevSensor::setValue (old_value, new_value);
}

Rts2DevSensorKeithley::Rts2DevSensorKeithley (int in_argc, char **in_argv):
Rts2DevSensor (in_argc, in_argv)
{
  gpib_dev = -1;

  minor = 0;
  pad = 14;

  createValue (azero, "AZERO", "SYSTEM:AZERO value");

  addOption ('m', "minor", 1, "board number (default to 0)");
  addOption ('p', "pad", 1,
	     "device number (default to 14, counted from 0, not from 1)");
}

Rts2DevSensorKeithley::~Rts2DevSensorKeithley (void)
{
  ibonl (gpib_dev, 0);
}

int
Rts2DevSensorKeithley::initValues ()
{
  int ret;
  Rts2ValueString *model = new Rts2ValueString ("model");
  ret = getGPIB ("IDN?", model);
  if (ret)
    return -1;
  addConstValue (model);
  return Rts2DevSensor::initValues ();
}

int
Rts2DevSensorKeithley::processOption (int in_opt)
{
  switch (in_opt)
    {
    case 'm':
      minor = atoi (optarg);
      break;
    case 'p':
      pad = atoi (optarg);
      break;
    default:
      return Rts2DevSensor::processOption (in_opt);
    }
  return 0;
}

int
Rts2DevSensorKeithley::init ()
{
  int ret;
  ret = Rts2DevSensor::init ();
  if (ret)
    return ret;

  gpib_dev = ibdev (minor, pad, 0, T3s, 1, 0);
  if (gpib_dev < 0)
    {
      logStream (MESSAGE_ERROR) << "cannot init cryocon on " << minor << " "
	<< pad << sendLog;
      return -1;
    }
  return 0;
}

int
Rts2DevSensorKeithley::info ()
{
  int ret;
  ret = getGPIB ("SYSTEM:AZERO?", azero);
  if (ret)
    return ret;
  return Rts2DevSensor::info ();
}

int
main (int argc, char **argv)
{
  Rts2DevSensorKeithley device = Rts2DevSensorKeithley (argc, argv);
  return device.run ();
}
