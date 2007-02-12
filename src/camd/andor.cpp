#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "camd.h"

#ifdef __GNUC__
#  if(__GNUC__ > 3 || __GNUC__ ==3)
#	define _GNUC3_
#  endif
#endif

#ifdef _GNUC3_
#  include <iostream>
#  include <fstream>
using namespace std;
#else
#  include <iostream.h>
#  include <fstream.h>
#endif

#include "atmcdLXd.h"

/**
 * Chip of Andor CCD.
 */
class CameraAndorChip:public CameraChip
{
private:
  unsigned short *dest;		// for chips..
  unsigned short *dest_top;
  char *send_top;
  bool useFT;
public:
    CameraAndorChip (Rts2DevCamera * in_cam, int in_chip_id, int in_width,
		     int in_height, double in_pixelX, double in_pixelY,
		     float in_gain, bool in_useFT);
    virtual ~ CameraAndorChip (void);
  virtual int init ();
  virtual int startExposure (int light, float exptime);
  virtual int stopExposure ();
  virtual long isExposing ();
  virtual int startReadout (Rts2DevConnData * dataConn, Rts2Conn * conn);
  virtual int readoutOneLine ();
  virtual bool supportFrameTransfer ();
};

CameraAndorChip::CameraAndorChip (Rts2DevCamera * in_cam, int in_chip_id,
				  int in_width, int in_height,
				  double in_pixelX, double in_pixelY,
				  float in_gain, bool in_useFT):
CameraChip (in_cam, in_chip_id, in_width, in_height, in_pixelX, in_pixelY,
	    in_gain)
{
  dest = new unsigned short[in_width * in_height];
  useFT = in_useFT;
};

CameraAndorChip::~CameraAndorChip (void)
{
  delete dest;
};


int
CameraAndorChip::init ()
{
  int ret;
  // use frame transfer mode
  if (useFT)
    {
      ret = SetFrameTransferMode (1);
      if (ret != DRV_SUCCESS)
	{
	  logStream (MESSAGE_ERROR) <<
	    "andor init cannot set frame transfer mode " << ret << sendLog;
	  return -1;
	}
    }
  return CameraChip::init ();
}

int
CameraAndorChip::startExposure (int light, float exptime)
{
  int ret;

  ret =
    SetImage (binningHorizontal, binningVertical, chipReadout->x + 1,
	      chipReadout->x + chipReadout->height, chipReadout->y + 1,
	      chipReadout->y + chipReadout->width);
  if (ret != DRV_SUCCESS)
    {
      logStream (MESSAGE_ERROR) << "andor SetImage return " << ret << sendLog;
      return -1;
    }

  subExposure = camera->getSubExposure ();
  if (!isnan (subExposure))
    {
      nAcc = (int) (exptime / subExposure);
      float acq_exp, acq_acc, acq_kinetic;
      if (nAcc == 0)
	{
	  nAcc = 1;
	  subExposure = exptime;
	}
      ret = SetAcquisitionMode (2);
      if (ret != DRV_SUCCESS)
	return -1;
      ret = SetExposureTime (subExposure);
      if (ret != DRV_SUCCESS)
	return -1;
      ret = SetNumberAccumulations (nAcc);
      if (ret != DRV_SUCCESS)
	return -1;
      ret = GetAcquisitionTimings (&acq_exp, &acq_acc, &acq_kinetic);
      if (ret != DRV_SUCCESS)
	return -1;
      exptime = nAcc * acq_exp;
      subExposure = acq_exp;
    }
  else
    {
      // single scan
      ret = SetAcquisitionMode (1);
      if (ret != DRV_SUCCESS)
	return -1;
      ret = SetExposureTime (exptime);
      if (ret != DRV_SUCCESS)
	return -1;
    }

  chipUsedReadout = new ChipSubset (chipReadout);
  usedBinningVertical = binningVertical;
  usedBinningHorizontal = binningHorizontal;

  SetShutter (1, light == 1 ? 0 : 2, 50, 50);
  ret = StartAcquisition ();
  if (ret != DRV_SUCCESS)
    return -1;
  return 0;
}

int
CameraAndorChip::stopExposure ()
{
  AbortAcquisition ();
  FreeInternalMemory ();
  return CameraChip::stopExposure ();
}

