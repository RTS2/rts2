/*!
 * Alternative driver for SBIG camera.
 *
 * RTS2 urvc2 interface.
 * 
 * @author petr
 */
#include <ctype.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <fcntl.h>
#include <string.h>
#include <sys/io.h>

#include "urvc2/urvc.h"
#include "camd.h"

#define DEFAULT_CAMERA	ST8_CAMERA	// in case geteeprom fails

class CameraUrvc2Chip:public CameraChip
{
  CAMERA *C;
  unsigned short *img;
  unsigned short *dest_top;
  char *send_top;
public:
    CameraUrvc2Chip (Rts2DevCamera * in_cam, int in_chip_id, int in_width,
		     int in_height, double in_pixelX, double in_pixelY);
    virtual ~ CameraUrvc2Chip ();

  virtual int setBinning (int in_vert, int in_hori);

  virtual int startExposure (int light, float exptime);
  virtual long isExposing ();
  virtual int startReadout (Rts2DevConnData * dataConn, Rts2Conn * conn);
  virtual int readoutOneLine ();
};

CameraUrvc2Chip::CameraUrvc2Chip (Rts2DevCamera * in_cam, int in_chip_id,
				  int in_width, int in_height,
				  double in_pixelX, double in_pixelY):
CameraChip (in_cam, in_chip_id, in_width, in_height, in_pixelX, in_pixelY)
{
  OpenCCD (in_chip_id, &C);
  img = new unsigned short int[C->horzImage * C->vertImage];
}

CameraUrvc2Chip::~CameraUrvc2Chip ()
{
  free (img);
  CloseCCD (C);
}

int
CameraUrvc2Chip::setBinning (int in_vert, int in_hori)
{
  if (in_vert < 1 || in_vert > 3 || in_hori < 1 || in_hori > 3)
    return -1;
  return CameraChip::setBinning (in_vert, in_hori);
};

int
CameraUrvc2Chip::startExposure (int light, float exptime)
{
  return (CCDExpose (C, (int) (100 * (exptime) + 0.5), light));
}

int
CameraUrvc2Chip::startReadout (Rts2DevConnData * dataConn, Rts2Conn * conn)
{
  dest_top = img;
  send_top = (char *) img;
  return CameraChip::startReadout (dataConn, conn);
}

long
CameraUrvc2Chip::isExposing ()
{
  int ret;
  ret = CameraChip::isExposing ();
  if (ret > 0)
    return ret;
  int imstate;
  imstate = CCDImagingState (C);
  if (imstate == -1)
    return -1;
  if (imstate)
    {
      return 100;		// recheck in 100 msec
    }
  return -2;
}

int
CameraUrvc2Chip::readoutOneLine ()
{
  if (readoutLine == 0)
    {
      if (CCDReadout
	  (img, C, chipReadout->x / binningVertical,
	   chipReadout->y / binningVertical,
	   chipReadout->width / binningVertical,
	   chipReadout->height / binningVertical, binningVertical))
	{
	  logStream (MESSAGE_DEBUG) <<
	    "urvc2 chip readoutOneLine readout return not-null" << sendLog;
	  return -1;
	}
      dest_top =
	img +
	((chipReadout->width / binningVertical) *
	 (chipReadout->height / binningVertical));
      readoutLine = 1;
      // save to file
      return 0;
    }
  if (sendLine == 0)
    {
      int ret;
      ret = CameraChip::sendFirstLine ();
      if (ret)
	return ret;
    }
  int send_data_size;
  sendLine++;
  send_data_size += sendReadoutData (send_top, (char *) dest_top - send_top);
  if (send_data_size < 0)
    return -1;
  send_top += send_data_size;
  if (send_top < (char *) dest_top)
    return 0;
  return -2;
}

class Rts2DevCameraUrvc2:public Rts2DevCamera
{
private:
  EEPROMContents eePtr;		// global to prevent multiple EEPROM calls
  CAMERA_TYPE cameraID;

  void get_eeprom ();
  void init_shutter ();
  int set_fan (int fan_state);
  int setcool (int reg, int setpt, int prel, int fan, int state);

  int ad_temp;
public:
    Rts2DevCameraUrvc2 (int argc, char **argv);

  virtual int init ();
  virtual int ready ();
  virtual int info ();
  virtual int camChipInfo (int chip)
  {
    return 0;
  }
  virtual int camExpose (int chip, int light, float exptime);

  virtual int camStopExpose (int chip);

  virtual int camCoolMax ();
  virtual int camCoolHold ();
  int camCoolTemp ()
  {
    return setcool (1, ad_temp, 0xaf, FAN_ON, CAMERA_COOL_HOLD);
  }
  virtual int camCoolTemp (float coolpoint);
  virtual int camCoolShutdown ();
  CAMERA_TYPE getCameraID (void)
  {
    return cameraID;
  }
};

/*!
 * Filter class for URVC2 based camera.
 * It needs it as there is now filter is an extra object.
 */

