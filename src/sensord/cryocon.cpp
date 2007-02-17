#include "sensord.h"

#include <gpib/ib.h>

class Rts2DevSensorCryocon:public Rts2DevSensor
{
private:
  int minor;
  int pad;

  int gpib_dev;

  int writeRead (char *buf, Rts2ValueDouble * val);

  Rts2ValueDouble *tempA;
public:
    Rts2DevSensorCryocon (int argc, char **argv);
    virtual ~ Rts2DevSensorCryocon (void);

  virtual int processOption (int in_opt);
  virtual int init ();
  virtual int info ();
};

int
Rts2DevSensorCryocon::writeRead (char *buf, Rts2ValueDouble * val)
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
  val->setValueDouble (atof (rb));
  return 0;
}

Rts2DevSensorCryocon::Rts2DevSensorCryocon (int in_argc, char **in_argv):
Rts2DevSensor (in_argc, in_argv)
{
  gpib_dev = -1;

  minor = 0;
  pad = 0;

  tempA = new Rts2ValueDouble ("TEMPA", "cryocon temperature from channel A");
  addValue (tempA);

  addOption ('m', "minor", 1, "board number (default to 0)");
  addOption ('p', "pad", 1,
	     "device number (default to 0, counted from 0, not from 1)");
}

Rts2DevSensorCryocon::~Rts2DevSensorCryocon (void)
{
  ibonl (gpib_dev, 0);
}

int
Rts2DevSensorCryocon::processOption (int in_opt)
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
Rts2DevSensorCryocon::init ()
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
Rts2DevSensorCryocon::info ()
{
  int ret;
  ret = writeRead ("INPUT A:TEMP?", tempA);
  if (ret)
    return ret;
  return Rts2DevSensor::info ();
}

int
main (int argc, char **argv)
{
  Rts2DevSensorCryocon device = Rts2DevSensorCryocon (argc, argv);
  return device.run ();
}
