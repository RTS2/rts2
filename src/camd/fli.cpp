/**
 * Copyright (C) 2005 Petr Kubanek <petr@kubanek.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*!
 * @file FLI camera driver
 *
 * @author petr
 * 
 * You will need FLIlib and option to ./configure (--with-fli=<llibflidir>)
 * to get that running. You can get libfli on http://www.moronski.com/fli
 */

#include "camera_cpp.h"
#include "filter_fli.h"

#include "libfli.h"

#include <signal.h>

class CameraFliChip:public CameraChip
{
private:
  uint16_t * buf;
  uint16_t *dest_top;
  char *send_top;
  flidev_t dev;

public:
    CameraFliChip (Rts2DevCamera * in_cam, int in_chip_id, flidev_t in_fli);
    virtual ~ CameraFliChip (void);
  virtual int init ();
  virtual int setBinning (int in_vert, int in_hori);
  virtual int box (int in_x, int in_y, int in_width, int in_height);
  virtual int startExposure (int light, float exptime);
  virtual long isExposing ();
  virtual int stopExposure ();
  virtual int startReadout (Rts2DevConnData * dataConn, Rts2Conn * conn);
  virtual int endReadout ();
  virtual int readoutOneLine ();
};

CameraFliChip::CameraFliChip (Rts2DevCamera * in_cam, int in_chip_id,
			      flidev_t in_fli):
CameraChip (in_cam, in_chip_id)
{
  dev = in_fli;
  buf = NULL;
}

CameraFliChip::~CameraFliChip (void)
{
  delete buf;
}

int
CameraFliChip::init ()
{
  LIBFLIAPI ret;
  long x, y, w, h;

  ret = FLIGetVisibleArea (dev, &x, &y, &w, &h);
  if (ret)
    return -1;
  // put true width & height
  w -= x;
  h -= y;
  chipSize = new ChipSubset ((int) x, (int) y, (int) w, (int) h);
  chipReadout = new ChipSubset (0, 0, (int) w, (int) h);

  ret = FLIGetPixelSize (dev, &pixelX, &pixelY);
  if (ret)
    return -1;

  return 0;
}

int
CameraFliChip::setBinning (int in_vert, int in_hori)
{
  LIBFLIAPI ret;
  ret = FLISetVBin (dev, in_vert);
  if (ret)
    return -1;
  ret = FLISetHBin (dev, in_hori);
  if (ret)
    return -1;
  return CameraChip::setBinning (in_vert, in_hori);
}

int
CameraFliChip::box (int in_x, int in_y, int in_width, int in_height)
{
  // tests for -1 -> full size
  if (in_x == -1)
    in_x = 0;
  if (in_y == -1)
    in_y = 0;
  if (in_width == -1)
    in_width = chipSize->width;
  if (in_height == -1)
    in_height = chipSize->height;
  if (in_x < chipSize->x || in_y < chipSize->y
      || ((in_x - chipSize->x) + in_width) > chipSize->width
      || ((in_y - chipSize->y) + in_height) > chipSize->height)
    return -1;
  chipReadout->x = in_x;
  chipReadout->y = in_y;
  chipReadout->width = in_width;
  chipReadout->height = in_height;
  return 0;
}

int
CameraFliChip::startExposure (int light, float exptime)
{
  LIBFLIAPI ret;

  ret =
    FLISetImageArea (dev, chipSize->x + chipReadout->x,
		     chipSize->y + chipReadout->y,
		     chipSize->x + chipReadout->x +
		     chipReadout->width / binningHorizontal,
		     chipSize->y + chipReadout->y +
		     chipReadout->height / binningVertical);
  if (ret)
    return -1;

  ret = FLISetExposureTime (dev, (long) (exptime * 1000));
  if (ret)
    return -1;

  ret =
    FLISetFrameType (dev,
		     light ? FLI_FRAME_TYPE_NORMAL : FLI_FRAME_TYPE_DARK);
  if (ret)
    return -1;

  ret = FLIExposeFrame (dev);
  if (ret)
    return -1;

  chipUsedReadout = new ChipSubset (chipReadout);
  usedBinningVertical = binningVertical;
  usedBinningHorizontal = binningHorizontal;

  // alloc space for data..now we support only 16bit mode
  delete buf;
  buf = new uint16_t[(chipUsedReadout->width / usedBinningHorizontal)
		     * (chipUsedReadout->height / usedBinningVertical)];
  return 0;
}

