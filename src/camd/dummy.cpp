#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <signal.h>

#include "camera_cpp.h"

class CameraDummyChip:public CameraChip
{

public:
  CameraDummyChip (void):CameraChip (0)
  {
    setSize (100, 200, 0, 0);
  }
  virtual int startExposure (int light, float exptime)
  {
    return 0;
  }
  virtual int endExposure ()
  {
    return 0;
  }
  virtual int readoutOneLine ()
  {
    int ret;
    if (readoutLine < 0)
      return -1;
    if (sendLine == 0)
      {
	ret = CameraChip::sendFirstLine ();
	if (ret)
	  return ret;
      }
    if (!readoutConn)
      {
	return -3;
      }
    if (readoutLine <
	(chipUsedReadout->y + chipUsedReadout->height) / usedBinningVertical)
      {
	char *data;
	data = new char[2 * (chipUsedReadout->width - chipUsedReadout->x)];
	for (int i = 0; i < 2 * chipUsedReadout->width; i++)
	  {
	    data[i] = i;
	  }
	readoutLine++;
	sendLine++;
	sendReadoutData (data,
			 2 * (chipUsedReadout->width - chipUsedReadout->x));
	return 0;
      }
    return -2;
  }
};

class Rts2DevCameraDummy:public Rts2DevCamera
{

public:
  Rts2DevCameraDummy (int argc, char **argv):Rts2DevCamera (argc, argv)
  {
    chips[0] = new CameraDummyChip ();
    chipNum = 1;
  }
  virtual int ready ()
  {
    return 0;
  }
  virtual int baseInfo ()
  {
    strcpy (ccdType, "Dummy");
    strcpy (serialNumber, "1");
    return 0;
  }
  virtual int info ()
  {
    tempCCD = 100;
    return 0;
  }
  virtual int camChipInfo ()
  {
    return 0;
  }
};

Rts2DevCameraDummy *device;

void
killSignal (int sig)
{
  if (device)
    delete device;
  exit (0);
}

int
main (int argc, char **argv)
{
  device = new Rts2DevCameraDummy (argc, argv);

  signal (SIGTERM, killSignal);
  signal (SIGINT, killSignal);

  int ret;
  ret = device->init ();
  if (ret)
    {
      syslog (LOG_ERR, "Cannot initialize dummy camera - exiting!\n");
      exit (1);
    }
  device->run ();
  delete device;
}
