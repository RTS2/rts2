#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

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

unsigned int
ccd_c2ad (double t)
{
  double r = 3.0 * exp (0.94390589891 * (25.0 - t) / 25.0);
  unsigned int ret = (unsigned int) (4096.0 / (10.0 / r + 1.0));
  if (ret > 4096)
    ret = 4096;
  return ret;
}

double
ccd_ad2c (unsigned int ad)
{
  double r;
  if (ad == 0)
    return 1000;
  r = 10.0 / (4096.0 / (double) ad - 1.0);
  return 25.0 - 25.0 * (log (r / 3.0) / 0.94390589891);
}

double
ambient_ad2c (unsigned int ad)
{
  double r;
  r = 3.0 / (4096.0 / (double) ad - 1.0);
  return 25.0 - 45.0 * (log (r / 3.0) / 2.0529692213);
}

class CameraSbigChip:public CameraChip
{
  GetCCDInfoResults0 chip_info;
  ReadoutLineParams rlp;
  unsigned short *dest;		// for chips..
  unsigned short *dest_top;
  char *send_top;
  int sbig_readout_mode;
public:
    CameraSbigChip (int in_chip_id, int in_width, int in_height,
		    float in_pixelX, float in_pixelY, float in_gain);
    virtual ~ CameraSbigChip ();
  virtual int setBinning (int in_vert, int in_hori)
  {
    if (in_vert != in_hori || in_vert < 1 || in_vert > 3)
      return -1;
    // use only modes 0, 1, 2, which equals to binning 1x1, 2x2, 3x3
    // it doesn't seems that Sbig offers more binning :(
    sbig_readout_mode = in_vert - 1;
    return CameraChip::setBinning (in_vert, in_hori);
  }
  virtual long isExposing ();
  virtual int startReadout (Rts2DevConnData * dataConn, Rts2Conn * conn);
  virtual int endReadout ();
  virtual int readoutOneLine ();
};

CameraSbigChip::CameraSbigChip (int in_chip_id, int in_width, int in_height,
				float in_pixelX, float in_pixelY,
				float in_gain):
CameraChip (in_chip_id, in_width, in_height, in_pixelX, in_pixelY, in_gain)
{
  dest = new unsigned short[in_width * in_height];
  GetCCDInfoParams req;
  req.request = (chipId == 0 ? CCD_INFO_IMAGING : CCD_INFO_TRACKING);
  sbig_readout_mode = 0;
  SBIGUnivDrvCommand (CC_GET_CCD_INFO, &req, &chip_info);
};

CameraSbigChip::~CameraSbigChip ()
{
  delete dest;
}

long
CameraSbigChip::isExposing ()
{
  long ret;
  ret = CameraChip::isExposing ();
  if (ret > 0)
    return ret;

  QueryCommandStatusParams qcsp;
  QueryCommandStatusResults qcsr;

  int complete = FALSE;

  qcsp.command = CC_START_EXPOSURE;
  if (SBIGUnivDrvCommand (CC_QUERY_COMMAND_STATUS, &qcsp, &qcsr) ==
      CE_NO_ERROR)
    {
      if (chipId == 0)
	complete = (qcsr.status & 0x03) != 0x02;
      else
	complete = (qcsr.status & 0x0C) != 0x08;
    }
  if (complete)
    return -2;
  return 0;
}

int
CameraSbigChip::startReadout (Rts2DevConnData * dataConn, Rts2Conn * conn)
{
  int ret = CameraChip::startReadout (dataConn, conn);
  StartReadoutParams srp;
  srp.ccd = chipId;
  srp.left = srp.top = 0;
  srp.height = chipReadout->height;
  srp.width = chipReadout->width;
  srp.readoutMode = sbig_readout_mode;
  SBIGUnivDrvCommand (CC_START_READOUT, &srp, NULL);
  rlp.ccd = chipId;
  rlp.pixelStart = chipUsedReadout->x / usedBinningVertical;
  rlp.pixelLength = chipUsedReadout->width / usedBinningVertical;
  rlp.readoutMode = sbig_readout_mode;
  dest_top = dest;
  send_top = (char *) dest;
  return ret;
}

int
CameraSbigChip::endReadout ()
{
  EndReadoutParams erp;
  erp.ccd = chipId;
  SBIGUnivDrvCommand (CC_END_READOUT, &erp, NULL);
  return CameraChip::endReadout ();
}