long
CameraFliChip::isExposing ()
{
  LIBFLIAPI ret;
  long tl;
  ret = FLIGetExposureStatus (dev, &tl);
  if (ret)
    return -1;

  return tl * 1000;		// we get tl in msec, needs to return usec
}

int
CameraFliChip::stopExposure ()
{
  LIBFLIAPI ret;
  long timer;
  ret = FLIGetExposureStatus (dev, &timer);
  if (ret == 0 && timer > 0)
    {
      ret = FLICancelExposure (dev);
      if (ret)
	return ret;
    }
  delete buf;
  buf = NULL;
  return CameraChip::stopExposure ();
}

int
CameraFliChip::startReadout (Rts2DevConnData * dataConn, Rts2Conn * conn)
{
  dest_top = buf;
  send_top = (char *) buf;
  return CameraChip::startReadout (dataConn, conn);
}

int
CameraFliChip::endReadout ()
{
  delete buf;
  buf = NULL;
  return CameraChip::endReadout ();
}

int
CameraFliChip::readoutOneLine ()
{
  LIBFLIAPI ret;
  if (readoutLine <
      (chipUsedReadout->y + (chipUsedReadout->height / usedBinningVertical)))
    {
      // flush lines..
      if (readoutLine < chipUsedReadout->y)
	{
	  ret = FLIFlushRow (dev, chipUsedReadout->y, 1);
	  if (ret)
	    return -1;
	  readoutLine = chipUsedReadout->y;
	}
      // read lines..
      ret =
	FLIGrabRow (dev, dest_top,
		    chipUsedReadout->width / usedBinningHorizontal);
      if (ret)
	return -1;
      dest_top += (chipUsedReadout->width / usedBinningHorizontal);
      readoutLine++;
      return 0;
    }
  if (sendLine == 0)
    {
      int s_ret;
      s_ret = CameraChip::sendFirstLine ();
      if (s_ret)
	return s_ret;
    }
  long send_data_size;
  sendLine++;
  send_data_size = sendReadoutData (send_top, (char *) dest_top - send_top);
  if (send_data_size < 0)
    {
      return -1;
    }
  send_top += send_data_size;
  if (send_top < (char *) dest_top)
    return 0;
  return -2;
}

/*************************
 * Main camera class
 * 
 *************************/

class Rts2DevCameraFli:public Rts2DevCamera
{
private:
  char *deviceName;
  flidomain_t deviceDomain;

  flidev_t dev;

  char *filterDevice;

  int fliDebug;
public:
    Rts2DevCameraFli (int in_argc, char **in_argv);
    virtual ~ Rts2DevCameraFli (void);

  virtual int processOption (int in_opt);

  virtual int init ();

  virtual int ready ();
  virtual int info ();
  virtual int baseInfo ();

  virtual int camChipInfo (int chip);

  virtual int camCoolMax ();
  virtual int camCoolHold ();
  virtual int camCoolTemp (float new_temp);
  virtual int camCoolShutdown ();
};

Rts2DevCameraFli::Rts2DevCameraFli (int in_argc, char **in_argv):
Rts2DevCamera (in_argc, in_argv)
{
  deviceName = NULL;
  deviceDomain = FLIDEVICE_CAMERA | FLIDOMAIN_USB;
  filterDevice = NULL;
  fliDebug = FLIDEBUG_NONE;
  addOption ('f', "device_name", 1, "device name");
  addOption ('D', "domain", 1,
	     "CCD Domain (default to USB; possible values: USB|LPT|SERIAL|INET)");
  addOption ('L', "fli_filter", 1,
	     "FLI filter wheel (will have same domain as device)");
  addOption ('b', "fli_debug", 1,
	     "FLI debug level (1, 2 or 3; 3 will print most error message to stdout)");
}

Rts2DevCameraFli::~Rts2DevCameraFli (void)
{
  FLIClose (dev);
}

