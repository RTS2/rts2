/*!
 * Driver for Apogee camera.
 *
 * Based on original apogee driver.
 *
 * @author petr
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif /* !_GNU_SOURCE */

#include <math.h>
#include <mcheck.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>

#include "apogee/CameraIO_Linux.h"

#include "camera_cpp.h"
#include "apogee/apcnst.h"

class CameraApogeeChip:public CameraChip
{
  CCameraIO *camera;
  unsigned short *dest;
  unsigned short *dest_top;
  char *send_top;
public:
    CameraApogeeChip (CCameraIO * in_camera);
    virtual ~ CameraApogeeChip (void);
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

CameraApogeeChip::CameraApogeeChip (CCameraIO * in_camera):CameraChip (0)
{
  camera = in_camera;
  dest = NULL;
}

CameraApogeeChip::~CameraApogeeChip (void)
{
  delete dest;
}

int
CameraApogeeChip::init ()
{
  setSize (camera->m_ImgColumns, camera->m_ImgRows, 0, 0);
  binningHorizontal = camera->m_BinX;
  binningVertical = camera->m_BinY;
  pixelX = camera->m_PixelXSize;
  pixelY = camera->m_PixelYSize;
  gain = camera->m_Gain;

  dest = new unsigned short[chipSize->width * chipSize->height];
}

int
CameraApogeeChip::setBinning (int in_vert, int in_hori)
{
  camera->m_BinX = in_hori;
  camera->m_BinY = in_vert;
  return 0;
}

int
CameraApogeeChip::startExposure (int light, float exptime)
{
  bool ret;
  Camera_Status status;
  ret = camera->Expose (exptime, light);

  if (!ret)
    {
      syslog (LOG_ERR, "CameraApogeeChip::startExposure exposure error");
      return -1;
    }

  chipUsedReadout = new ChipSubset (chipReadout);
  usedBinningVertical = binningVertical;
  usedBinningHorizontal = binningHorizontal;
  return 0;
}

long
CameraApogeeChip::isExposing ()
{
  long ret;
  Camera_Status status;
  ret = CameraChip::isExposing ();
  if (ret > 0)
    return ret;

  status = camera->read_Status ();

  if (status != Camera_Status_ImageReady)
    return 2000000;
  // exposure has ended.. 
  return -2;
}

int
CameraApogeeChip::endExposure ()
{
  camera->m_WaitingforTrigger = false;
  return CameraChip::endExposure ();
}

int
CameraApogeeChip::stopExposure ()
{
  camera->Reset ();
  return CameraChip::stopExposure ();
}

int
CameraApogeeChip::startReadout (Rts2DevConnData * dataConn, Rts2Conn * conn)
{
  int ret;
  ret = CameraChip::startReadout (dataConn, conn);
  dest_top = dest;
  send_top = (char *) dest;
  return ret;
}

int
CameraApogeeChip::readoutOneLine ()
{
  if (readoutLine < 0)
    return -1;

  if (readoutLine <
      (chipUsedReadout->y + chipUsedReadout->height) / usedBinningVertical)
    {
      if (readoutLine < chipUsedReadout->y)
	{
	  camera->Flush (chipUsedReadout->y);
	  readoutLine = chipUsedReadout->y;
	}
      Camera_Status status;
      camera->DigitizeLine ();
      do
	{
	  status = camera->read_Status ();
	}
      while (status == Camera_Status_Downloading);
      readoutLine++;
      short length = chipUsedReadout->width / usedBinningHorizontal;
      camera->GetLine (dest_top, length);
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

int
CameraApogeeChip::endReadout ()
{
  camera->Reset ();
  return CameraChip::endReadout ();
}

class Rts2DevCameraApogee:public Rts2DevCamera
{
  CCameraIO *camera;
  int device_id;
public:
    Rts2DevCameraApogee (int argc, char **argv);
    virtual ~ Rts2DevCameraApogee (void);
  virtual int processOption (int in_opt);
  virtual int init ();

  virtual int ready ();
  virtual int baseInfo ();
  virtual int info ();

  virtual int camChipInfo (int chip);

  virtual int camCoolMax ();
  virtual int camCoolHold ();
  virtual int camCoolTemp (float new_temp);
  virtual int camCoolShutdown ();
  virtual int camFilter (int new_filter);
};

Rts2DevCameraApogee::Rts2DevCameraApogee (int argc, char **argv):
Rts2DevCamera (argc, argv)
{
  addOption ('n', "device_id", 1,
	     "device ID (ussualy 0, which is also default)");
  device_id = 0;

  camera = new CCameraIO ();
  camera->m_Rows = AP_Rows;
  camera->m_Columns = AP_Columns;
  camera->m_ImgRows = AP_ImgRows;
  camera->m_ImgColumns = AP_ImgCols;
  camera->m_BIC = AP_BIC;
  camera->m_BIR = AP_BIR;
  camera->m_SkipC = AP_SkipC;
  camera->m_SkipR = AP_SkipR;
  camera->m_HFlush = AP_HFlush;
  camera->m_VFlush = AP_VFlush;

  // temperature constants
  camera->m_TempControl = AP_Control;
  camera->m_TempCalibration = AP_Cal;
  camera->m_TempScale = AP_Scale;

  fan = 0;
  canDF = 1;
}

Rts2DevCameraApogee::~Rts2DevCameraApogee (void)
{
  delete camera;
}

int
Rts2DevCameraApogee::processOption (int in_opt)
{
  switch (in_opt)
    {
    case 'n':
      device_id = atoi (optarg);
      break;
    default:
      return Rts2DevCamera::processOption (in_opt);
    }
  return 0;
}

int
Rts2DevCameraApogee::init ()
{
  int ret;
  ret = Rts2DevCamera::init ();
  if (ret)
    return ret;

  Camera_Status status;

  if (!camera->InitDriver (device_id))
    return -1;

  camera->Reset ();

  camera->write_LongCable (AP_LongCable);
  camera->m_DataBits = AP_Data_Bits;

  if (!strcmp (AP_CCD_Sensor, "CCD"))
    camera->m_SensorType = Camera_SensorType_CCD;
  else
    camera->m_SensorType = Camera_SensorType_CMOS;

  if (!camera->read_Present ())
    return -1;

  status = camera->read_Status ();

  if (status < 0)
    return -1;

  chipNum = 1;
  chips[0] = new CameraApogeeChip (camera);

  return Rts2DevCamera::initChips ();
}

int
Rts2DevCameraApogee::ready ()
{
  return camera->read_Present ();
}

int
Rts2DevCameraApogee::baseInfo ()
{
  strcpy (ccdType, "Apogee ");
  strncat (ccdType, camera->m_Sensor, 10);
  strcpy (serialNumber, "007");
}

int
Rts2DevCameraApogee::info ()
{

  tempRegulation = camera->read_CoolerMode ();
  tempSet = camera->read_CoolerSetPoint ();
  tempCCD = camera->read_Temperature ();
  tempAir = nan ("f");
  coolingPower = 5000;
  filter = camera->m_FilterPosition;
  return 0;
}

int
Rts2DevCameraApogee::camChipInfo (int chip)
{
  return 0;
}

int
Rts2DevCameraApogee::camCoolMax ()
{
  camera->write_CoolerSetPoint (-100);
  camera->write_CoolerMode (Camera_CoolerMode_On);
  return 0;
}

int
Rts2DevCameraApogee::camCoolHold ()
{
  camera->write_CoolerSetPoint (-20);
  camera->write_CoolerMode (Camera_CoolerMode_On);
  return 0;
}

int
Rts2DevCameraApogee::camCoolShutdown ()
{
  camera->write_CoolerMode (Camera_CoolerMode_Shutdown);
  return 0;
}

int
Rts2DevCameraApogee::camCoolTemp (float new_temp)
{
  camera->write_CoolerSetPoint (new_temp);
  camera->write_CoolerMode (Camera_CoolerMode_On);
  return 0;
}

int
Rts2DevCameraApogee::camFilter (int new_filter)
{
  camera->FilterSet (new_filter);
  filter = new_filter;
  return 0;
}

int
main (int argc, char **argv)
{
  mtrace ();

  Rts2DevCameraApogee *device = new Rts2DevCameraApogee (argc, argv);

  int ret;
  ret = device->init ();
  if (ret)
    {
      syslog (LOG_ERR, "Cannot initialize apogee camera - exiting!\n");
      exit (1);
    }
  device->run ();
  delete device;
}
