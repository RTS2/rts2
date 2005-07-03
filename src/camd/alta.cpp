/*!
 * Driver for Apogee Alta cameras.
 *
 * Based on test_alta.cpp from Alta source.
 *
 * @author petr
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <math.h>
#include <signal.h>

#include "camera_cpp.h"
#include "ApnCamera.h"

class CameraChipAlta:public CameraChip
{
private:
  CApnCamera * alta;
  short unsigned int *dest;
  short unsigned int *dest_top;
  char *send_top;
public:
    CameraChipAlta (CApnCamera * in_alta);
  virtual int init ();
  virtual int setBinning (int in_vert, int in_hori);
  virtual int startExposure (int light, float exptime);
  virtual long isExposing ();
  virtual int endExposure ();
  virtual int stopExposure ();
  virtual int startReadout (Rts2DevConnData * dataConn, Rts2Conn * conn);
  virtual int readoutOneLine ();
  virtual int endReadout ();
};

CameraChipAlta::CameraChipAlta (CApnCamera * in_alta):CameraChip (0)
{
  alta = in_alta;
}

int
CameraChipAlta::init ()
{
  setSize (alta->m_RoiPixelsH, alta->m_RoiPixelsV, 0, 0);
  binningHorizontal = alta->m_RoiBinningH;
  binningVertical = alta->m_RoiBinningV;
  pixelX = nan ("f");
  pixelY = nan ("f");
  gain = alta->m_ReportedGainSixteenBit;

  dest = new short unsigned int[chipSize->width * chipSize->height];
  return 0;
}

int
CameraChipAlta::setBinning (int in_vert, int in_hori)
{
  alta->m_RoiBinningH = in_vert;
  alta->m_RoiBinningV = in_hori;
  return 0;
}

int
CameraChipAlta::startExposure (int light, float exptime)
{
  int ret;
  ret = alta->Expose (exptime, light);
  if (!ret)
    return -1;

  chipUsedReadout = new ChipSubset (chipReadout);
  usedBinningVertical = binningVertical;
  usedBinningHorizontal = binningHorizontal;
  return 0;
}

long
CameraChipAlta::isExposing ()
{
  long ret;
  Apn_Status status;
  ret = CameraChip::isExposing ();
  if (ret > 0)
    return ret;

  status = alta->read_ImagingStatus ();

  if (status != Apn_Status_ImageReady)
    return 2000000;
  if (status == -1)
    return -1;
  // exposure has ended.. 
  return -2;
}

int
CameraChipAlta::endExposure ()
{
  return CameraChip::endExposure ();
}

int
CameraChipAlta::stopExposure ()
{
  // we need to digitize image:(
  alta->StopExposure (true);
  return CameraChip::stopExposure ();
}

int
CameraChipAlta::startReadout (Rts2DevConnData * dataConn, Rts2Conn * conn)
{
  int ret;
  ret = CameraChip::startReadout (dataConn, conn);
  // set region of intereset..
  alta->m_RoiStartX = chipUsedReadout->x;
  alta->m_RoiStartY = chipUsedReadout->y;
  alta->m_RoiPixelsH = chipUsedReadout->width;
  alta->m_RoiPixelsV = chipUsedReadout->height;
  dest_top = dest;
  send_top = (char *) dest;
  return ret;
}

int
CameraChipAlta::readoutOneLine ()
{
  int ret;

  if (readoutLine < 0)
    return -1;

  if (readoutLine <
      (chipUsedReadout->y + chipUsedReadout->height) / usedBinningVertical)
    {
      bool status;
      unsigned short width, height;
      width = chipUsedReadout->width;
      height = chipUsedReadout->height;
      unsigned long count;
      status = alta->GetImageData (dest_top, width, height, count);
      if (!status)
	return -3;
      dest_top += width * height;
      readoutLine = chipUsedReadout->height + chipUsedReadout->y;
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

int
CameraChipAlta::endReadout ()
{
  // reset system??
  return CameraChip::endReadout ();
}

class Rts2DevCameraAlta:public Rts2DevCamera
{
private:
  CApnCamera * alta;
public:
  Rts2DevCameraAlta::Rts2DevCameraAlta (int argc, char **argv);
  virtual ~ Rts2DevCameraAlta (void);
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

Rts2DevCameraAlta::Rts2DevCameraAlta (int argc, char **argv):
Rts2DevCamera (argc, argv)
{
  alta = NULL;
}

Rts2DevCameraAlta::~Rts2DevCameraAlta (void)
{
  if (alta)
    delete alta;
}

int
Rts2DevCameraAlta::init ()
{
  int ret;
  ret = Rts2DevCamera::init ();
  if (ret)
    {
      return ret;
    }
  alta = (CApnCamera *) new CApnCamera ();
  ret = alta->InitDriver (0, 0, 0);

  if (!ret)
    return -1;

  // Do a system reset to ensure known state, flushing enabled etc
  ret = alta->ResetSystem ();

  if (!ret)
    return -1;

  chipNum = 1;
  chips[0] = new CameraChipAlta (alta);

  return Rts2DevCamera::initChips ();
}

int
Rts2DevCameraAlta::ready ()
{
  int ret;
  ret = alta->read_Present ();
  return (ret ? 0 : -1);
}

int
Rts2DevCameraAlta::baseInfo ()
{
  strcpy (ccdType, "Alta ");
  strncat (ccdType, alta->m_CameraModel, 10);
  sprintf (serialNumber, "%i", alta->m_CameraId);
  return 0;
}

int
Rts2DevCameraAlta::info ()
{
  tempRegulation = alta->read_CoolerEnable ();
  tempSet = alta->read_CoolerSetPoint ();
  tempCCD = alta->read_TempCCD ();
  tempAir = alta->read_TempHeatsink ();
  fan = alta->read_FanMode () == Apn_FanMode_Low ? 0 : 1;
  return 0;
}

int
Rts2DevCameraAlta::camChipInfo (int chip)
{
  return 0;
}

int
Rts2DevCameraAlta::camCoolMax ()
{
  alta->write_CoolerEnable (true);
  alta->write_FanMode (Apn_FanMode_High);
  alta->write_CoolerSetPoint (-100);
  return 0;
}

int
Rts2DevCameraAlta::camCoolHold ()
{
  alta->write_CoolerEnable (true);
  alta->write_FanMode (Apn_FanMode_High);
  return 0;
}

int
Rts2DevCameraAlta::camCoolTemp (float new_temp)
{
  alta->write_CoolerEnable (true);
  alta->write_FanMode (Apn_FanMode_High);
  alta->write_CoolerSetPoint (new_temp);
  return 0;
}

int
Rts2DevCameraAlta::camCoolShutdown ()
{
  alta->write_CoolerEnable (false);
  alta->write_FanMode (Apn_FanMode_Low);
  alta->write_CoolerSetPoint (100);
  return 0;
}

Rts2DevCameraAlta *device;

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
  int ret;

  device = new Rts2DevCameraAlta (argc, argv);

  signal (SIGINT, killSignal);
  signal (SIGTERM, killSignal);

  ret = device->init ();
  if (ret)
    {
      syslog (LOG_ERR, "Cannot initialize apogee alta camera - exiting!");
      exit (1);
    }
  device->run ();
  delete device;
}
