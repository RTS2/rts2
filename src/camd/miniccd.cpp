#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <fcntl.h>
#include <signal.h>
#include "camera_cpp.h"

#include "ccd_msg.h"
#include "../utils/rts2device.h"

class CameraMiniccdChip:public CameraChip
{
private:
  int fd_chip;
  char *device_name;
  int sizeof_pixel;
  char *_data;
  char *send_top;
  char *dest_top;

  int ccd_dac_bits;

  int usedRowBytes;

  int interleavedReadout;

  CCD_ELEM_TYPE msgw[CCD_MSG_CCD_LEN / CCD_ELEM_SIZE];
  CCD_ELEM_TYPE msgr[CCD_MSG_CCD_LEN / CCD_ELEM_SIZE];
public:
    CameraMiniccdChip (Rts2DevCamera * in_cam, int in_chip_id,
		       int in_fd_chip);
    CameraMiniccdChip (Rts2DevCamera * in_cam, int in_chip_id,
		       char *chip_dev);
    virtual ~ CameraMiniccdChip (void);
  virtual int init ();
  int startExposureInterleaved (int light, float exptime, int ccdFlags = 0)
  {
    return startExposure (light, exptime, ccdFlags, 1);
  }
  virtual int startExposure (int light, float exptime)
  {
    return startExposure (light, exptime, 0, 0);
  }
  int startExposure (int ligth, float exptime, int ccdFlags,
		     int in_interleaved);
  void setInterleavedReadout ()
  {
    interleavedReadout = 1;
  }
  virtual long isExposing ();
  virtual int stopExposure ();
  virtual int readoutOneLine ();
  virtual int endReadout ()
  {
    interleavedReadout = 0;
    return CameraChip::endReadout ();
  }
  int sendLineData (int numLines = -1);
  char *getLineData ();

  int getSizeOfPixel ()
  {
    return sizeof_pixel;
  }

  int getUsedRowBytes ()
  {
    return usedRowBytes;
  }

  bool haveUnsendData ()
  {
    return (!(send_top >= dest_top || !_data));
    /*
       if (send_top >= dest_top || !_data)
       return false;
       return true;
     */
  }
};

CameraMiniccdChip::CameraMiniccdChip (Rts2DevCamera * in_cam, int in_chip_id, int in_fd_chip):
CameraChip (in_cam, in_chip_id)
{
  device_name = NULL;
  fd_chip = in_fd_chip;
  _data = NULL;
  interleavedReadout = 0;
}

CameraMiniccdChip::CameraMiniccdChip (Rts2DevCamera * in_cam, int in_chip_id,
				      char *chip_dev):
CameraChip (in_cam, in_chip_id)
{
  device_name = chip_dev;
  fd_chip = -1;
  _data = NULL;
}

CameraMiniccdChip::~CameraMiniccdChip (void)
{
  delete _data;
  close (fd_chip);
}

int
CameraMiniccdChip::init ()
{
  int in_width;
  int in_height;

  int msg_len;

  if (fd_chip < 0)
    {
      fd_chip = open (device_name, O_RDWR);
      if (fd_chip < 0)
	{
	  syslog (LOG_ERR,
		  "CameraMiniccdChip::init cannot open device file: %m");
	  return fd_chip;
	}
    }

  msgw[CCD_MSG_HEADER_INDEX] = CCD_MSG_HEADER;
  msgw[CCD_MSG_LENGTH_LO_INDEX] = CCD_MSG_QUERY_LEN;
  msgw[CCD_MSG_LENGTH_HI_INDEX] = 0;
  msgw[CCD_MSG_INDEX] = CCD_MSG_QUERY;

  write (fd_chip, (char *) msgw, CCD_MSG_QUERY_LEN);
  msg_len = read (fd_chip, (char *) msgr, CCD_MSG_CCD_LEN);

  if (msg_len != CCD_MSG_CCD_LEN)
    {
      syslog (LOG_ERR,
	      "CameraMiniccdChip::init CCD message length wrong: %d %m",
	      msg_len);
      return -1;
    }

  in_width = msgr[CCD_CCD_WIDTH_INDEX];
  in_height = msgr[CCD_CCD_HEIGHT_INDEX];

  sizeof_pixel = (msgr[CCD_CCD_DEPTH_INDEX] + 7) / 8;

  setSize (in_width, in_height, 0, 0);

  _data = new char[in_height * in_width * sizeof_pixel + CCD_MSG_IMAGE_LEN];

  ccd_dac_bits = msgr[CCD_CCD_DAC_INDEX];

  return 0;
}