long
CameraAndorChip::isExposing ()
{
  int status;
  int ret;
  ret = CameraChip::isExposing ();
  if (ret)
    return ret;
  ret = GetStatus (&status);
  if (ret != DRV_SUCCESS)
    return -1;
  if (status == DRV_ACQUIRING)
    return 100;
  return 0;
}

int
CameraAndorChip::startReadout (Rts2DevConnData * dataConn, Rts2Conn * conn)
{
  dest_top = dest;
  send_top = (char *) dest;
  return CameraChip::startReadout (dataConn, conn);
}

int
CameraAndorChip::readoutOneLine ()
{
  int ret;
  if (readoutLine < chipUsedReadout->height)
    {
      int size =
	(chipUsedReadout->width / usedBinningHorizontal) *
	(chipUsedReadout->height / usedBinningVertical);
      ret = GetAcquiredData16 (dest, size);
      if (ret != DRV_SUCCESS)
	{
	  logStream (MESSAGE_ERROR) << "andor GetAcquiredData16 return " <<
	    ret << sendLog;
	  return -1;
	}
      readoutLine = chipUsedReadout->height;
      dest_top += size;
      return 0;
    }
  if (sendLine == 0)
    {
      ret = CameraChip::sendFirstLine ();
      if (ret)
	return ret;
    }
  int send_data_size;
  sendLine++;
  send_data_size = sendReadoutData (send_top, (char *) dest_top - send_top);
  if (send_data_size < 0)
    return -1;
  send_top += send_data_size;
  if (send_top < (char *) dest_top)
    return 0;
  return -2;
}

bool CameraAndorChip::supportFrameTransfer ()
{
  return useFT;
}

class
  Rts2DevCameraAndor:
  public
  Rts2DevCamera
{
private:
  char *
    andorRoot;
  int
    horizontalSpeed;
  int
    verticalSpeed;
  int
    vsamp;
  int
    adChannel;
  bool
    printSpeedInfo;
  // number of AD channels
  int
    chanNum;
  bool
    useFT;

  int
  printChannelInfo (int channel);

  void
  getTemp ();
protected:
  virtual int
  processOption (int in_opt);
  virtual void
  help ();
  virtual int
  setGain (double in_gain);
public:
  Rts2DevCameraAndor (int argc, char **argv);
  virtual ~
  Rts2DevCameraAndor (void);

  virtual int
  init ();

  // callback functions for Camera alone
  virtual int
  ready ();
  virtual int
  info ();
  virtual int
  camChipInfo (int chip);
  virtual int
  camCoolMax ();
  virtual int
  camCoolHold ();
  virtual int
  camCoolTemp (float new_temp);
  virtual int
  camCoolShutdown ();

  virtual int
  camExpose (int chip, int light, float exptime);
};

Rts2DevCameraAndor::Rts2DevCameraAndor (int in_argc, char **in_argv):
Rts2DevCamera (in_argc, in_argv)
{
  andorRoot = "/root/andor/examples/common";

  defaultGain = 255;
  gain->setValueDouble (defaultGain);

  horizontalSpeed = -1;
  verticalSpeed = -1;
  vsamp = -1;
  adChannel = -1;
  printSpeedInfo = false;
  chanNum = 0;

  useFT = true;

  addOption ('r', "root", 1, "directory with Andor detector.ini file");
  addOption ('g', "gain", 1, "set camera gain level (0-255)");
  addOption ('H', "horizontal_speed", 1, "set horizontal readout speed");
  addOption ('v', "vertical_speed", 1, "set vertical readout speed");
  addOption ('A', "vs_amplitude", 1, "VS amplitude (0-4)");
  addOption ('C', "ad_channel", 1, "set AD channel which will be used");
  addOption ('N', "noft", 0, "do not use frame transfer mode");
  addOption ('I', "speed_info", 0,
	     "print speed info - information about speed available");
}

Rts2DevCameraAndor::~Rts2DevCameraAndor (void)
{
  ShutDown ();
}

void
Rts2DevCameraAndor::help ()
{
  std::cout << "Driver for Andor CCDs (iXon & others)" << std::endl;
  std::
    cout << "Optimal values for vertical speed on iXon are: -H 1 -v 1 -C 1" <<
    std::endl;
  Rts2DevCamera::help ();
}

