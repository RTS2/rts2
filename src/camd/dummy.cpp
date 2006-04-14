#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <signal.h>

#include "camera_cpp.h"

class CameraDummyChip:public CameraChip
{
  char *data;
public:
    CameraDummyChip (Rts2DevCamera * in_cam):CameraChip (in_cam, 0)
  {
    setSize (100, 200, 0, 0);
    data = NULL;
  }
  virtual int startExposure (int light, float exptime)
  {
    return 0;
  }
  virtual int endExposure ()
  {
    return 0;
  }
  virtual int readoutOneLine ();

  virtual int readoutEnd ()
  {
    if (data)
      {
	delete data;
	data = NULL;
      }
    return CameraChip::endReadout ();
  }
};

int
CameraDummyChip::readoutOneLine ()
{
  int ret;
  if (sendLine == 0)
    {
      ret = CameraChip::sendFirstLine ();
      if (ret)
	return ret;
      data = new char[2 * (chipUsedReadout->width - chipUsedReadout->x)];
    }
//  if (readoutLine == 0)
//    sleep (10);
  for (int i = 0; i < 2 * chipUsedReadout->width; i++)
    {
      data[i] = i + readoutLine;
    }
  readoutLine++;
  sendLine++;
  ret = sendReadoutData (data,
			 2 * (chipUsedReadout->width - chipUsedReadout->x));
  if (ret < 0)
    return ret;
  if (readoutLine <
      (chipUsedReadout->y + chipUsedReadout->height) / usedBinningVertical)
    return 0;			// imediately send new data
  return -2;			// no more data.. 
}

class Rts2DevCameraDummy:public Rts2DevCamera
{

public:
  Rts2DevCameraDummy (int in_argc, char **in_argv):Rts2DevCamera (in_argc,
								  in_argv)
  {
    chips[0] = new CameraDummyChip (this);
    chipNum = 1;
  }
  virtual int ready ()
  {
    return 0;
  }
  virtual int baseInfo ()
  {
    sleep (1);
    strcpy (ccdType, "Dummy");
    strcpy (serialNumber, "1");
    return 0;
  }
  virtual int info ()
  {
    sleep (1);
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