int
CameraMiniccdChip::startExposure (int light, float exptime,
				  int ccdFlags, int in_interleaved)
{
  CCD_ELEM_TYPE msg[CCD_MSG_EXP_LEN / CCD_ELEM_SIZE];
  int exposure_msec = (int) (exptime * 1000);
  int ret;

  send_top = _data;
  dest_top = _data;

  ccdFlags |= light ? 0 : CCD_EXP_FLAGS_NOOPEN_SHUTTER;

  msg[CCD_MSG_HEADER_INDEX] = CCD_MSG_HEADER;
  msg[CCD_MSG_LENGTH_LO_INDEX] = CCD_MSG_EXP_LEN;
  msg[CCD_MSG_LENGTH_HI_INDEX] = 0;
  msg[CCD_MSG_INDEX] = CCD_MSG_EXP;
  msg[CCD_EXP_WIDTH_INDEX] = chipReadout->width;
  msg[CCD_EXP_HEIGHT_INDEX] = chipReadout->height;
  msg[CCD_EXP_XOFFSET_INDEX] = chipReadout->x;
  msg[CCD_EXP_YOFFSET_INDEX] = chipReadout->y;
  msg[CCD_EXP_XBIN_INDEX] = binningHorizontal;
  msg[CCD_EXP_YBIN_INDEX] = binningVertical;
  msg[CCD_EXP_DAC_INDEX] = ccd_dac_bits;
  msg[CCD_EXP_FLAGS_INDEX] = ccdFlags;
  msg[CCD_EXP_MSEC_LO_INDEX] = exposure_msec & 0xFFFF;
  msg[CCD_EXP_MSEC_HI_INDEX] = exposure_msec >> 16;
  ret = write (fd_chip, (char *) msg, CCD_MSG_EXP_LEN);
  if (ret == CCD_MSG_EXP_LEN)
    {
      chipUsedReadout = new ChipSubset (chipReadout);
      usedBinningVertical = binningVertical;
      usedBinningHorizontal = binningHorizontal;
      usedRowBytes =
	(chipUsedReadout->width / usedBinningHorizontal) * sizeof_pixel;
      readoutLine = 0;
      if (in_interleaved)
	setInterleavedReadout ();
      return 0;
    }
  return -1;
}

long
CameraMiniccdChip::isExposing ()
{
  fd_set set;
  struct timeval read_tout;
  long ret;

  ret = CameraChip::isExposing ();
  if (ret > 0)
    return ret;

  if (!chipUsedReadout
      || readoutLine >= (chipUsedReadout->height / usedBinningVertical))
    {
      return 0;
    }

  FD_ZERO (&set);
  FD_SET (fd_chip, &set);
  read_tout.tv_sec = read_tout.tv_usec = 0;
  select (fd_chip + 1, &set, NULL, NULL, &read_tout);
  if (FD_ISSET (fd_chip, &set))
    {
      int row_bytes = usedRowBytes;

      // readout some data, use them..
      int ret1;
      if (readoutLine == 0)
	row_bytes += CCD_MSG_IMAGE_LEN;
      ret1 = read (fd_chip, dest_top, row_bytes);
      // second try should help in case of header, which can be passed
      // in different readout:(
      if (ret1 != row_bytes)
	{
	  int ret2;
	  ret2 = read (fd_chip, dest_top + ret1, row_bytes - ret1);
	  // that's an error
	  if (ret2 + ret1 != row_bytes)
	    {
	      syslog (LOG_ERR,
		      "CameraMiniccdChip::isExposing cannot readout line");
	      endReadout ();
	      return -2;
	    }
	}
      dest_top += row_bytes;
      readoutLine++;
      return (readoutLine == 1) ? 2 : 1;
    }
  return 10;
}