int
CameraSbigChip::readoutOneLine ()
{
  if (readoutLine < 0)
    return -1;
  if (readoutLine <
      (chipUsedReadout->y + chipUsedReadout->height) / usedBinningVertical)
    {
      if (readoutLine < chipUsedReadout->y)
	{
	  DumpLinesParams dlp;
	  dlp.ccd = chipId;
	  dlp.lineLength =
	    ((chipUsedReadout->y - readoutLine) / usedBinningVertical);
	  dlp.readoutMode = sbig_readout_mode;
	  SBIGUnivDrvCommand (CC_DUMP_LINES, &dlp, NULL);
	  readoutLine = chipReadout->y;
	}
      SBIGUnivDrvCommand (CC_READOUT_LINE, &rlp, dest_top);
      dest_top += rlp.pixelLength;
      readoutLine++;
      return 0;
    }
  if (sendLine == 0)
    {
      int ret;
      ret = CameraChip::sendFirstLine ();
      if (ret)
	return ret;
    }
  if (!readoutConn)
    {
      return -3;
    }
  if (send_top < (char *) dest_top)
    {
      int send_data_size;
      sendLine++;
      send_data_size =
	sendReadoutData (send_top, (char *) dest_top - send_top);
      if (send_data_size < 0)
	return -2;

      send_top += send_data_size;
      return 0;
    }
  endReadout ();
  return -2;
}

class Rts2DevCameraSbig:public Rts2DevCamera
{
  CSBIGCam *pcam;
  int checkSbigHw (PAR_ERROR ret)
  {
    if (ret == CE_NO_ERROR)
      return 0;
    syslog (LOG_ERR, "Rts2DevCameraSbig::checkSbigHw ret: %i", ret);
    return -1;
  }
public:
    Rts2DevCameraSbig (int argc, char **argv);
  virtual ~ Rts2DevCameraSbig ();

  virtual int init ();

  // callback functions for Camera alone
  virtual int ready ();
  virtual int info ();
  virtual int baseInfo ();
  virtual int camChipInfo (int chip);
  virtual int camExpose (int chip, int light, float exptime);
  virtual long camWaitExpose (int chip);
  virtual int camStopExpose (int chip);
  virtual int camBox (int chip, int x, int y, int width, int height);
  virtual int camStopRead (int chip);
  virtual int camCoolMax ();
  virtual int camCoolHold ();
  virtual int camCoolTemp (float new_temp);
  virtual int camCoolShutdown ();
  virtual int camFilter (int new_filter);
};

Rts2DevCameraSbig::Rts2DevCameraSbig (int argc, char **argv):
Rts2DevCamera (argc, argv)
{
  pcam = NULL;
}

Rts2DevCameraSbig::~Rts2DevCameraSbig ()
{
  delete pcam;
}

int
Rts2DevCameraSbig::init ()
{
  int ret_c_init;
  ret_c_init = Rts2DevCamera::init ();
  if (ret_c_init)
    return ret_c_init;
  pcam = new CSBIGCam (DEV_USB);
  if (pcam->GetError () != CE_NO_ERROR)
    {
      delete pcam;
      return -1;
    }
  pcam->EstablishLink ();
  if (pcam->GetError () != CE_NO_ERROR)
    {
      delete pcam;
      return -1;
    }
  // get device information
  chipNum = 2;			// hardcoded

  // init chips
  GetCCDInfoParams req;
  GetCCDInfoResults0 res;

  CameraSbigChip *cc;
  PAR_ERROR ret;

  req.request = CCD_INFO_IMAGING;
  ret = pcam->SBIGUnivDrvCommand (CC_GET_CCD_INFO, &req, &res);
  if (ret != CE_NO_ERROR)
    {
      return -1;
    }
  cc =
    new CameraSbigChip (0, res.readoutInfo[0].width,
			res.readoutInfo[0].height,
			res.readoutInfo[0].pixelWidth,
			res.readoutInfo[0].pixelHeight,
			res.readoutInfo[0].gain);
  chips[0] = cc;

  req.request = CCD_INFO_TRACKING;
  ret = pcam->SBIGUnivDrvCommand (CC_GET_CCD_INFO, &req, &res);
  if (ret != CE_NO_ERROR)
    {
      return -1;
    }
  cc =
    new CameraSbigChip (1, res.readoutInfo[0].width,
			res.readoutInfo[0].height / 2,
			res.readoutInfo[0].pixelWidth,
			res.readoutInfo[0].pixelHeight,
			res.readoutInfo[0].gain);
  chips[1] = cc;

  return Rts2DevCamera::initChips ();
}

int
Rts2DevCameraSbig::ready ()
{
  double ccdTemp;
  PAR_ERROR ret;
  ret = pcam->GetCCDTemperature (ccdTemp);
  return checkSbigHw (ret);
}

int
Rts2DevCameraSbig::info ()
{
  MY_LOGICAL enabled;
  double amb, ccd, set, per;
  if (pcam->QueryTemperatureStatus (enabled, amb, set, per) != CE_NO_ERROR)
    return -1;
  fan = enabled;
  tempAir = amb;
  tempSet = set;
  coolingPower = (int) (per * 1000);
  if (pcam->GetCCDTemperature (ccd) != CE_NO_ERROR)
    return -1;
  tempCCD = ccd;
  tempRegulation = 1;
  return 0;
}

