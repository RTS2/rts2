#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

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
	readoutLine++;
	sendLine++;
	sendReadoutData (data,
			 2 * (chipUsedReadout->width - chipUsedReadout->x));
	return 0;
      }
    return -2;
  }
  virtual int endReadout ()
  {
    return 0;
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
    return 0;
  }
  virtual int info ()
  {
    return 0;
  }
  virtual int camChipInfo ()
  {
    return 0;
  }
};

int
main (int argc, char **argv)
{
  Rts2DevCameraDummy *device = new Rts2DevCameraDummy (argc, argv);

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