int
CameraMiniccdChip::readoutOneLine ()
{
  int ret;

  if (interleavedReadout)
    return -1;

  if (sendLine == 0)
    {
      ret = CameraChip::sendFirstLine ();
      if (ret)
	return ret;
    }

  ret = sendLineData ();
  return ret;
}

int
CameraMiniccdChip::sendLineData (int numLines)
{
  int send_data_size;

  if (sendLine == 0)
    {
      // test data size & ignore data header
      CCD_ELEM_TYPE *msg = (CCD_ELEM_TYPE *) _data;
      if (msg[CCD_MSG_INDEX] != CCD_MSG_IMAGE)
	{
	  syslog (LOG_ERR,
		  "CameraMiniccdChip::sendLineData wrong image message");
	  return -1;
	}
      if (!chipUsedReadout)
	{
	  syslog (LOG_ERR,
		  "CameraMiniccdChip::sendLineData not chipUsedReadout");
	  return -1;
	}
      if ((unsigned int) (msg[CCD_MSG_LENGTH_LO_INDEX] +
			  (msg[CCD_MSG_LENGTH_HI_INDEX] << 16)) !=
	  ((chipUsedReadout->height / usedBinningVertical) *
	   usedRowBytes) + CCD_MSG_IMAGE_LEN)
	{
	  syslog (LOG_ERR,
		  "CameraMiniccdChip::sendLineData wrong size %i",
		  msg[CCD_MSG_LENGTH_LO_INDEX] +
		  (msg[CCD_MSG_LENGTH_HI_INDEX] << 16));
	  return -1;
	}
      send_top += CCD_MSG_IMAGE_LEN;
    }
  sendLine++;
  if (numLines == -1)
    {
      send_data_size = sendReadoutData (send_top, dest_top - send_top);
    }
  else
    {
      // assume that connection will not fail..
      send_data_size = sendReadoutData (send_top, numLines * usedRowBytes);
    }
  if (send_data_size < 0)
    return -1;

  send_top += send_data_size;
  if (send_top >= dest_top || !_data)
    return -2;			// end OK
  return 0;
}

char *
CameraMiniccdChip::getLineData ()
{
  char *ret;

  if (sendLine == 0)
    {
      // test data size & ignore data header
      CCD_ELEM_TYPE *msg = (CCD_ELEM_TYPE *) _data;
      if (msg[CCD_MSG_INDEX] != CCD_MSG_IMAGE)
	{
	  syslog (LOG_ERR,
		  "CameraMiniccdChip::getLineData wrong image message");
	  return NULL;
	}
      if (!chipUsedReadout)
	{
	  syslog (LOG_ERR,
		  "CameraMiniccdChip::getLineData not chipUsedReadout");
	  return NULL;
	}
      if ((unsigned int) (msg[CCD_MSG_LENGTH_LO_INDEX] +
			  (msg[CCD_MSG_LENGTH_HI_INDEX] << 16)) !=
	  ((chipUsedReadout->height / usedBinningVertical) *
	   usedRowBytes) + CCD_MSG_IMAGE_LEN)
	{
	  syslog (LOG_ERR,
		  "CameraMiniccdChip::getLineData wrong size %i",
		  msg[CCD_MSG_LENGTH_LO_INDEX] +
		  (msg[CCD_MSG_LENGTH_HI_INDEX] << 16));
	  return NULL;
	}
      send_top += CCD_MSG_IMAGE_LEN;
    }
  sendLine++;
  ret = send_top;
  send_top += usedRowBytes;
  return ret;
}

