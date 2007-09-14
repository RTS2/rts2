#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>

#include "camd.h"
#include "rts2devcliwheel.h"
#include "rts2devclifocuser.h"

void
CameraChip::initData (Rts2DevCamera * in_cam, int in_chip_id)
{
  camera = in_cam;
  chipId = in_chip_id;
  exposureEnd.tv_sec = 0;
  exposureEnd.tv_usec = 0;
  readoutConn = NULL;
  binningVertical = 1;
  binningHorizontal = 1;
  chipSize = NULL;
  chipReadout = NULL;
  chipUsedReadout = NULL;
  pixelX = nan ("f");
  pixelY = nan ("f");
  readoutLine = -1;
  sendLine = -1;
  shutter_state = -1;

  subExposure = nan ("f");
  nAcc = 1;

  focusingData = NULL;
  focusingDataTop = NULL;

  usedDataType = RTS2_DATA_USHORT;
}

CameraChip::CameraChip (Rts2DevCamera * in_cam, int in_chip_id)
{
  initData (in_cam, in_chip_id);
}

CameraChip::CameraChip (Rts2DevCamera * in_cam, int in_chip_id, int in_width,
			int in_height, double in_pixelX, double in_pixelY)
{
  initData (in_cam, in_chip_id);
  setSize (in_width, in_height, 0, 0);
  pixelX = in_pixelX;
  pixelY = in_pixelY;
}

CameraChip::~CameraChip (void)
{
  delete chipSize;
  delete chipReadout;

  delete focusingData;
}

int
CameraChip::init ()
{
  return 0;
}

int
CameraChip::setBinning (int in_vert, int in_hori)
{
  binningVertical = in_vert;
  binningHorizontal = in_hori;
  return 0;
}

