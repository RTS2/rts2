/*!
 * Driver for Apogee Alta cameras.
 *
 * Based on test_alta.cpp from Alta source.
 *
 * @author petr
 */
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
    CameraChipAlta (Rts2DevCamera * in_camer, CApnCamera * in_alta);
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

CameraChipAlta::CameraChipAlta (Rts2DevCamera * in_cam, CApnCamera * in_alta):CameraChip (in_cam,
	    0)
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
  alta->write_RoiBinningH (in_hori);
  alta->write_RoiBinningV (in_vert);
  return CameraChip::setBinning (in_vert, in_hori);
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

  if (status < 0)
    return -2;
  if (status != Apn_Status_ImageReady)
    return 200;
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
  alta->write_RoiPixelsV (chipUsedReadout->height);
  alta->write_RoiStartY (chipUsedReadout->y);
  alta->m_RoiStartX = chipUsedReadout->x;
  alta->m_RoiPixelsH = chipUsedReadout->width;
  dest_top = dest;
  send_top = (char *) dest;
  return ret;
}

int
CameraChipAlta::readoutOneLine ()
{
  int ret;

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
	return -1;
      dest_top += width * height;
      readoutLine = chipUsedReadout->y + chipUsedReadout->height;
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
  int bit12;
public:
    Rts2DevCameraAlta::Rts2DevCameraAlta (int argc, char **argv);
    virtual ~ Rts2DevCameraAlta (void);
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

Rts2DevCameraAlta::Rts2DevCameraAlta (int in_argc, char **in_argv):
Rts2DevCamera (in_argc, in_argv)
{
  alta = NULL;
  addOption ('B', "12bits", 0,
	     "switch to 12 bit readout mode; see alta specs for details");
  bit12 = 0;
}

Rts2DevCameraAlta::~Rts2DevCameraAlta (void)
{
  if (alta)
    {
      alta->CloseDriver ();
      delete alta;
    }
}

int
Rts2DevCameraAlta::processOption (int in_opt)
{
  switch (in_opt)
    {
    case 'B':
      bit12 = 1;
      break;
    default:
      return Rts2DevCamera::processOption (in_opt);
    }
  return 0;
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
    {
      alta->ResetSystem ();
      alta->CloseDriver ();
      sleep (2);
      ret = alta->InitDriver (0, 0, 0);
      if (!ret)
	return -1;
    }

  // Do a system reset to ensure known state, flushing enabled etc
  ret = alta->ResetSystem ();

  if (!ret)
    return -1;

  // set data bits..
  if (bit12)
    {
      alta->write_DataBits (Apn_Resolution_TwelveBit);
    }
  else
    {
      alta->write_DataBits (Apn_Resolution_SixteenBit);
    }

  chipNum = 1;
  chips[0] = new CameraChipAlta (this, alta);

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
  strcpy (ccdType, "Alta_");
  strncat (ccdType, alta->m_ApnSensorInfo->m_Sensor, 10);
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
  return Rts2DevCamera::info ();
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
  alta->write_CoolerSetPoint (-60);
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
      std::cerr << "Cannot initialize apogee alta camera - exiting!" << std::
	endl;
      exit (1);
    }
  device->run ();
  delete device;
}