int
CameraMiniccdChip::stopExposure ()
{
  chipUsedReadout = NULL;
  CCD_ELEM_TYPE msg[CCD_MSG_ABORT_LEN / CCD_ELEM_SIZE];
  /*
   * Send the abort request.
   */
  msg[CCD_MSG_HEADER_INDEX] = CCD_MSG_HEADER;
  msg[CCD_MSG_LENGTH_LO_INDEX] = CCD_MSG_ABORT_LEN;
  msg[CCD_MSG_LENGTH_HI_INDEX] = 0;
  msg[CCD_MSG_INDEX] = CCD_MSG_ABORT;
  write (fd_chip, (char *) msg, CCD_MSG_ABORT_LEN);
  return CameraChip::stopExposure ();
}

/**************************************************************
 *
 * This class is used for interleaved chip readout.
 * It combines two chips together..
 *
 **************************************************************/

class CameraMiniccdInterleavedChip:public CameraChip
{
private:
  enum
  { NO_ACTION, SLAVE1_EXPOSING, SLAVE2_EXPOSING, SLAVE1_READOUT,
    SLAVE2_READOUT
  } slaveState;
  CameraMiniccdChip *slaveChip[2];
  long firstReadoutTime;
  struct timeval slave1ReadoutStart;
  struct timeval slave2ExposureStart;
  int chip1_light;
  float chip1_exptime;
  int usedRowBytes;
  // do 2x2 binning
  void doBinning (uint16_t * row1, uint16_t * row2);
public:
    CameraMiniccdInterleavedChip (Rts2DevCamera * in_cam, int in_chip_id,
				  int in_fd_chip,
				  CameraMiniccdChip * in_chip1,
				  CameraMiniccdChip * in_chip2);
    virtual ~ CameraMiniccdInterleavedChip (void);
  virtual int init ();
  virtual int startExposure (int light, float exptime);
  virtual long isExposing ();
  virtual int stopExposure ();
  virtual int startReadout (Rts2DevConnData * dataConn, Rts2Conn * conn);
  virtual int endReadout ();
  virtual int readoutOneLine ();
};

CameraMiniccdInterleavedChip::CameraMiniccdInterleavedChip (Rts2DevCamera *
							    in_cam,
							    int in_chip_id,
							    int in_fd_chip,
							    CameraMiniccdChip
							    * in_chip1,
							    CameraMiniccdChip
							    * in_chip2):
CameraChip (in_cam, in_chip_id)
{
  slaveChip[0] = in_chip1;
  slaveChip[1] = in_chip2;
  firstReadoutTime = 3 * USEC_SEC;	// default readout is expected to last 3 sec..
  slaveState = NO_ACTION;
}

CameraMiniccdInterleavedChip::~CameraMiniccdInterleavedChip (void)
{

}

void
CameraMiniccdInterleavedChip::doBinning (uint16_t * row1, uint16_t * row2)
{
  uint16_t *row_out = row1;
  // row offset
  int i = 0;
  int out = 0;
  // bin it now - first row;
  for (i = 0; i < (usedRowBytes); i++)
    {
      out += *row1;
      row1++;
      out += *row2;
      row2++;
      // divide by 4..
      if ((i % 2) == 1)
	{
	  out /= 4;
	  *row_out = (uint16_t) out;
	  row_out++;
	  out = 0;
	}
    }
}

