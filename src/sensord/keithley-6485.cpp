#include "sensord.h"

#include <gpib/ib.h>

class Rts2DevSensorKeithley:public Rts2DevSensor
{
private:
  int minor;
  int pad;

  int gpib_dev;

  int writeRead (char *buf, Rts2ValueInteger * val);

  Rts2ValueInteger *azero;
public:
    Rts2DevSensorKeithley (int argc, char **argv);
    virtual ~ Rts2DevSensorKeithley (void);

  virtual int processOption (int in_opt);
  virtual int init ();
  virtual int info ();
};

int
Rts2DevSensorKeithley::writeRead (char *buf, Rts2ValueInteger * val)
{
  char rb[50];
  int ret;
  rb[0] = '0';
  ret = ibwrt (gpib_dev, buf, strlen (buf));
  if (ret & ERR)
    {
      logStream (MESSAGE_ERROR) << "error writing " << buf << " " << ret <<
	sendLog;
      return -1;
    }
#ifdef DEBUG_EXTRA
  logStream (MESSAGE_DEBUG) << "dev " << gpib_dev << " write " << buf <<
    " ret " << ret << sendLog;
#endif
  ret = ibrd (gpib_dev, rb, 50);
  if (ret & ERR)
    {
      logStream (MESSAGE_ERROR) << "error reading " << rb << " " << ret <<
	sendLog;
      return -1;
    }
#ifdef DEBUG_EXTRA
  logStream (MESSAGE_DEBUG) << "dev " << gpib_dev << " read " << rb << " ret "
    << ret << sendLog;
#endif
  if (!strcmp (rb, "ON"))
    val->setValueInteger (1);
  else
    val->setValueInteger (0);
  return 0;
}

Rts2DevSensorKeithley::Rts2DevSensorKeithley (int in_argc, char **in_argv):
Rts2DevSensor (in_argc, in_argv)
{
  gpib_dev = -1;

  minor = 0;
  pad = 0;

  createValue (azero, "AZERO", "SYSTEM:AZERO value");

  addOption ('m', "minor", 1, "board number (default to 0)");
  addOption ('p', "pad", 1,
	     "device number (default to 0, counted from 0, not from 1)");
}

Rts2DevSensorKeithley::~Rts2DevSensorKeithley (void)
{
  ibonl (gpib_dev, 0);
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
  ret = writeRead ("SYSTEM:AZERO?", azero);
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