class Rts2FilterUrvc2:public Rts2Filter
{
  Rts2DevCameraUrvc2 *camera;
  int filter;
public:
    Rts2FilterUrvc2 (Rts2DevCameraUrvc2 * in_camera);
    virtual ~ Rts2FilterUrvc2 (void);
  virtual int init (void);
  virtual int getFilterNum (void);
  virtual int setFilterNum (int new_filter);
};

Rts2FilterUrvc2::Rts2FilterUrvc2 (Rts2DevCameraUrvc2 * in_camera):Rts2Filter ()
{
  camera = in_camera;
}

Rts2FilterUrvc2::~Rts2FilterUrvc2 (void)
{
  setFilterNum (0);
}

int
Rts2FilterUrvc2::init ()
{
  setFilterNum (0);
  return 0;
}

int
Rts2FilterUrvc2::getFilterNum (void)
{
  return filter;
}

int
Rts2FilterUrvc2::setFilterNum (int new_filter)
{
  PulseOutParams pop;

  if (new_filter < 0 || new_filter > 4)
    {
      return -1;
    }

  pop.pulsePeriod = 18270;
  pop.pulseWidth = 500 + 300 * new_filter;
  pop.numberPulses = 60;

  if (MicroCommand (MC_PULSE, camera->getCameraID (), &pop, NULL))
    return -1;
  filter = new_filter;

  return 0;
}

void
Rts2DevCameraUrvc2::get_eeprom ()
{
  if (tempRegulation->getValueInteger () == -1)
    {
      if (GetEEPROM (cameraID, &eePtr) != CE_NO_ERROR)
	{
	  eePtr.version = 0;
	  eePtr.model = DEFAULT_CAMERA;	// ST8 camera
	  eePtr.abgType = 0;
	  eePtr.badColumns = 0;
	  eePtr.trackingOffset = 0;
	  eePtr.trackingGain = 304;
	  eePtr.imagingOffset = 0;
	  eePtr.imagingGain = 560;
	  bzero (eePtr.columns, 4);
	  strcpy ((char *) eePtr.serialNumber, "EEEE");
	}
      else
	{
	  // check serial number
	  char *b = (char *) eePtr.serialNumber;
	  for (; *b; b++)
	    if (!isalnum (*b))
	      {
		*b = '0';
		break;
	      }
	}
    }
}

void
Rts2DevCameraUrvc2::init_shutter ()
{
  MiscellaneousControlParams ctrl;
  StatusResults sr;

  if (cameraID == ST237_CAMERA || cameraID == ST237A_CAMERA)
    return;

  if (MicroCommand (MC_STATUS, cameraID, NULL, &sr) == CE_NO_ERROR)
    {
      ctrl.fanEnable = sr.fanEnabled;
      ctrl.shutterCommand = 3;
      ctrl.ledState = sr.ledStatus;
      MicroCommand (MC_MISC_CONTROL, cameraID, &ctrl, NULL);
      sleep (2);
    }
}

int
Rts2DevCameraUrvc2::set_fan (int fan_state)
{
  MiscellaneousControlParams ctrl;
  StatusResults sr;

  ctrl.fanEnable = fan_state;
  ctrl.shutterCommand = 0;

  if ((MicroCommand (MC_STATUS, cameraID, NULL, &sr)))
    return -1;
  ctrl.ledState = sr.ledStatus;
  if (MicroCommand (MC_MISC_CONTROL, cameraID, &ctrl, NULL))
    return -1;

  return 0;
}

int
Rts2DevCameraUrvc2::setcool (int reg, int setpt, int prel, int in_fan,
			     int coolstate)
{
  MicroTemperatureRegulationParams cool;

  cool.regulation = reg;
  cool.ccdSetpoint = setpt;
  cool.preload = prel;

  set_fan (in_fan);

  if (MicroCommand (MC_REGULATE_TEMP, cameraID, &cool, NULL))
    return -1;
  tempRegulation->setValueInteger (coolstate);

  return 0;
}

Rts2DevCameraUrvc2::Rts2DevCameraUrvc2 (int in_argc, char **in_argv):Rts2DevCamera (in_argc,
	       in_argv)
{
  cameraID = DEFAULT_CAMERA;
  tempRegulation->setValueInteger (-1);
}