int
CameraMiniccdInterleavedChip::init ()
{
  int in_width;
  int in_height;

  int sizeof_pixel;

  in_width = slaveChip[0]->getWidth ();

  if (in_width != slaveChip[1]->getWidth ())
    {
      syslog (LOG_ERR, "CameraMiniccdInterleavedChip::init not same width");
      return -1;
    }

  sizeof_pixel = slaveChip[0]->getSizeOfPixel ();

  if (sizeof_pixel != slaveChip[1]->getSizeOfPixel ())
    {
      syslog (LOG_ERR,
	      "CameraMiniccdInterleavedChip::init not same sizeof pixel");
      return -1;
    }

  in_height = slaveChip[0]->getHeight () + slaveChip[1]->getHeight ();

  setSize (in_width, in_height, 0, 0);

  return 0;
}

int
CameraMiniccdInterleavedChip::startExposure (int light, float exptime)
{
  int ret;

  gettimeofday (&slave2ExposureStart, NULL);

  ret = slaveChip[0]->startExposureInterleaved (light, exptime);
  if (ret)
    return -1;

  slave2ExposureStart.tv_usec += firstReadoutTime;

  slave2ExposureStart.tv_sec += slave2ExposureStart.tv_usec / USEC_SEC;
  slave2ExposureStart.tv_usec %= USEC_SEC;

  slaveState = SLAVE1_EXPOSING;

  chip1_light = light;
  chip1_exptime = exptime;

  return 0;
}

long
CameraMiniccdInterleavedChip::isExposing ()
{
  struct timeval now;
  long ret;

  switch (slaveState)
    {
    case SLAVE1_EXPOSING:
      gettimeofday (&now, NULL);
      if (timercmp (&now, &slave2ExposureStart, <))
	return (now.tv_sec - slave2ExposureStart.tv_sec) * USEC_SEC +
	  (now.tv_usec - slave2ExposureStart.tv_usec);
      ret =
	slaveChip[1]->startExposureInterleaved (chip1_light, chip1_exptime,
						CCD_EXP_FLAGS_NOWIPE_FRAME);
      if (ret)
	{
	  return -2;
	}
      slaveState = SLAVE2_EXPOSING;
    case SLAVE2_EXPOSING:
      ret = slaveChip[0]->isExposing ();
      if (ret == 2)
	{
	  gettimeofday (&slave1ReadoutStart, NULL);
	  ret = 1;
	}
      if (ret)
	return ret;
      // calculate firstReadoutTime
      gettimeofday (&now, NULL);
      firstReadoutTime = (now.tv_sec - slave1ReadoutStart.tv_sec) * USEC_SEC +
	(now.tv_usec - slave1ReadoutStart.tv_usec);
      slaveState = SLAVE1_READOUT;
    case SLAVE1_READOUT:
      ret = slaveChip[1]->isExposing ();
      if (ret)
	return ret;
      slaveState = SLAVE2_READOUT;
      break;
    default:
      return CameraChip::isExposing ();
    }
  return 0;
}

int
CameraMiniccdInterleavedChip::stopExposure ()
{
  return slaveChip[0]->stopExposure () | slaveChip[1]->stopExposure ();
}

int
CameraMiniccdInterleavedChip::startReadout (Rts2DevConnData * dataConn,
					    Rts2Conn * conn)
{
  slaveChip[0]->setReadoutConn (dataConn);
  slaveChip[1]->setReadoutConn (dataConn);
  // we use 2x2 default bin
  chipUsedReadout =
    new ChipSubset (chipReadout->x / 2, chipReadout->y / 2,
		    chipReadout->getWidth () / 2,
		    chipReadout->getHeight () / 2);
  usedBinningVertical = 1;
  usedBinningHorizontal = 1;
  return CameraChip::startReadout (dataConn, conn);
}

int
CameraMiniccdInterleavedChip::endReadout ()
{
  slaveChip[0]->clearReadout ();
  slaveChip[1]->clearReadout ();
  return CameraChip::endReadout ();
}