int
Rts2DevCameraFli::processOption (int in_opt)
{
  switch (in_opt)
    {
    case 'f':
      deviceName = optarg;
      break;
    case 'D':
      deviceDomain = FLIDEVICE_CAMERA;
      if (!strcasecmp ("USB", optarg))
	deviceDomain |= FLIDOMAIN_USB;
      else if (!strcasecmp ("LPT", optarg))
	deviceDomain |= FLIDOMAIN_PARALLEL_PORT;
      else if (!strcasecmp ("SERIAL", optarg))
	deviceDomain |= FLIDOMAIN_SERIAL;
      else if (!strcasecmp ("INET", optarg))
	deviceDomain |= FLIDOMAIN_INET;
      else
	return -1;
      break;
    case 'L':
      filterDevice = optarg;
      break;
    case 'b':
      switch (atoi (optarg))
	{
	case 1:
	  fliDebug = FLIDEBUG_FAIL;
	  break;
	case 2:
	  fliDebug = FLIDEBUG_FAIL | FLIDEBUG_WARN;
	  break;
	case 3:
	  fliDebug = FLIDEBUG_ALL;
	  break;
	default:
	  return -1;
	}
      break;
    default:
      return Rts2DevCamera::processOption (in_opt);
    }
  return 0;
}

int
Rts2DevCameraFli::init ()
{
  LIBFLIAPI ret;

  int ret_c;
  ret_c = Rts2DevCamera::init ();
  if (ret_c)
    return ret_c;

  if (fliDebug)
    FLISetDebugLevel (NULL, fliDebug);

  ret = FLIOpen (&dev, deviceName, deviceDomain);
  if (ret)
    return -1;

  chipNum = 1;
  chips[0] = new CameraFliChip (this, 0, dev);

  if (filterDevice)
    filter = new Rts2FilterFli (filterDevice, deviceDomain);

  return initChips ();
}

int
Rts2DevCameraFli::ready ()
{
  long fwrev;
  LIBFLIAPI ret;
  ret = FLIGetFWRevision (dev, &fwrev);
  if (ret)
    return -1;
  return 0;
}

int
Rts2DevCameraFli::info ()
{
  LIBFLIAPI ret;
  double fliTemp;
  ret = FLIGetTemperature (dev, &fliTemp);
  if (ret)
    return -1;
  tempCCD = fliTemp;
  return 0;
}

int
Rts2DevCameraFli::baseInfo ()
{
  LIBFLIAPI ret;
  long hwrev;
  long fwrev;
  ret = FLIGetModel (dev, ccdType, 64);
  if (ret)
    return -1;
  ret = FLIGetHWRevision (dev, &hwrev);
  if (ret)
    return -1;
  ret = FLIGetFWRevision (dev, &fwrev);
  if (ret)
    return -1;
  sprintf (serialNumber, "%li.%li", hwrev, fwrev);

  return 0;
}

int
Rts2DevCameraFli::camChipInfo (int chip)
{
  return 0;
}

int
Rts2DevCameraFli::camCoolMax ()
{
  return camCoolTemp (nightCoolTemp);
}

int
Rts2DevCameraFli::camCoolHold ()
{
  return camCoolTemp (nightCoolTemp);
}

int
Rts2DevCameraFli::camCoolTemp (float new_temp)
{
  LIBFLIAPI ret;
  ret = FLISetTemperature (dev, new_temp);
  if (ret)
    return -1;
  tempSet = new_temp;
  return 0;
}

int
Rts2DevCameraFli::camCoolShutdown ()
{
  return camCoolTemp (40);
}

Rts2DevCameraFli *device = NULL;

void
killSignal (int sig)
{
  delete device;
  exit (0);
}

int
main (int argc, char **argv)
{
  device = new Rts2DevCameraFli (argc, argv);

  signal (SIGTERM, killSignal);
  signal (SIGINT, killSignal);

  int ret;
  ret = device->init ();
  if (ret)
    {
      syslog (LOG_ERR, "Cannot initialize miniccd camera - exiting!\n");
      exit (1);
    }
  device->run ();
  delete device;
}