int
Rts2DevCameraSbig::baseInfo ()
{
  GetDriverInfoResults0 gccdir0;
  pcam->GetDriverInfo (DRIVER_STD, gccdir0);
  if (pcam->GetError () != CE_NO_ERROR)
    return -1;
  sprintf (ccdType, "SBIG_%i", pcam->GetCameraType ());
  // get serial number

  GetCCDInfoParams req;
  GetCCDInfoResults2 res;
  PAR_ERROR ret;

  req.request = CCD_INFO_EXTENDED2_IMAGING;
  ret = pcam->SBIGUnivDrvCommand (CC_GET_CCD_INFO, &req, &res);
  if (ret != CE_NO_ERROR)
    return -1;
  //strncpy (serialNumber, res.serialNumber, 10);
  strncpy (serialNumber, "007", 10);
  canDF = 1;
  return 0;
}

int
Rts2DevCameraSbig::camChipInfo (int chip)
{
  return 0;
}

int
Rts2DevCameraSbig::camExpose (int chip, int light, float exptime)
{
  PAR_ERROR ret;
  if (!pcam->CheckLink ())
    return -1;

  StartExposureParams sep;
  sep.ccd = chip;
  sep.exposureTime = (unsigned long) (exptime * 100.0 + 0.5);
  if (sep.exposureTime < 1)
    sep.exposureTime = 1;
  sep.abgState = ABG_LOW7;
  sep.openShutter = light ? SC_OPEN_SHUTTER : SC_CLOSE_SHUTTER;

  ret = pcam->SBIGUnivDrvCommand (CC_START_EXPOSURE, &sep, NULL);
  return checkSbigHw (ret);
}

long
Rts2DevCameraSbig::camWaitExpose (int chip)
{
  long ret;
  ret = Rts2DevCamera::camWaitExpose (chip);
  if (ret == -2)
    {
      camStopExpose (chip);	// SBIG devices are strange, there is same command for forced stop and normal stop
      return -2;
    }
  return ret;
}

int
Rts2DevCameraSbig::camStopExpose (int chip)
{
  PAR_ERROR ret;
  if (!pcam->CheckLink ())
    {
      return DEVDEM_E_HW;
    }

  EndExposureParams eep;
  eep.ccd = chip;

  ret = pcam->SBIGUnivDrvCommand (CC_END_EXPOSURE, &eep, NULL);
  return checkSbigHw (ret);
}

int
Rts2DevCameraSbig::camBox (int chip, int x, int y, int width, int height)
{
  return -1;
}

int
Rts2DevCameraSbig::camStopRead (int chip)
{
  return -1;
}

int
Rts2DevCameraSbig::camCoolMax ()
{
  SetTemperatureRegulationParams temp;
  PAR_ERROR ret;
  temp.regulation = REGULATION_OVERRIDE;
  temp.ccdSetpoint = 255;
  ret = pcam->SBIGUnivDrvCommand (CC_SET_TEMPERATURE_REGULATION, &temp, NULL);
  return checkSbigHw (ret);
}

int
Rts2DevCameraSbig::camCoolHold ()
{
  if (isnan (nightCoolTemp))
    return camCoolTemp (-5);
  return camCoolTemp (nightCoolTemp);
}

int
Rts2DevCameraSbig::camCoolTemp (float new_temp)
{
  SetTemperatureRegulationParams temp;
  PAR_ERROR ret;
  temp.regulation = REGULATION_ON;
  syslog (LOG_DEBUG, "Rts2DevCameraSbig::camCoolTemp setTemp: %f", new_temp);
  temp.ccdSetpoint = ccd_c2ad (new_temp);
  ret = pcam->SBIGUnivDrvCommand (CC_SET_TEMPERATURE_REGULATION, &temp, NULL);
  return checkSbigHw (ret);
}

int
Rts2DevCameraSbig::camCoolShutdown ()
{
  SetTemperatureRegulationParams temp;
  PAR_ERROR ret;
  temp.regulation = REGULATION_OFF;
  temp.ccdSetpoint = 0;
  ret = pcam->SBIGUnivDrvCommand (CC_SET_TEMPERATURE_REGULATION, &temp, NULL);
  return checkSbigHw (ret);
}

int
Rts2DevCameraSbig::camFilter (int new_filter)
{


}

int
main (int argc, char **argv)
{
  mtrace ();

  Rts2DevCameraSbig *device = new Rts2DevCameraSbig (argc, argv);

  int ret;
  ret = device->init ();
  if (ret)
    {
      syslog (LOG_ERR, "Cannot initialize sbigusb camera - exiting!\n");
      exit (1);
    }
  device->run ();
  delete device;
}
