#include "sensorgpib.h"

int
Rts2DevSensorGpib::gpibWrite (const char *buf)
{
  int ret;
  ret = ibwrt (gpib_dev, buf, strlen (buf));
#ifdef DEBUG_EXTRA
  logStream (MESSAGE_DEBUG) << "write " << buf << sendLog;
#endif
  if (ret & ERR)
    {
      logStream (MESSAGE_ERROR) << "error writing " << buf << sendLog;
      return -1;
    }
  return 0;
}

int
Rts2DevSensorGpib::gpibRead (char *buf, int blen)
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
Rts2DevSensorGpib::gpibWriteRead (char *buf, char *val)
{
  int ret;
  *val = '\0';
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
  ret = ibrd (gpib_dev, val, 50);
  if (ret & ERR)
    {
      logStream (MESSAGE_ERROR) << "error reading reply from " << buf <<
	", readed " << val << " " << ret << sendLog;
      return -1;
    }
#ifdef DEBUG_EXTRA
  logStream (MESSAGE_DEBUG) << "dev " << gpib_dev << " read " << val <<
    " ret " << ret << sendLog;
#endif
  return 0;
}

int
Rts2DevSensorGpib::processOption (int in_opt)
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
Rts2DevSensorGpib::init ()
{
  int ret;
  ret = Rts2DevSensor::init ();
  if (ret)
    return ret;

  gpib_dev = ibdev (minor, pad, 0, T3s, 1, 0);
  if (gpib_dev < 0)
    {
      logStream (MESSAGE_ERROR) << "cannot init GPIB device on minor " <<
	minor << ", pad " << pad << sendLog;
      return -1;
    }
  return 0;
}

Rts2DevSensorGpib::Rts2DevSensorGpib (int in_argc, char **in_argv):Rts2DevSensor (in_argc,
	       in_argv)
{
  gpib_dev = -1;

  minor = 0;
  pad = 0;

  addOption ('m', "minor", 1, "board number (default to 0)");
  addOption ('p', "pad", 1, "device number (counted from 0, not from 1)");
}

Rts2DevSensorGpib::~Rts2DevSensorGpib (void)
{
  ibonl (gpib_dev, 0);
}