int
Rts2DevCameraAndor::setGain (double in_gain)
{
  int ret;
  ret = SetEMCCDGain ((int) in_gain);
  if (ret != DRV_SUCCESS)
    {
      logStream (MESSAGE_ERROR) << "andor setGain error " << ret << sendLog;
      return -1;
    }
  gain->setValueDouble (in_gain);
  return 0;
}

int
Rts2DevCameraAndor::processOption (int in_opt)
{
  switch (in_opt)
    {
    case 'C':
      adChannel = atoi (optarg);
      break;
    case 'g':
      defaultGain = atof (optarg);
      if (defaultGain > 255 || defaultGain < 0)
	{
	  printf ("gain must be in 0-255 range\n");
	  exit (EXIT_FAILURE);
	}
      gain->setValueDouble (defaultGain);
      break;
    case 'r':
      andorRoot = optarg;
      break;
    case 'H':
      horizontalSpeed = atoi (optarg);
      break;
    case 'v':
      verticalSpeed = atoi (optarg);
      break;
    case 'A':
      vsamp = atoi (optarg);
      if (vsamp < 0 || vsamp > 4)
	{
	  printf ("amplitude must be in 0-4 range\n");
	  exit (EXIT_FAILURE);
	}
    case 'I':
      printSpeedInfo = true;
      break;
    case 'N':
      useFT = false;
      break;
    default:
      return Rts2DevCamera::processOption (in_opt);
    }
  return 0;
}

int
Rts2DevCameraAndor::printChannelInfo (int channel)
{
  int ret;
  int speeds;
  float value;
  int depth;
  int i;
  ret = GetBitDepth (channel, &depth);
  if (ret != DRV_SUCCESS)
    {
      logStream (MESSAGE_ERROR) <<
	"andor printChannelInfo cannto get depth for channel " << channel <<
	sendLog;
      return -1;
    }
  logStream (MESSAGE_ERROR) << "andor printChannelInfo depth for channel " <<
    channel << " value: " << depth << sendLog;
  for (int j = 0; j < 2; j++)
    {
      ret = GetNumberHSSpeeds (channel, j, &speeds);
      if (ret != DRV_SUCCESS)
	{
	  logStream (MESSAGE_ERROR) <<
	    "andor printChannelInfo cannot get horizontal speeds for channel "
	    << channel << " type " << j << sendLog;
	  return -1;
	}
      for (i = 0; i < speeds; i++)
	{
	  ret = GetHSSpeed (channel, j, i, &value);
	  if (ret != DRV_SUCCESS)
	    {
	      logStream (MESSAGE_ERROR) <<
		"andor printChannelInfo cannot get horizontal speed " << i <<
		" channel " << channel << " type " << j << sendLog;
	      return -1;
	    }
	  std::cout <<
	    "andor printChannelInfo horizontal speed " << i << " channel " <<
	    channel << " type " << j << " value " << value << " MHz" <<
	    std::endl;
	}
    }
  return 0;
}

