#include "camd.h"

class CameraDummyChip:public CameraChip
{
private:
  char *data;
  bool supportFrameT;
  int readoutSleep;
public:
    CameraDummyChip (Rts2DevCamera * in_cam, bool in_supportFrameT,
		     int in_readoutSleep, int in_width,
		     int in_height):CameraChip (in_cam, 0)
  {
    supportFrameT = in_supportFrameT;
    readoutSleep = in_readoutSleep;
    setSize (in_width, in_height, 0, 0);
    data = NULL;
  }
  virtual int startExposure (int light, float exptime)
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

  virtual bool supportFrameTransfer ()
  {
    return supportFrameT;
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
  if (readoutLine == 0 && readoutSleep > 0)
    usleep (readoutSleep);
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
private:
  bool supportFrameT;
  int infoSleep;
  Rts2ValueDouble *readoutSleep;
  int width;
  int height;
public:
    Rts2DevCameraDummy (int in_argc, char **in_argv):Rts2DevCamera (in_argc,
								    in_argv)
  {
    supportFrameT = false;
    infoSleep = 0;
    createValue (readoutSleep, "readout", "readout sleep in sec", true);
    readoutSleep->setValueDouble (0);

    width = 200;
    height = 100;
    addOption ('f', "frame_transfer", 0,
	       "when set, dummy CCD will act as frame transfer device");
    addOption ('I', "info_sleep", 1,
	       "device will sleep i nanosecunds before each info and baseInfo return");
    addOption ('r', "readout_sleep", 1,
	       "device will sleep i nanosecunds before each readout");
    addOption ('w', "width", 1, "width of simulated CCD");
    addOption ('g', "height", 1, "height of simulated CCD");
  }
  virtual ~ Rts2DevCameraDummy (void)
  {
    readoutSleep = NULL;
  }
  virtual int processOption (int in_opt)
  {
    switch (in_opt)
      {
      case 'f':
	supportFrameT = true;
	break;
      case 'I':
	infoSleep = atoi (optarg);
	break;
      case 'r':
	readoutSleep->setValueDouble (atoi (optarg));
	break;
      case 'w':
	width = atoi (optarg);
	break;
      case 'g':
	height = atoi (optarg);
	break;
      default:
	return Rts2DevCamera::processOption (in_opt);
      }
    return 0;
  }
  virtual int init ()
  {
    int ret;
    ret = Rts2DevCamera::init ();
    if (ret)
      return ret;

    usleep (infoSleep);
    strcpy (ccdType, "Dummy");
    strcpy (serialNumber, "1");

    return initChips ();
  }
  virtual int initChips ()
  {
    chips[0] =
      new CameraDummyChip (this, supportFrameT,
			   readoutSleep->getValueInteger (), width, height);
    chipNum = 1;
    return Rts2DevCamera::initChips ();
  }
  virtual int ready ()
  {
    return 0;
  }
  virtual int info ()
  {
    usleep (infoSleep);
    tempCCD->setValueDouble (100);
    return Rts2DevCamera::info ();
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
  int ret = device->run ();
  delete device;
  return ret;
}