int
CameraChip::box (int in_x, int in_y, int in_width, int in_height)
{
  // tests for -1 -> full size
  if (in_x == -1)
    in_x = chipSize->x;
  if (in_y == -1)
    in_y = chipSize->y;
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
CameraChip::center (int in_w, int in_h)
{
  int x, y, w, h;
  if (in_w > 0 && chipSize->width >= in_w)
    {
      w = in_w;
      x = chipSize->width / 2 - w / 2;
    }
  else
    {
      w = chipSize->width / 2;
      x = chipSize->width / 4;
    }
  if (in_h > 0 && chipSize->height >= in_h)
    {
      h = in_h;
      y = chipSize->height / 2 - h / 2;
    }
  else
    {
      h = chipSize->height / 2;
      y = chipSize->height / 4;
    }
  return box (x, y, w, h);
}

int
CameraChip::startExposure (int light, float exptime)
{
  return -1;
}

int
CameraChip::setExposure (float exptime, int in_shutter_state)
{
  struct timeval tv;
  gettimeofday (&tv, NULL);

  long int f_exptime = (long int) floor (exptime);
  exposureEnd.tv_sec = tv.tv_sec + f_exptime;
  exposureEnd.tv_usec =
    tv.tv_usec + (long int) ((exptime - f_exptime) * USEC_SEC);
  if (tv.tv_usec > USEC_SEC)
    {
      exposureEnd.tv_sec += tv.tv_usec / USEC_SEC;
      exposureEnd.tv_usec = tv.tv_usec % USEC_SEC;
    }
  shutter_state = in_shutter_state;
  return 0;
}

/**
 * Check if exposure has ended.
 *
 * @return 0 if there was pending exposure which ends, -1 if there wasn't any exposure, > 0 time remainnign till end of exposure
 */
long
CameraChip::isExposing ()
{
  struct timeval tv;
  if (exposureEnd.tv_sec == 0 && exposureEnd.tv_usec == 0)
    return -1;			// no exposure running
  gettimeofday (&tv, NULL);
  if (tv.tv_sec > exposureEnd.tv_sec
      || (tv.tv_sec == exposureEnd.tv_sec
	  && tv.tv_usec >= exposureEnd.tv_usec))
    {
      return 0;			// exposure ended
    }
  return (exposureEnd.tv_sec - tv.tv_sec) * USEC_SEC + (exposureEnd.tv_usec - tv.tv_usec);	// timeout
}

int
CameraChip::endExposure ()
{
  int ret;
  exposureEnd.tv_sec = 0;
  exposureEnd.tv_usec = 0;
  if (camera->isFocusing ())
    {
      // fake readout - only to memory
      ret = camera->camReadout (chipId);
      if (ret)
	camera->endFocusing ();
    }
  return 0;
}

int
CameraChip::stopExposure ()
{
  return endExposure ();
}

int
CameraChip::sendChip (Rts2Conn * conn, char *name, int value)
{
  char *msg;
  int ret;

  asprintf (&msg, "chip %i %s %i", chipId, name, value);
  ret = conn->send (msg);
  free (msg);
  return ret;
}

int
CameraChip::sendChip (Rts2Conn * conn, char *name, float value)
{
  char *msg;
  int ret;

  asprintf (&msg, "chip %i %s %0.2f", chipId, name, value);
  ret = conn->send (msg);
  free (msg);
  return ret;
}

int
CameraChip::sendChip (Rts2Conn * conn, char *name, double value)
{
  char *msg;
  int ret;

  asprintf (&msg, "chip %i %s %lf", chipId, name, value);
  ret = conn->send (msg);
  free (msg);
  return ret;
}

int
CameraChip::send (Rts2Conn * conn)
{
  sendChip (conn, "width", chipSize->width);
  sendChip (conn, "height", chipSize->height);
  sendChip (conn, "binning_vertical", binningVertical);
  sendChip (conn, "binning_horizontal", binningHorizontal);
  sendChip (conn, "pixelX", pixelX);
  sendChip (conn, "pixelY", pixelY);
  return 0;
}

int
CameraChip::sendReadoutData (char *data, size_t data_size)
{
  int ret;
  time_t now;
  if (!readoutConn)
    {
      if (camera->isFocusing ())
	{
	  return processData (data, data_size);
	}
      // completly strange then..
      return -1;
    }
  ret = readoutConn->send (data, data_size);
  if (ret == -2)
    {
      time (&now);
      if (now > readout_started + readoutConn->getConnTimeout ())
	{
	  logStream (MESSAGE_ERROR) <<
	    "Chip sendReadoutData connection not established within timeout"
	    << sendLog;
	  return -1;
	}
    }
  if (ret == -1)
    {
      logStream (MESSAGE_ERROR) << "Chip sendReadoutData " << strerror (errno)
	<< sendLog;
    }
  return ret;
}

int
CameraChip::processData (char *data, size_t size)
{
  memcpy (focusingDataTop, data, size);
  focusingDataTop += size;
  return size;
}

int
CameraChip::doFocusing ()
{
  // dummy method to write data to see it's working as expected
  int f = open ("/tmp/focusing.dat", O_CREAT | O_TRUNC | O_WRONLY);
  write (f, focusingData,
	 focusingHeader.sizes[0] * focusingHeader.sizes[1] * 2);
  close (f);
  return 0;
}

int
CameraChip::startReadout (Rts2DevConnData * dataConn, Rts2Conn * conn)
{
  char *msg;
  char address[200];
  if (!chipUsedReadout)
    {
      chipUsedReadout = new ChipSubset (chipReadout);
      usedBinningVertical = binningVertical;
      usedBinningHorizontal = binningHorizontal;
    }
  setReadoutConn (dataConn);
  if (dataConn && conn)
    {
      dataConn->getAddress ((char *) &address, 200);

      asprintf (&msg, "D connect %i %s %i %i", chipId, address,
		dataConn->getLocalPort (),
		chipUsedSize () * usedPixelByteSize () + sizeof (imghdr));
      conn->send (msg);
      free (msg);
    }
  return 0;
}

void
CameraChip::setReadoutConn (Rts2DevConnData * dataConn)
{
  readoutConn = dataConn;
  readoutLine = 0;
  sendLine = 0;
  time (&readout_started);
}

void
CameraChip::deleteConnection (Rts2Conn * conn)
{
  if (conn == readoutConn)
    {
      readoutConn = NULL;
    }
}

int
CameraChip::endReadout ()
{
  if (camera->isFocusing ())
    {
      int ret;
      ret = doFocusing ();
      if (ret)
	{
	  delete[]focusingData;
	  focusingData = NULL;
	  focusingDataTop = NULL;
	  camera->endFocusing ();
	}
    }
  clearReadout ();
  if (readoutConn)
    {
      readoutConn->endConnection ();
      readoutConn = NULL;
    }
  delete chipUsedReadout;
  chipUsedReadout = NULL;
  return 0;
}

void
CameraChip::clearReadout ()
{
  readoutLine = -1;
  sendLine = -1;
}

int
CameraChip::sendFirstLine ()
{
  int w, h;
  w = chipUsedReadout->width / usedBinningHorizontal;
  h = chipUsedReadout->height / usedBinningVertical;
  if (camera->isFocusing ())
    {
      // alocate data..
      if (!focusingData || focusingHeader.sizes[0] < w
	  || focusingHeader.sizes[1] < h)
	{
	  delete[]focusingData;
	  focusingData = new char[w * h * 2];
	}
      focusingDataTop = focusingData;
    }
  focusingHeader.data_type = usedDataType;
  focusingHeader.naxes = 2;
  focusingHeader.sizes[0] = chipUsedReadout->width / usedBinningHorizontal;
  focusingHeader.sizes[1] = chipUsedReadout->height / usedBinningVertical;
  focusingHeader.binnings[0] = usedBinningHorizontal;
  focusingHeader.binnings[1] = usedBinningVertical;
  focusingHeader.x = chipUsedReadout->x;
  focusingHeader.y = chipUsedReadout->y;
  focusingHeader.filter = camera->getLastFilterNum ();
  focusingHeader.shutter = shutter_state;
  focusingHeader.subexp = subExposure;
  focusingHeader.nacc = nAcc;
  if (readoutConn)
    {
      int ret;
      ret = sendReadoutData ((char *) &focusingHeader, sizeof (imghdr));
      if (ret == -2)
	return 100;		// not yet connected, wait for connection..
      if (ret > 0)		// data send sucessfully
	return 0;
      return ret;		// can be -1 as well
    }
  else if (camera->isFocusing ())
    return 0;
  return -1;
}

int
CameraChip::readoutOneLine ()
{
  return -1;
}

void
CameraChip::cancelPriorityOperations ()
{
  stopExposure ();
  if (camera->isFocusing ())
    camera->endFocusing ();
  endReadout ();
  chipUsedReadout = NULL;
  delete[]focusingData;
  focusingData = NULL;
  focusingDataTop = NULL;
  subExposure = nan ("f");
  nAcc = 1;
  box (-1, -1, -1, -1);
}

bool
CameraChip::supportFrameTransfer ()
{
  return false;
}

int
Rts2DevCamera::setSubExposure (double in_subexposure)
{
  subExposure->setValueDouble (in_subexposure);
  return 0;
}

Rts2DevCamera::Rts2DevCamera (int in_argc, char **in_argv):
Rts2ScriptDevice (in_argc, in_argv, DEVICE_TYPE_CCD, "C0")
{
  int i;
  for (i = 0; i < MAX_CHIPS; i++)
    chips[i] = NULL;
  tempAir = NULL;
  tempCCD = NULL;
  tempSet = NULL;
  tempRegulation = NULL;
  coolingPower = NULL;
  fan = NULL;
  filter = NULL;
  ccdType[0] = '\0';
  ccdRealType = ccdType;
  serialNumber[0] = '\0';

  createValue (lastExp, "exposure", "current exposure time", false);
  createValue (subExposure, "subexposure", "current subexposure", false, 0,
	       CAM_EXPOSING | CAM_READING | CAM_DATA, true);
  createValue (camFilterVal, "filter", "used filter number", false, 0,
	       CAM_EXPOSING, false);

  createValue (camFocVal, "focpos", "position of focuser", false, 0,
	       CAM_EXPOSING, true);

  createValue (camShutterVal, "shutter", "shutter position", false);

  defaultSubExposure = nan ("f");

  nightCoolTemp = nan ("f");
  focuserDevice = NULL;
  wheelDevice = NULL;

  createValue (nextExp, "next_exp",
	       "state of next exposure (0 when next exposure is possible",
	       false);
  nextExp->setValueInteger (NOT_EXP);

  nextExpChip = -1;
  nextExpLight = 1;
  defBinning = 1;
  defFocusExposure = 10;

  createValue (rnoise, "RNOISE", "CCD readout noise");

  // cooling & other options..
  addOption ('c', "cooling_temp", 1, "default night cooling temperature");
  addOption ('F', "focuser", 1,
	     "name of focuser device, which will be granted to do exposures without priority");
  addOption ('W', "filterwheel", 1,
	     "name of device which is used as filter wheel");
  addOption ('b', "default_bin", 1, "default binning (ussualy 1)");
  addOption ('e', "focexp", 1, "starting focusing exposure duration");
  addOption ('s', "subexposure", 1, "default subexposure");
  addOption ('t', "type", 1,
	     "specify camera type (in case camera do not store it in FLASH ROM)");
}

Rts2DevCamera::~Rts2DevCamera ()
{
  int i;
  for (i = 0; i < chipNum; i++)
    {
      delete chips[i];
      chips[i] = NULL;
    }
  delete filter;
}

int
Rts2DevCamera::willConnect (Rts2Address * in_addr)
{
  if (wheelDevice && in_addr->getType () == DEVICE_TYPE_FW
      && in_addr->isAddress (wheelDevice))
    return 1;
  if (focuserDevice && in_addr->getType () == DEVICE_TYPE_FOCUS
      && in_addr->isAddress (focuserDevice))
    return 1;
  return Rts2ScriptDevice::willConnect (in_addr);
}

Rts2DevClient *
Rts2DevCamera::createOtherType (Rts2Conn * conn, int other_device_type)
{
  switch (other_device_type)
    {
    case DEVICE_TYPE_FW:
      return new Rts2DevClientFilterCamera (conn);
    case DEVICE_TYPE_FOCUS:
      return new Rts2DevClientFocusCamera (conn);
    }
  return Rts2ScriptDevice::createOtherType (conn, other_device_type);
}

void
Rts2DevCamera::cancelPriorityOperations ()
{
  int i;
  for (i = 0; i < chipNum; i++)
    {
      chips[i]->cancelPriorityOperations ();
      maskStateChip (i, CAM_MASK_EXPOSE | CAM_MASK_DATA | CAM_MASK_READING,
		     CAM_NOEXPOSURE | CAM_NODATA | CAM_NOTREADING,
		     BOP_TEL_MOVE, 0, "chip exposure interrupted");
      chips[i]->setBinning (defBinning, defBinning);
    }
  nextExp->setValueInteger (NOT_EXP);
  nextExpChip = -1;
  setTimeout (USEC_SEC);
  // init states etc..
  clearStatesPriority ();
  setSubExposure (defaultSubExposure);
  Rts2ScriptDevice::cancelPriorityOperations ();
}

int
Rts2DevCamera::info ()
{
  camFilterVal->setValueInteger (getFilterNum ());
  camFocVal->setValueInteger (getFocPos ());
  return Rts2ScriptDevice::info ();
}

int
Rts2DevCamera::scriptEnds ()
{
  int i;
  for (i = 0; i < chipNum; i++)
    {
      chips[i]->box (-1, -1, -1, -1);
      chips[i]->setBinning (defBinning, defBinning);
    }
  setTimeout (USEC_SEC);
  return Rts2ScriptDevice::scriptEnds ();
}

int
Rts2DevCamera::processOption (int in_opt)
{
  switch (in_opt)
    {
    case 'c':
      nightCoolTemp = atof (optarg);
      break;
    case 'F':
      focuserDevice = optarg;
      break;
    case 'W':
      wheelDevice = optarg;
      break;
    case 'b':
      defBinning = atoi (optarg);
      break;
    case 'e':
      defFocusExposure = atof (optarg);
      break;
    case 's':
      defaultSubExposure = atof (optarg);
      setSubExposure (defaultSubExposure);
      break;
    case 't':
      ccdRealType = optarg;
      break;
    default:
      return Rts2ScriptDevice::processOption (in_opt);
    }
  return 0;
}

int
Rts2DevCamera::initChips ()
{
  int ret;
  for (int i = 0; i < chipNum; i++)
    {
      ret = chips[i]->init ();
      if (ret)
	return ret;
      if (defBinning != 1)
	chips[i]->setBinning (defBinning, defBinning);
    }
  // init filter
  if (filter)
    {
      ret = filter->init ();
      if (ret)
	{
	  return ret;
	}
    }

  return 0;
}

int
Rts2DevCamera::initValues ()
{
  // TODO init focuser - try to read focuser offsets & initial position from file
  addConstValue ("focuser", focuserDevice);
  addConstValue ("wheel", wheelDevice);

  addConstValue ("chips", chipNum);

  addConstValue ("CCD_TYPE", "camera type", ccdRealType);
  addConstValue ("CCD_SER", "camera serial number", serialNumber);

  return Rts2ScriptDevice::initValues ();
}

void
Rts2DevCamera::checkExposures ()
{
  long ret;
  for (int i = 0; i < chipNum; i++)
    {
      if (getStateChip (i) & CAM_EXPOSING)
	{
	  // try to end exposure
	  ret = camWaitExpose (i);
	  if (ret >= 0)
	    {
	      setTimeout (ret);
	    }
	  else
	    {
	      if (ret == -2)
		{
		  maskStateChip (i, CAM_MASK_EXPOSE | CAM_MASK_DATA,
				 CAM_NOEXPOSURE | CAM_DATA, BOP_TEL_MOVE, 0,
				 "exposure chip finished");
		  chips[i]->endExposure ();
		}
	      if (ret == -1)
		{
		  maskStateChip (i,
				 DEVICE_ERROR_MASK | CAM_MASK_EXPOSE |
				 CAM_MASK_DATA,
				 DEVICE_ERROR_HW | CAM_NOEXPOSURE |
				 CAM_NODATA, BOP_TEL_MOVE, 0,
				 "exposure chip finished with error");
		  chips[i]->stopExposure ();
		}
	    }
	}
    }
}

void
Rts2DevCamera::checkReadouts ()
{
  int ret;
  for (int i = 0; i < chipNum; i++)
    {
      if ((getStateChip (i) & CAM_MASK_READING) != CAM_READING)
	continue;
      ret = chips[i]->readoutOneLine ();
      if (ret >= 0)
	{
	  setTimeout (ret);
	}
      else
	{
	  chips[i]->endReadout ();
	  afterReadout ();
	  if (ret == -2)
	    maskStateChip (i, CAM_MASK_READING, CAM_NOTREADING, BOP_TEL_MOVE,
			   0, "chip readout ended");
	  else
	    maskStateChip (i, DEVICE_ERROR_MASK | CAM_MASK_READING,
			   DEVICE_ERROR_HW | CAM_NOTREADING, BOP_TEL_MOVE, 0,
			   "chip readout ended with error");
	}
    }
}

void
Rts2DevCamera::afterReadout ()
{
  if (nextExp->getValueInteger () == FT_EXP)
    {
      camStartExposure (nextExpChip, nextExpLight, nextExpTime);
      nextExp->setValueInteger (NOT_EXP);
    }
  setTimeout (USEC_SEC);
}

int
Rts2DevCamera::setValue (Rts2Value * old_value, Rts2Value * new_value)
{
  if (old_value == camFocVal)
    {
      return setFocuser (new_value->getValueInteger ()) == 0 ? 0 : -2;
    }
  if (old_value == subExposure)
    {
      return setSubExposure (new_value->getValueDouble ()) == 0 ? 0 : -2;
    }
  if (old_value == camFilterVal)
    {
      return camFilter (new_value->getValueInteger ()) == 0 ? 0 : -2;
    }
  if (old_value == tempSet)
    {
      return camCoolTemp (new_value->getValueFloat ()) == 0 ? 0 : -2;
    }
  return Rts2ScriptDevice::setValue (old_value, new_value);
}

void
Rts2DevCamera::deviceReady (Rts2Conn * conn)
{
  // if that's filter wheel
  if (wheelDevice && !strcmp (conn->getName (), wheelDevice)
      && conn->getOtherDevClient ())
    {
      // copy content of device filter variable to our list..
      Rts2Value *val = conn->getValue ("filter");
      // it's filter and it's correct type
      if (val->getValueType () == RTS2_VALUE_SELECTION)
	camFilterVal->duplicateSelVals ((Rts2ValueSelection *) val);
    }
}

void
Rts2DevCamera::postEvent (Rts2Event * event)
{
  switch (event->getType ())
    {
    case EVENT_FILTER_MOVE_END:
    case EVENT_FOCUSER_END_MOVE:
      switch (event->getType ())
	{
	case EVENT_FILTER_MOVE_END:
	  nextExp->setValueInteger (nextExp->
				    getValueInteger () & ~FILTER_MOVE);
	  break;
	case EVENT_FOCUSER_END_MOVE:
	  nextExp->setValueInteger (nextExp->
				    getValueInteger () & ~FOCUSER_MOVE);
	  break;
	}
      // update info about FW
      infoAll ();
      if (nextExp->getValueInteger () == NOT_EXP && nextExpChip >= 0)
	{
	  // start qued exposure
	  camStartExposure (nextExpChip, nextExpLight, nextExpTime);
	  nextExpChip = -1;
	}
      break;
    }
  Rts2ScriptDevice::postEvent (event);
}

int
Rts2DevCamera::idle ()
{
  int ret;
  checkExposures ();
  checkReadouts ();
  if (isIdle () && isFocusing ())
    {
      ret = camStartExposure (0, 1, focusExposure);
      if (ret)
	endFocusing ();
    }
  return Rts2ScriptDevice::idle ();
}

int
Rts2DevCamera::changeMasterState (int new_state)
{
  switch (new_state & (SERVERD_STATUS_MASK | SERVERD_STANDBY_MASK))
    {
    case SERVERD_DUSK | SERVERD_STANDBY:
    case SERVERD_NIGHT | SERVERD_STANDBY:
    case SERVERD_DAWN | SERVERD_STANDBY:
    case SERVERD_DUSK:
    case SERVERD_NIGHT:
    case SERVERD_DAWN:
      camCoolHold ();
      break;
    case SERVERD_EVENING | SERVERD_STANDBY:
    case SERVERD_EVENING:
      camCoolMax ();
      break;
    default:
      camCoolShutdown ();
    }
  return Rts2ScriptDevice::changeMasterState (new_state);
}

int
Rts2DevCamera::camChipInfo (Rts2Conn * conn, int chip)
{
  int ret;
  ret = camChipInfo (chip);
  if (ret)
    {
      conn->sendCommandEnd (DEVDEM_E_HW, "error during chipinfo call");
      return -1;
    }
  return chips[chip]->send (conn);
}

int
Rts2DevCamera::camStartExposure (int chip, int light, float exptime)
{
  int ret;

  ret = camExpose (chip, light, exptime);
  if (ret)
    return ret;

  lastExp->setValueFloat (exptime);
  infoAll ();
  maskStateChip (chip, CAM_MASK_EXPOSE | CAM_MASK_DATA,
		 CAM_EXPOSING | CAM_NODATA, BOP_TEL_MOVE, BOP_TEL_MOVE,
		 "exposure chip started");
  chips[chip]->setExposure (exptime,
			    light ? SHUTTER_SYNCHRO : SHUTTER_CLOSED);
  lastFilterNum = getFilterNum ();
  // call us to check for exposures..
  long new_timeout;
  new_timeout = camWaitExpose (chip);
  if (new_timeout >= 0)
    {
      setTimeout (new_timeout);
    }
  return 0;
}

int
Rts2DevCamera::camExpose (Rts2Conn * conn, int chip, int light, float exptime)
{
  int ret;

  if (light && nextExp->getValueInteger () != NOT_EXP)
    {
      // que exposure after filter or focuser move ends
      nextExpChip = chip;
      nextExpLight = light;
      nextExpTime = exptime;
      ret = 0;
    }
  else
    {
      nextExpChip = -1;
      ret = camStartExposure (chip, light, exptime);
    }
  if (ret)
    {
      conn->sendCommandEnd (DEVDEM_E_HW, "cannot exposure on chip");
    }
  return ret;
}

long
Rts2DevCamera::camWaitExpose (int chip)
{
  int ret;
  ret = chips[chip]->isExposing ();
  return (ret == 0 ? -2 : ret);
}

int
Rts2DevCamera::camStopExpose (Rts2Conn * conn, int chip)
{
  if (chips[chip]->isExposing () >= 0)
    {
      maskStateChip (chip, CAM_MASK_EXPOSE, CAM_NOEXPOSURE, BOP_TEL_MOVE, 0,
		     "exposure canceled");
      chips[chip]->endExposure ();
      return camStopExpose (chip);
    }
  return -1;
}

int
Rts2DevCamera::camBox (Rts2Conn * conn, int chip, int x, int y, int width,
		       int height)
{
  int ret;
  ret = chips[chip]->box (x, y, width, height);
  if (!ret)
    return ret;
  conn->sendCommandEnd (DEVDEM_E_PARAMSVAL, "invalid box size");
  return ret;
}

int
Rts2DevCamera::camCenter (Rts2Conn * conn, int chip, int in_h, int in_w)
{
  int ret;
  ret = chips[chip]->center (in_h, in_w);
  if (ret)
    conn->sendCommandEnd (DEVDEM_E_PARAMSVAL, "cannot set box size");
  return ret;
}

// when we don't have data connection - handy for exposing
int
Rts2DevCamera::camReadout (int chip)
{
  int ret;
  maskStateChip (chip, CAM_MASK_READING | CAM_MASK_DATA,
		 CAM_READING | CAM_NODATA, BOP_TEL_MOVE, 0,
		 "chip readout started");
  ret = chips[chip]->startReadout (NULL, NULL);
  if (!ret)
    {
      return 0;
    }
  maskStateChip (chip, DEVICE_ERROR_MASK | CAM_MASK_READING,
		 DEVICE_ERROR_HW | CAM_NOTREADING, 0, 0,
		 "chip readout failed");
  return -1;
}

int
Rts2DevCamera::camReadout (Rts2Conn * conn, int chip)
{
  int ret;
  // open data connection - wait socket

  Rts2DevConnData *data_conn;
  data_conn = new Rts2DevConnData (this, conn);

  ret = data_conn->init ();
  // add data connection
  addConnection (data_conn);

  struct sockaddr_in our_addr;

  if (conn->getOurAddress (&our_addr) < 0)
    {
      delete data_conn;
      conn->sendCommandEnd (DEVDEM_E_SYSTEM, "cannot get our address");
      return -1;
    }

  data_conn->setAddress (&our_addr.sin_addr);

  maskStateChip (chip, CAM_MASK_READING | CAM_MASK_DATA,
		 CAM_READING | CAM_NODATA, BOP_TEL_MOVE, 0,
		 "chip readout started");
  ret = chips[chip]->startReadout (data_conn, conn);
  if (!ret)
    {
      return 0;
    }
  maskStateChip (chip, DEVICE_ERROR_MASK | CAM_MASK_READING,
		 DEVICE_ERROR_HW | CAM_NOTREADING, 0, 0,
		 "chip readout failed");
  conn->sendCommandEnd (DEVDEM_E_HW, "cannot read chip");
  return -1;
}

int
Rts2DevCamera::camReadoutExpose (Rts2Conn * conn, int chip, int light,
				 float exptime)
{
  int ret;
  // test if we can do frame transfer..
  if (chips[chip]->supportFrameTransfer ())
    {
      ret = camExpose (conn, chip, light, exptime);
      if (ret)
	return ret;
    }
  else
    {
      // if we don't support frame transfer, store exposure
      nextExp->setValueInteger (FT_EXP);
      nextExpChip = chip;
      nextExpTime = exptime;
      nextExpLight = light;
    }
  return camReadout (conn, chip);
}

int
Rts2DevCamera::camBinning (Rts2Conn * conn, int chip, int x_bin, int y_bin)
{
  int ret;
  ret = chips[chip]->setBinning (x_bin, y_bin);
  if (ret)
    conn->sendCommandEnd (DEVDEM_E_HW, "cannot set requested binning");
  return ret;
}

int
Rts2DevCamera::camStopRead (Rts2Conn * conn, int chip)
{
  int ret;
  ret = camStopRead (chip);
  if (ret)
    conn->sendCommandEnd (DEVDEM_E_HW, "cannot end readout");
  return ret;
}

int
Rts2DevCamera::camCoolMax (Rts2Conn * conn)
{
  int ret = camCoolMax ();
  if (ret)
    conn->sendCommandEnd (DEVDEM_E_HW, "cannot set cooling mode to cool max");
  return ret;
}

int
Rts2DevCamera::camCoolHold (Rts2Conn * conn)
{
  int ret;
  ret = camCoolHold ();
  if (ret)
    conn->sendCommandEnd (DEVDEM_E_HW, "cannot set cooling mode to cool max");
  return ret;
}

int
Rts2DevCamera::camCoolTemp (Rts2Conn * conn, float new_temp)
{
  int ret;
  ret = camCoolTemp (new_temp);
  if (ret)
    conn->sendCommandEnd (DEVDEM_E_HW,
			  "cannot set cooling temp to requested temperature");
  return ret;
}

int
Rts2DevCamera::camCoolShutdown (Rts2Conn * conn)
{
  int ret;
  ret = camCoolShutdown ();
  if (ret)
    conn->sendCommandEnd (DEVDEM_E_HW,
			  "cannot shutdown camera cooling system");
  return ret;
}

int
Rts2DevCamera::camFilter (int new_filter)
{
  int ret = -1;
  if (wheelDevice)
    {
      struct filterStart fs;
      fs.filterName = wheelDevice;
      fs.filter = new_filter;
      postEvent (new Rts2Event (EVENT_FILTER_START_MOVE, (void *) &fs));
      // filter move will be performed
      if (fs.filter == -1)
	{
	  nextExp->setValueInteger (nextExp->
				    getValueInteger () | FILTER_MOVE);
	  ret = 0;
	}
      else
	{
	  nextExp->setValueInteger (nextExp->
				    getValueInteger () & ~FILTER_MOVE);
	  ret = -1;
	}
    }
  else
    {
      ret = filter->setFilterNum (new_filter);
      Rts2ScriptDevice::infoAll ();
    }
  return ret;
}

int
Rts2DevCamera::getStateChip (int chip_num)
{
  return (getState () & (CAM_MASK_CHIP << (chip_num * 4))) >> (chip_num * 4);
}

void
Rts2DevCamera::maskStateChip (int chip_num, int chip_state_mask,
			      int chip_new_state, int state_mask,
			      int new_state, char *description)
{
  maskState (state_mask | (chip_state_mask << (4 * chip_num)),
	     new_state | (chip_new_state << (4 * chip_num)), description);
}

int
Rts2DevCamera::getFilterNum ()
{
  if (wheelDevice)
    {
      struct filterStart fs;
      fs.filterName = wheelDevice;
      fs.filter = -1;
      postEvent (new Rts2Event (EVENT_FILTER_GET, (void *) &fs));
      return fs.filter;
    }
  else if (filter)
    {
      return filter->getFilterNum ();
    }
  return 0;
}

int
Rts2DevCamera::setFocuser (int new_set)
{
  if (!focuserDevice)
    {
      return -1;
    }
  struct focuserMove fm;
  fm.focuserName = focuserDevice;
  fm.value = new_set;
  postEvent (new Rts2Event (EVENT_FOCUSER_SET, (void *) &fm));
  if (fm.focuserName)
    {
      nextExp->setValueInteger (nextExp->getValueInteger () & ~FOCUSER_MOVE);
      return -1;
    }
  nextExp->setValueInteger (nextExp->getValueInteger () | FOCUSER_MOVE);
  return 0;
}

int
Rts2DevCamera::stepFocuser (int step_count)
{
  if (!focuserDevice)
    {
      return -1;
    }
  struct focuserMove fm;
  fm.focuserName = focuserDevice;
  fm.value = step_count;
  postEvent (new Rts2Event (EVENT_FOCUSER_STEP, (void *) &fm));
  if (fm.focuserName)
    {
      nextExp->setValueInteger (nextExp->getValueInteger () & ~FOCUSER_MOVE);
      return -1;
    }
  nextExp->setValueInteger (nextExp->getValueInteger () | FOCUSER_MOVE);
  return 0;
}

int
Rts2DevCamera::getFocPos ()
{
  if (!focuserDevice)
    return -1;
  struct focuserMove fm;
  fm.focuserName = focuserDevice;
  postEvent (new Rts2Event (EVENT_FOCUSER_GET, (void *) &fm));
  if (fm.focuserName)
    return -1;
  return fm.value;
}

int
Rts2DevCamera::startFocus (Rts2Conn * conn)
{
  if (isFocusing ())
    {
      conn->sendCommandEnd (DEVDEM_E_HW, "already performing autofocus");
      return -1;
    }
  focusExposure = defFocusExposure;
  // idle routine will check for that..
  maskState (CAM_MASK_FOCUSING, CAM_FOCUSING);
  return 0;
}

int
Rts2DevCamera::endFocusing ()
{
  maskState (CAM_MASK_FOCUSING, CAM_NOFOCUSING);
  // to reset binnings etc..
  scriptEnds ();
  return 0;
}

bool
Rts2DevCamera::isIdle ()
{
  return ((getStateChip (0) &
	   (CAM_MASK_EXPOSE | CAM_MASK_DATA | CAM_MASK_READING)) ==
	  (CAM_NOEXPOSURE | CAM_NODATA | CAM_NOTREADING));
}

bool
Rts2DevCamera::isFocusing ()
{
  return ((getStateChip (0) & CAM_MASK_FOCUSING) == CAM_FOCUSING);
}

int
Rts2DevCamera::paramNextChip (Rts2Conn * conn, int *in_chip)
{
  int ret;
  ret = conn->paramNextInteger (in_chip);
  if (ret)
    return ret;
  if (*in_chip < 0 || *in_chip >= MAX_CHIPS)
    {
      return -1;
    }
  return 0;
}

int
Rts2DevCamera::commandAuthorized (Rts2Conn * conn)
{
  int chip;
  float exptime;
  int light;

  if (conn->isCommand ("chipinfo"))
    {
      if (paramNextChip (conn, &chip) || !conn->paramEnd ())
	return -2;
      return camChipInfo (conn, chip);
    }
  else if (conn->isCommand ("help"))
    {
      conn->send ("ready - is camera ready?");
      conn->send ("info - information about camera");
      conn->send ("chipinfo <chip> - information about chip");
      conn->send
	("expose <chip> <light> <exposure> - start exposition on given chip");
      conn->send ("stopexpo <chip> - stop exposition on given chip");
      conn->send ("progexpo <chip> - query exposition progress");
      conn->send ("readout <chip> - start reading given chip");
      conn->send
	("readout_exposure <chip> <light> <exposure> - start readout and start/que next exposure, handy for frame transfer CDDs");
      conn->send ("mirror <open|close> - open/close mirror");
      conn->send
	("binning <chip> <binning_id> - set new binning; actual from next readout on");
      conn->send ("stopread <chip> - stop reading given chip");
      conn->send ("cooltemp <temp> - cooling temperature");
      conn->send ("focus - try to autofocus picture");
      conn->send ("exit - exit from connection");
      conn->send ("help - print, what you are reading just now");
      return 0;
    }
  // commands which requires priority
  // priority test must come after command string test,
  // otherwise we will be unable to answer DEVDEM_E_PRIORITY
  else if (conn->isCommand ("expose"))
    {
      CHECK_PRIORITY;
      if (paramNextChip (conn, &chip)
	  || conn->paramNextInteger (&light)
	  || conn->paramNextFloat (&exptime) || !conn->paramEnd ())
	return -2;
      return camExpose (conn, chip, light, exptime);
    }
  else if (conn->isCommand ("stopexpo"))
    {
      CHECK_PRIORITY;
      if (paramNextChip (conn, &chip) || !conn->paramEnd ())
	return -2;
      return camStopExpose (conn, chip);
    }
  else if (conn->isCommand ("box"))
    {
      int x, y, w, h;
      CHECK_PRIORITY;
      if (paramNextChip (conn, &chip)
	  || conn->paramNextInteger (&x)
	  || conn->paramNextInteger (&y)
	  || conn->paramNextInteger (&w) || conn->paramNextInteger (&h)
	  || !conn->paramEnd ())
	return -2;
      return camBox (conn, chip, x, y, w, h);
    }
  else if (conn->isCommand ("center"))
    {
      CHECK_PRIORITY;
      if (paramNextChip (conn, &chip))
	return -2;
      int w, h;
      if (conn->paramEnd ())
	{
	  w = -1;
	  h = -1;
	}
      else
	{
	  if (conn->paramNextInteger (&w) || conn->paramNextInteger (&h)
	      || !conn->paramEnd ())
	    return -2;
	}
      return camCenter (conn, chip, w, h);
    }
  else if (conn->isCommand ("readout"))
    {
      CHECK_PRIORITY;
      if (paramNextChip (conn, &chip) || !conn->paramEnd ())
	return -2;
      return camReadout (conn, chip);
    }
  else if (conn->isCommand ("readout_exposure"))
    {
      CHECK_PRIORITY;
      if (paramNextChip (conn, &chip)
	  || conn->paramNextInteger (&light)
	  || conn->paramNextFloat (&exptime) || !conn->paramEnd ())
	return -2;
      return camReadoutExpose (conn, chip, light, exptime);
    }
  else if (conn->isCommand ("binning"))
    {
      int vertical, horizontal;
      CHECK_PRIORITY;
      if (paramNextChip (conn, &chip)
	  || conn->paramNextInteger (&vertical)
	  || conn->paramNextInteger (&horizontal) || !conn->paramEnd ())
	return -2;
      return camBinning (conn, chip, vertical, horizontal);
    }
  else if (conn->isCommand ("stopread"))
    {
      CHECK_PRIORITY;
      if (paramNextChip (conn, &chip) || !conn->paramEnd ())
	return -2;
      return camStopRead (conn, chip);
    }
  else if (conn->isCommand ("coolmax"))
    {
      return camCoolMax (conn);
    }
  else if (conn->isCommand ("coolhold"))
    {
      return camCoolHold (conn);
    }
  else if (conn->isCommand ("cooltemp"))
    {
      float new_temp;
      if (conn->paramNextFloat (&new_temp) || !conn->paramEnd ())
	return -2;
      return camCoolTemp (conn, new_temp);
    }
  else if (conn->isCommand ("focus"))
    {
      return startFocus (conn);
    }
  return Rts2ScriptDevice::commandAuthorized (conn);
}