int
Rts2DevCameraAndor::init ()
{
  CameraAndorChip *cc;
  unsigned long error;
  int width;
  int height;
  int ret;

  ret = Rts2DevCamera::init ();
  if (ret)
    return ret;

  error = Initialize (andorRoot);

  if (error != DRV_SUCCESS)
    {
      cout << "Initialisation error...exiting" << endl;
      return -1;
    }

  sleep (2);			//sleep to allow initialization to complete

  //Set Read Mode to --Image--
  SetReadMode (4);

  //Get Detector dimensions
  GetDetector (&width, &height);

  //Initialize Shutter
  SetShutter (1, 0, 50, 50);

  SetExposureTime (5.0);
  setGain (defaultGain);

  // adChannel
  if (adChannel >= 0)
    {
      ret = SetADChannel (adChannel);
      if (ret != DRV_SUCCESS)
	{
	  logStream (MESSAGE_ERROR) << "andor init cannot set AD channel to "
	    << adChannel << sendLog;
	  return -1;
	}
    }

  // vertical amplitude
  if (vsamp >= 0)
    {
      ret = SetVSAmplitude (vsamp);
      if (ret != DRV_SUCCESS)
	{
	  logStream (MESSAGE_ERROR) << "andor init set vs amplitude to " <<
	    vsamp << " failed" << sendLog;
	  return -1;
	}
    }

  if (horizontalSpeed >= 0)
    {
      ret = SetHSSpeed (0, horizontalSpeed);
      if (ret != DRV_SUCCESS)
	{
	  logStream (MESSAGE_ERROR) <<
	    "andor init cannot set horizontal speed to channel " << adChannel
	    << ", type 0, value " << horizontalSpeed << sendLog;
	  return -1;
	}
    }

  if (verticalSpeed >= 0)
    {
      ret = SetVSSpeed (verticalSpeed);
      if (ret != DRV_SUCCESS)
	{
	  logStream (MESSAGE_ERROR) <<
	    "andor init cannot set vertical speed to " << verticalSpeed <<
	    sendLog;
	  return -1;
	}
    }

  chipNum = 1;

  cc = new CameraAndorChip (this, 0, width, height, 10, 10, 1, useFT);
  chips[0] = cc;
  chips[1] = NULL;

  if (printSpeedInfo)
    {
      int channels;
      int speeds;
      float value;
      int i;
      ret = GetNumberADChannels (&channels);
      if (ret != DRV_SUCCESS)
	{
	  logStream (MESSAGE_ERROR) <<
	    "andor init cannot get number of AD channels" << sendLog;
	  return -1;
	}
      logStream (MESSAGE_INFO) << "andor init channels " << channels <<
	sendLog;
      // print horizontal channels..
      for (i = 0; i < channels; i++)
	{
	  ret = printChannelInfo (i);
	  if (ret)
	    {
	      return ret;
	    }
	}

      // print vertical channels..
      ret = GetNumberVSSpeeds (&speeds);
      if (ret != DRV_SUCCESS)
	{
	  logStream (MESSAGE_ERROR) <<
	    "andor init cannot get horizontal speeds" << sendLog;
	  return -1;
	}
      for (i = 0; i < speeds; i++)
	{
	  ret = GetVSSpeed (i, &value);
	  if (ret != DRV_SUCCESS)
	    {
	      logStream (MESSAGE_ERROR) <<
		"andor init cannot get vertical speed " << i << sendLog;
	      return -1;
	    }
	  std::cout << "andor::init vertical speed " << i <<
	    " value " << value << " MHz" << std::endl;
	}
    }
  sprintf (ccdType, "ANDOR");

  return Rts2DevCamera::initChips ();
}

int
Rts2DevCameraAndor::ready ()
{
  return 0;
}

void
Rts2DevCameraAndor::getTemp ()
{
  int c_status;
  float tmpTemp;
  c_status = GetTemperatureF (&tmpTemp);
  if (!
      (c_status == DRV_ACQUIRING || c_status == DRV_NOT_INITIALIZED
       || c_status == DRV_ERROR_ACK))
    {
      tempCCD->setValueDouble (tmpTemp);
      tempRegulation->setValueInteger (c_status != DRV_TEMPERATURE_OFF);
    }
  else
    {
      logStream (MESSAGE_DEBUG) << "andor info status " << c_status <<
	sendLog;
    }
}

int
Rts2DevCameraAndor::info ()
{
  tempAir->setValueDouble (nan ("f"));
  coolingPower->setValueInteger ((int) (50 * 1000));
  if (isIdle ())
    {
      getTemp ();
    }
  return Rts2DevCamera::info ();
}

int
Rts2DevCameraAndor::camChipInfo (int chip)
{
  return 0;
}

int
Rts2DevCameraAndor::camCoolMax ()
{
  return camCoolHold ();
}

int
Rts2DevCameraAndor::camCoolHold ()
{
  if (isnan (nightCoolTemp))
    return camCoolTemp (-5);
  else
    return camCoolTemp (nightCoolTemp);
}

int
Rts2DevCameraAndor::camCoolTemp (float new_temp)
{
  CoolerON ();
  SetTemperature ((int) new_temp);
  tempSet->setValueDouble (new_temp);
  return 0;
}

int
Rts2DevCameraAndor::camCoolShutdown ()
{
  CoolerOFF ();
  SetTemperature (20);
  tempSet->setValueDouble (+50);
  return 0;
}

int
Rts2DevCameraAndor::camExpose (int chip, int light, float exptime)
{
  getTemp ();
  return Rts2DevCamera::camExpose (chip, light, exptime);
}

int
main (int argc, char **argv)
{
  Rts2DevCameraAndor *device = new Rts2DevCameraAndor (argc, argv);
  int ret = device->run ();
  delete device;
  return ret;
}