int
CameraMiniccdInterleavedChip::readoutOneLine ()
{
  int ret;
  char *row1, *row2;

  if (sendLine == 0)
    {
      ret = CameraChip::sendFirstLine ();
      if (ret)
	return ret;
      usedRowBytes = slaveChip[0]->getUsedRowBytes () / 2;
    }

  switch (slaveState)
    {
    case SLAVE2_READOUT:
      row1 = slaveChip[0]->getLineData ();
      row2 = slaveChip[1]->getLineData ();
      if (!(row1 && row2))
	{
	  // error while retrieving data
	  return -1;
	}
      doBinning ((uint16_t *) row1, (uint16_t *) row2);
      // calculate new data in ret1
      sendReadoutData (row1, usedRowBytes);
      sendLine++;
      if (slaveChip[0]->haveUnsendData ())
	return 0;
      return -2;
    default:
      slaveState = NO_ACTION;
    }
  return -2;
}

/****************************************************************
 *
 * Camera main class
 *
 ****************************************************************/

class Rts2DevCameraMiniccd:public Rts2DevCamera
{
  int fd_ccd;
  int interleave;
  char *device_file;
public:
    Rts2DevCameraMiniccd (int argc, char **argv);
    virtual ~ Rts2DevCameraMiniccd (void);

  virtual int processOption (int in_opt);
  virtual int initChips ();
  virtual int init ();

  virtual int ready ();
  virtual int info ();
  virtual int baseInfo ();
  virtual int camChipInfo (int chip)
  {
    return !(fd_ccd != -1);
  }
  virtual int camCoolMax ();
  virtual int camCoolHold ();
  virtual int camCoolTemp (float new_temp);
  virtual int camCoolShutdown ();
  virtual int camFilter (int new_filter);
};

Rts2DevCameraMiniccd::Rts2DevCameraMiniccd (int in_argc, char **in_argv):
Rts2DevCamera (in_argc, in_argv)
{
  fd_ccd = -1;
  interleave = 0;
  device_file = NULL;

  addOption ('f', "device_file", 1, "miniccd device file (/dev/xxx entry)");
}

Rts2DevCameraMiniccd::~Rts2DevCameraMiniccd (void)
{
  close (fd_ccd);
}

int
Rts2DevCameraMiniccd::processOption (int in_opt)
{
  switch (in_opt)
    {
    case 'f':
      device_file = optarg;
      break;
    default:
      return Rts2DevCamera::processOption (in_opt);
    }
  return 0;
}

int
Rts2DevCameraMiniccd::initChips ()
{
  // init master as last chip - if it's interlaced, we need info about
  // slave chips
  int ret;
  for (int i = 1; i < chipNum; i++)
    {
      ret = chips[i]->init ();
      if (ret)
	return ret;
      if (defBinning != 1)
	chips[i]->setBinning (defBinning, defBinning);
    }
  ret = chips[0]->init ();
  chips[0]->setBinning (defBinning, defBinning);
  return ret;
};

