#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#define MAX_CHIPS 	2	//maximal number of chips

#include <math.h>
#include <mcheck.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "camera_cpp.h"
#include "../utils/rts2device.h"
#include "../utils/rts2block.h"

#include "sbigudrv.h"
#include "csbigcam.h"

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

class CameraAndorChip:public CameraChip
{
  long *dest;		// for chips..
  long *dest_top;
  long *send_top;
public:
    CameraAndorChip (int in_chip_id, int in_width, int in_height,
		    int in_pixelX, int in_pixelY, float in_gain);
    ~CameraAndorChip (void)
    {
	delete dest;
    }
  virtual int startReadout (Rts2DevConnData * dataConn, Rts2Conn * conn);
  virtual int readoutOneLine ();
};

CameraAndorChip::CameraAndorChip (int in_chip_id, int in_width, int in_height,
				int in_pixelX, int in_pixelY, float in_gain):
CameraChip (in_chip_id, in_width, in_height, in_pixelX, in_pixelY, in_gain)
{
  dest = new long[in_width * in_height];
};

int
CameraAndorChip::startReadout (Rts2DevConnData * dataConn, Rts2Conn * conn)
{
  dest_top = dest;
  send_top = dest;
  return CameraChip::startReadout (dataConn, conn);
}

int
CameraAndorChip::readoutOneLine ()
{
  if (readoutLine < 0)
    return -1;
  if (readoutLine < chipSize->height)
    {
      readoutLine = chipSize->height;
      GetAcquiredData (dest, chipSize->height * chipSize->width);
      return 0;
    }
  if (sendLine == 0)
    {
      int ret;
      ret = CameraChip::sendFirstLine ();
      if (ret)
	return ret;
    }
  if (send_top < dest_top)
    {
      sendLine++;
      send_top +=
	readoutConn->send (send_top,
			   chipSize->width);
      return 0;
    }
  readoutConn->endConnection ();
  readoutLine = -1;
  return -2;
}

class Rts2DevCameraAndor:public Rts2DevCamera
{
public:
    Rts2DevCameraAndor (int argc, char **argv);
  ~Rts2DevCameraAndor (void);

  virtual int init ();

  // callback functions for Camera alone
  virtual int camReady ();
  virtual int camInfo ();
  virtual int camBaseInfo ();
  virtual int camChipInfo (int chip);
  virtual int camExpose (int chip, int light, float exptime);
  virtual int camWaitExpose (int chip);
  virtual int camStopExpose (int chip);
  virtual int camBox (int chip, int x, int y, int width, int height);
  virtual int camBinning (int chip, int x_bin, int y_bin);
  virtual int camStopRead (int chip);
  virtual int camCoolMax ();
  virtual int camCoolHold ();
  virtual int camCoolTemp (float new_temp);
  virtual int camCoolShutdown ();
  virtual int camFilter (int new_filter);
};

Rts2DevCameraAndor::Rts2DevCameraAndor (int argc, char **argv):
Rts2DevCamera (argc, argv)
{
}

Rts2DevCameraAndor::~Rts2DevCameraAndor (void)
{
}

int
Rts2DevCameraAndor::init ()
{
  CameraAndorChip *cc;
  unsigned long error;
  int width;
  int height;

  error = Initialize ("/root/andor/examples/common");

  if(error!=DRV_SUCCESS){
	cout << "Initialisation error...exiting" << endl;
	return -1;
  }

  sleep(2); //sleep to allow initialization to complete
  
  //Set Read Mode to --Image--
  SetReadMode(4);
  
  //Set Acquisition mode to --Single scan--
  SetAcquisitionMode(1);

  //Get Detector dimensions
  GetDetector(&width, &height);

  chipNum = 1;
  
  cc =
    new CameraAndorChip (0, width, height, 10, 10, 1);
  chips[0] = cc;
  chips[1] = NULL;

  return Rts2DevCamera::init ();
}

int
Rts2DevCameraAndor::camReady ()
{
  return 0;
}

int
Rts2DevCameraAndor::camInfo ()
{
  tempAir = 10;
  tempSet = -10;
  coolingPower = (int) (50 * 1000);
  tempCCD = -15;
  tempRegulation = 1;
  return 0;
}

int
Rts2DevCameraAndor::camBaseInfo ()
{
  sprintf (ccdType, "ANDOR");
  // get serial number
  return 0;
}

int
Rts2DevCameraAndor::camChipInfo (int chip)
{
  return 0;
}

int
Rts2DevCameraAndor::camExpose (int chip, int light, float exptime)
{
  SetExposureTime(exptime);
  StartAcquisition();
  return 0;
}

int
Rts2DevCameraAndor::camWaitExpose (int chip)
{
  int status;
  GetStatus (&status);
  if (status == DRV_ACQUIRING)
    return 0;
  return -2;
}

int
Rts2DevCameraAndor::camStopExpose (int chip)
{
  // not supported
  return -1;
}

int
Rts2DevCameraAndor::camBox (int chip, int x, int y, int width, int height)
{
  return -1;
}

int
Rts2DevCameraAndor::camBinning (int chip, int x_bin, int y_bin)
{
  return 0;
}

int
Rts2DevCameraAndor::camStopRead (int chip)
{
  return -1;
}

int
Rts2DevCameraAndor::camCoolMax ()
{
  return -1;
}

int
Rts2DevCameraAndor::camCoolHold ()
{
  return -1;
}

int
Rts2DevCameraAndor::camCoolTemp (float new_temp)
{
  return -1;
}

int
Rts2DevCameraAndor::camCoolShutdown ()
{
  return -1;
}

int
Rts2DevCameraAndor::camFilter (int new_filter)
{
  return -1;
}

int
main (int argc, char **argv)
{
  mtrace ();

  Rts2DevCameraAndor *device = new Rts2DevCameraAndor (argc, argv);

  int ret;
  ret = device->init ();
  if (ret)
    {
      fprintf (stderr, "Cannot initialize camera - exiting!\n");
      exit (0);
    }
  device->run ();
  delete device;
}