int
Rts2DevCameraUrvc2::init ()
{
  short base;
  int i;
  //StatusResults sr;
  QueryTemperatureStatusResults qtsr;
  GetVersionResults gvr;
  int ret;

  ret = Rts2DevCamera::init ();
  if (ret)
    return ret;

  base = getbaseaddr (0);	// sbig_port...?

  // This is a standard driver init procedure 
  begin_realtime ();
  CameraInit (base);

  logStream (MESSAGE_DEBUG) << "urvc2 init" << sendLog;

  if ((i = MicroCommand (MC_GET_VERSION, ST7_CAMERA, NULL, &gvr)))
    {
      logStream (MESSAGE_DEBUG) << "urvc2 GET_VERSION ret " << i << sendLog;
      return -1;
    }

  cameraID = (CAMERA_TYPE) gvr.cameraID;

  if ((i = MicroCommand (MC_TEMP_STATUS, cameraID, NULL, &qtsr)))
    {
      logStream (MESSAGE_DEBUG) << "urvc2 TEMP_STATUS ret " << i << sendLog;
      return -1;
    }

  logStream (MESSAGE_DEBUG) << "urvc2 get eeprom" << sendLog;

  get_eeprom ();

  if (Cams[eePtr.model].hasTrack)
    chipNum = 2;
  else
    chipNum = 1;

  for (i = 0; i < chipNum; i++)
    {
      logStream (MESSAGE_DEBUG) << "urvc2 new CameraUrvc2Chip " << i <<
	sendLog;
      chips[i] =
	new CameraUrvc2Chip (this, i, Cams[eePtr.model].horzImage,
			     Cams[eePtr.model].vertImage,
			     Cams[eePtr.model].pixelX,
			     Cams[eePtr.model].pixelY);
    }

  // determine temperature regulation state
  switch (qtsr.enabled)
    {
    case 0:
      // a bit strange: may have different
      // meanings... (freeze)
      tempRegulation->
	setValueInteger ((qtsr.power >
			  250) ? CAMERA_COOL_MAX : CAMERA_COOL_OFF);
      break;
    case 1:
      tempRegulation->setValueInteger (CAMERA_COOL_HOLD);
      break;
    default:
      tempRegulation->setValueInteger (CAMERA_COOL_OFF);
    }

  logStream (MESSAGE_DEBUG) << "urvc2 init return " << Cams[eePtr.model].
    horzImage << sendLog;

  filter = new Rts2FilterUrvc2 (this);

  strcpy (ccdType, (char *) Cams[eePtr.model].fullName);
  strcpy (serialNumber, (char *) eePtr.serialNumber);

  return Rts2DevCameraUrvc2::initChips ();
}

int
Rts2DevCameraUrvc2::ready ()
{
  StatusResults gvr;
  if (MicroCommand (MC_STATUS, cameraID, NULL, &gvr))
    return -1;
  return 0;
}

int
Rts2DevCameraUrvc2::info ()
{
  StatusResults gvr;
  QueryTemperatureStatusResults qtsr;

  if (MicroCommand (MC_STATUS, cameraID, NULL, &gvr))
    return -1;
  if (MicroCommand (MC_TEMP_STATUS, cameraID, NULL, &qtsr))
    return -1;

  tempSet->setValueDouble (ccd_ad2c (qtsr.ccdSetpoint));
  coolingPower->
    setValueInteger ((int) ((qtsr.power == 255) ? 1000 : qtsr.power * 3.906));
  tempAir->setValueDouble (ambient_ad2c (qtsr.ambientThermistor));
  tempCCD->setValueDouble (ccd_ad2c (qtsr.ccdThermistor));
  fan->setValueInteger (gvr.fanEnabled);
  canDF->setValueInteger (1);
  return Rts2DevCamera::info ();
}

int
Rts2DevCameraUrvc2::camExpose (int chip_id, int light, float exptime)
{
#ifdef INIT_SHUTTER
  init_shutter ();
#else
  if (!light)			// init shutter only for dark images
    init_shutter ();
#endif
  return Rts2DevCamera::camExpose (chip_id, light, exptime);
}

int
Rts2DevCameraUrvc2::camStopExpose (int chip)
{
  EndExposureParams eep;
  eep.ccd = chip;

  if (MicroCommand (MC_END_EXPOSURE, cameraID, &eep, NULL))
    return -1;

  return 0;
};

int
Rts2DevCameraUrvc2::camCoolMax ()	/* try to max temperature */
{
  return setcool (2, 255, 0, FAN_ON, CAMERA_COOL_MAX);
}

int
Rts2DevCameraUrvc2::camCoolHold ()	/* hold on that temperature */
{
  QueryTemperatureStatusResults qtsr;
  float ot;			// optimal temperature

  if (tempRegulation->getValueInteger () == CAMERA_COOL_HOLD)
    return 0;			// already cooled

  if (isnan (nightCoolTemp))
    {
      if (MicroCommand (MC_TEMP_STATUS, cameraID, NULL, &qtsr))
	return -1;

      ot = ccd_ad2c (qtsr.ccdThermistor);
      ot = ((int) (ot + 5) / 5) * 5;
    }
  else
    {
      ot = nightCoolTemp;
    }

  set_fan (1);

  return camCoolTemp (ot);
}

int
Rts2DevCameraUrvc2::camCoolTemp (float coolpoint)	/* set direct setpoint */
{
  ad_temp = ccd_c2ad (coolpoint) + 0x7;	// zaokrohlovat a neorezavat!
  return camCoolTemp ();
}

int
Rts2DevCameraUrvc2::camCoolShutdown ()	/* ramp to ambient */
{
  return setcool (0, 0, 0, FAN_OFF, CAMERA_COOL_OFF);
}

int
main (int argc, char **argv)
{
  Rts2DevCameraUrvc2 device = Rts2DevCameraUrvc2 (argc, argv);
  return device.run ();
}