int
Rts2DevCameraMiniccd::init ()
{
  int ret;
  int msg_len, i;
  CCD_ELEM_TYPE msgw[CCD_MSG_CCD_LEN / CCD_ELEM_SIZE];
  CCD_ELEM_TYPE msgr[CCD_MSG_CCD_LEN / CCD_ELEM_SIZE];

  ret = Rts2DevCamera::init ();
  if (ret)
    return ret;

  fd_ccd = open (device_file, O_RDWR);
  if (fd_ccd < 0)
    {
      syslog (LOG_ERR,
	      "Rts2DevCameraMiniccd::init Unable to open device: %s %m",
	      device_file);
      return -1;
    }
  /*
   * Request CCD parameters.
   */
  msgw[CCD_MSG_HEADER_INDEX] = CCD_MSG_HEADER;
  msgw[CCD_MSG_LENGTH_LO_INDEX] = CCD_MSG_QUERY_LEN;
  msgw[CCD_MSG_LENGTH_HI_INDEX] = 0;
  msgw[CCD_MSG_INDEX] = CCD_MSG_QUERY;
  write (fd_ccd, (char *) msgw, CCD_MSG_QUERY_LEN);
  msg_len = read (fd_ccd, (char *) msgr, CCD_MSG_CCD_LEN);
  if (msg_len != CCD_MSG_CCD_LEN)
    {
      syslog (LOG_ERR,
	      "Rts2DevCameraMiniccd::init CCD message length wrong: %d",
	      msg_len);
      return -1;
    }
  /*
   * Response from CCD query.
   */
  if (msgr[CCD_MSG_INDEX] != CCD_MSG_CCD)
    {
      syslog (LOG_ERR,
	      "Rts2DevCameraMiniccd::init Wrong message returned from query: 0x%04X",
	      msgr[CCD_MSG_INDEX]);
      return -1;
    }
  chipNum = msgr[CCD_CCD_FIELDS_INDEX];

  if (chipNum > 1)
    {
      // 0 chip is compostition of two interleaved exposures
      chipNum++;
      interleave = 1;
    }

  strncpy (ccdType, (char *) &msgr[CCD_CCD_NAME_INDEX], CCD_CCD_NAME_LEN / 2);

  for (i = 0; i < CCD_CCD_NAME_LEN / 2; i++)
    if (ccdType[i] == ' ')
      ccdType[i] = '_';

  ccdType[CCD_CCD_NAME_LEN] = '\x0';

  CameraMiniccdChip *miniChips[2];

  for (i = 1; i < chipNum; i++)
    {
      char *chip_device_name;
      chip_device_name = new char[strlen (device_file) + 2];
      strcpy (chip_device_name, device_file);
      chip_device_name[strlen (device_file)] = i + '0';
      chip_device_name[strlen (device_file) + 1] = '\x0';
      miniChips[i - 1] = new CameraMiniccdChip (this, i, chip_device_name);
      chips[i] = miniChips[i - 1];
    }

  if (interleave)
    {
      chips[0] =
	new CameraMiniccdInterleavedChip (this, 0, fd_ccd, miniChips[0],
					  miniChips[1]);
    }
  else
    {
      chips[0] = new CameraMiniccdChip (this, 0, fd_ccd);
    }

  canDF = 0;			// starlight cameras cannot do DF
  return Rts2DevCameraMiniccd::initChips ();
}

int
Rts2DevCameraMiniccd::ready ()
{
  return !(fd_ccd != -1);
}

int
Rts2DevCameraMiniccd::baseInfo ()
{
  return 0;
}

int
Rts2DevCameraMiniccd::info ()
{
  return 0;
}

int
Rts2DevCameraMiniccd::camCoolMax ()
{
/*  CCD_ELEM_TYPE msg[CCD_MSG_TEMP_LEN / CCD_ELEM_SIZE];

  msg[CCD_MSG_HEADER_INDEX] = CCD_MSG_HEADER;
  msg[CCD_MSG_LENGTH_LO_INDEX] = CCD_MSG_TEMP_LEN;
  msg[CCD_MSG_LENGTH_HI_INDEX] = 0;
  msg[CCD_MSG_INDEX] = CCD_MSG_TEMP;
  msg[CCD_TEMP_SET_HI_INDEX] = 0;
  msg[CCD_TEMP_SET_LO_INDEX] = 0;
  write (fd_ccd, (char *) msg, CCD_MSG_TEMP_LEN);
  read (fd_ccd, (char *) msg, CCD_MSG_TEMP_LEN);*/
  return 0;
}

int
Rts2DevCameraMiniccd::camCoolHold ()
{
  return 0;
}

int
Rts2DevCameraMiniccd::camCoolTemp (float new_temp)
{
  return 0;
}

int
Rts2DevCameraMiniccd::camCoolShutdown ()
{
  return 0;
}

int
Rts2DevCameraMiniccd::camFilter (int in_filter)
{
  return 0;
}

Rts2DevCameraMiniccd *device;

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
  device = new Rts2DevCameraMiniccd (argc, argv);

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
