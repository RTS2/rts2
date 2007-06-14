#include "sensord.h"

#include <gpib/ib.h>

class Rts2DevSensorCryocon;

/**
 * Structure which holds information about input channel
 */
class Rts2ValueTempInput
{
private:
  char chan;
public:
    Rts2ValueDouble * values[6];

    Rts2ValueTempInput (Rts2DevSensorCryocon * dev, char in_chan);

  char getChannel ()
  {
    return chan;
  }
};

class Rts2DevSensorCryocon:public Rts2DevSensor
{
private:
  int minor;
  int pad;

  int gpib_dev;

  int writeRead (char *buf, Rts2ValueDouble * val);
  int writeRead (char *buf, Rts2ValueFloat * val);

  Rts2ValueTempInput *chans[4];
  Rts2ValueDouble *statTime;

  Rts2ValueDouble *amb;
  Rts2ValueFloat *htrread;
  Rts2ValueFloat *htrhst;
public:
    Rts2DevSensorCryocon (int argc, char **argv);
    virtual ~ Rts2DevSensorCryocon (void);

  void createTempInputValue (Rts2ValueDouble ** val, char chan,
			     const char *name, const char *desc);

  virtual int processOption (int in_opt);
  virtual int init ();
  virtual int info ();
};

Rts2ValueTempInput::Rts2ValueTempInput (Rts2DevSensorCryocon * dev,
					char in_chan)
{
  chan = in_chan;
  // values are passed to dev, and device deletes them!
  dev->createTempInputValue (&values[0], chan, "TEMP",
			     "cryocon temperature from channel ");
  dev->createTempInputValue (&values[1], chan, "MINIMUM",
			     "minimum temperature on channel ");
  dev->createTempInputValue (&values[2], chan, "MAXIMUM",
			     "maximum temperature on channel ");
  dev->createTempInputValue (&values[3], chan, "VARIANCE",
			     "temperature variance of channel ");
  dev->createTempInputValue (&values[4], chan, "SLOPE", "slope of channel ");
  dev->createTempInputValue (&values[5], chan, "OFFSET",
			     "temp offset of channel ");
}

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

int
Rts2DevSensorCryocon::writeRead (char *buf, Rts2ValueFloat * val)
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
  val->setValueFloat (atof (rb));
  return 0;
}

Rts2DevSensorCryocon::Rts2DevSensorCryocon (int in_argc, char **in_argv):
Rts2DevSensor (in_argc, in_argv)
{
  gpib_dev = -1;

  minor = 0;
  pad = 12;

  for (int i = 0; i < 4; i++)
    {
      chans[i] = new Rts2ValueTempInput (this, 'A' + i);
    }

  createValue (statTime, "STATTIME", "time for which statistic was collected",
	       true);

  createValue (amb, "AMBIENT", "cryocon ambient temperature", true);
  createValue (htrread, "HTRREAD", "Heater read back current", true);
  createValue (htrhst, "HTRHST", "Heater heat sink temperature", true);

  addOption ('m', "minor", 1, "board number (default to 0)");
  addOption ('p', "pad", 1,
	     "device number (default to 12, counted from 0, not from 1)");
}

Rts2DevSensorCryocon::~Rts2DevSensorCryocon (void)
{
  ibonl (gpib_dev, 0);
}

void
Rts2DevSensorCryocon::createTempInputValue (Rts2ValueDouble ** val, char chan,
					    const char *name,
					    const char *desc)
{
  char *n = new char[strlen (name) + 2];
  strcpy (n, name);
  strncat (n, &chan, 1);
  char *d = new char[strlen (desc) + 2];
  strcpy (d, desc);
  strncat (d, &chan, 1);
  createValue (*val, n, d, true);
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
  char buf[50] = "INPUT ";
  for (int i = 0; i < 4; i++)
    {
      buf[6] = chans[i]->getChannel ();
      buf[7] = ':';
      for (int v = 0; v < 6; v++)
	{
	  strcpy (buf + 8, chans[i]->values[v]->getName ().c_str ());
	  buf[strlen (buf) - 1] = '?';
	  ret = writeRead (buf, chans[i]->values[v]);
	  if (ret)
	    return ret;
	}
    }
  ret = writeRead ("STATS:TIME?", statTime);
  if (ret)
    return ret;
  statTime->setValueDouble (statTime->getValueDouble () * 60.0);
  ret = writeRead ("SYSTEM:AMBIENT?", amb);
  if (ret)
    return ret;
  ret = writeRead ("SYSTEM:HTRREAD?", htrread);
  if (ret)
    return ret;
  ret = writeRead ("SYSTEM:HTRHST?", htrhst);
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
