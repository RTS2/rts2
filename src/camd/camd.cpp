#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#define MAX_CHIPS 	2	//!maximal number of chips

#include <getopt.h>
#include <mcheck.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>

#include "../utils/rts2device.h"
#include "../utils/rts2block.h"

#include "camera_cpp.h"
#include "imghdr.h"

CameraChip::CameraChip (int in_chip_id)
{
  chipId = in_chip_id;
  exposureEnd.tv_sec = 0;
  exposureEnd.tv_usec = 0;
  readoutConn = NULL;
  binningVertical = 0;
  binningHorizontal = 0;
  pixelX = 0;
  pixelY = 0;
  gain = 0;
  chipSize = new ChipSubset (0, 0, 0, 0);
  chipReadout = new ChipSubset (0, 0, 0, 0);
  readoutLine = -1;
  sendLine = -1;
}

CameraChip::CameraChip (int in_chip_id, int in_width, int in_height,
			int in_pixelX, int in_pixelY, float in_gain)
{
  chipId = in_chip_id;
  exposureEnd.tv_sec = 0;
  exposureEnd.tv_usec = 0;
  readoutConn = NULL;
  binningVertical = 1;
  binningHorizontal = 1;
  pixelX = in_pixelX;
  pixelY = in_pixelY;
  gain = in_gain;
  chipSize = new ChipSubset (0, 0, in_width, in_height);
  chipReadout = new ChipSubset (0, 0, in_width, in_height);
  readoutLine = -1;
  sendLine = -1;
}

CameraChip::~CameraChip (void)
{
  delete chipSize;
  delete chipReadout;
}

int
CameraChip::setExposure (float exptime)
{
  struct timeval tv;
  gettimeofday (&tv, NULL);

  long int f_exptime = (long int) floor (exptime);
  exposureEnd.tv_sec = tv.tv_sec + f_exptime;
  exposureEnd.tv_usec =
    tv.tv_usec + (long int) ((exptime - f_exptime) * 1000000);
  if (tv.tv_usec > 1000000)
    {
      exposureEnd.tv_sec += tv.tv_usec / 1000000;
      exposureEnd.tv_usec = tv.tv_usec % 1000000;
    }
  return 0;
}

/**
 * Check if exposure has ended.
 *
 * @return 0 if there was pending exposure which ends, -1 if there wasn't any exposure, > 0 time remainnign till end of exposure
 */
int
CameraChip::checkExposure ()
{
  struct timeval tv;
  if (exposureEnd.tv_sec == 0 && exposureEnd.tv_usec == 0)
    return -1;			// no exposure running
  gettimeofday (&tv, NULL);
  if (tv.tv_sec > exposureEnd.tv_sec
      || (tv.tv_sec == exposureEnd.tv_sec
	  && tv.tv_usec >= exposureEnd.tv_usec))
    {
      endExposure ();
      return 0;			// exposure ended
    }
  return -1;
}

int
CameraChip::endExposure ()
{
  exposureEnd.tv_sec = 0;
  exposureEnd.tv_usec = 0;
  return 0;
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
CameraChip::send (Rts2Conn * conn)
{
  sendChip (conn, "width", chipSize->width);
  sendChip (conn, "height", chipSize->height);
  sendChip (conn, "binning_vertical", binningVertical);
  sendChip (conn, "binning_horizontal", binningHorizontal);
  sendChip (conn, "pixelX", chipSize->x);
  sendChip (conn, "pixelY", chipSize->y);
  sendChip (conn, "gain", gain);
}

int
CameraChip::startReadout (Rts2DevConnData * dataConn, Rts2Conn * conn)
{
  char *msg;
  char address[200];
  readoutConn = dataConn;
  readoutLine = 0;
  sendLine = 0;
  dataConn->getAddress ((char *) &address, 200);
  asprintf (&msg, "D connect %i %s:%i %i", chipId, address,
	    dataConn->getLocalPort (),
	    chipReadout->width * chipReadout->height *
	    sizeof (unsigned short) + sizeof (imghdr));
  conn->send (msg);
  free (msg);
  return 0;
}

int
CameraChip::sendFirstLine ()
{
  if (readoutConn)
    {
      struct imghdr header;
      header.data_type = 1;
      header.naxes = 2;
      header.sizes[0] = chipReadout->width;
      header.sizes[1] = chipReadout->height;
      header.binnings[0] = binningHorizontal;
      header.binnings[1] = binningVertical;
      header.status = STATUS_FLIP;
      int ret;
      ret = readoutConn->send ((char *) &header, sizeof (imghdr));
      if (ret < 0)
	return 100;		// TODO some better timeout handling
      return 0;
    }
  return -1;
}

int
CameraChip::readoutOneLine ()
{
  return -1;
}

Rts2DevCamera::Rts2DevCamera (int argc, char **argv):Rts2Device (argc, argv, DEVICE_TYPE_CCD, 5554,
	    "C0")
{
  int
    i;
  char *
  states_names[MAX_CHIPS] = { "img_chip", "trc_chip" };
  for (i = 0; i < MAX_CHIPS; i++)
    chips[i] = NULL;
  setStateNames (MAX_CHIPS, states_names);
}

int
Rts2DevCamera::init ()
{
  int ret;
  return Rts2Device::init ();
}

int
Rts2DevCamera::addConnection (int in_sock)
{
  int i;
  for (i = 0; i < MAX_CONN; i++)
    {
      if (!connections[i])
	{
	  printf ("add conn: %i\n", i);
	  connections[i] = new Rts2DevConnCamera (in_sock, this);
	  return 0;
	}
    }
  return -1;
}

int
Rts2DevCamera::checkExposures ()
{
  int ret;
  for (int i = 0; i < chipNum; i++)
    {
      // try to end exposure
      ret = camWaitExpose (i);
      if (ret >= 0)
	      setTimeout (ret);
      if (ret == -2)
      maskState (i, CAM_MASK_EXPOSE | CAM_MASK_DATA,
		 CAM_NOEXPOSURE | CAM_DATA, "exposure chip finished");
    }
}

int
Rts2DevCamera::checkReadouts ()
{
  int ret;
  for (int i = 0; i < chipNum; i++)
    {
      ret = chips[i]->readoutOneLine ();
      if (ret >= 0)
	{
	  setTimeout (ret);
	}
      if (ret == -2)
	{
	  setTimeout (1000000);
	  maskState (i, CAM_MASK_READING, CAM_NOTREADING,
		     "chip readout ended");
	}
    }
}

int
Rts2DevCamera::idle ()
{
  checkExposures ();
  checkReadouts ();
}

int
Rts2DevCamera::camReady (Rts2Conn * conn)
{
  int ret;
  ret = camReady ();
  if (ret)
    {
      conn->sendCommandEnd (DEVDEM_E_HW, "camera not ready");
      return -1;
    }
  return 0;
}

int
Rts2DevCamera::camInfo (Rts2Conn * conn)
{
  int ret;
  ret = camInfo ();
  if (ret)
    {
      conn->sendCommandEnd (DEVDEM_E_HW, "camera not ready");
      return -1;
    }
  conn->sendValue ("temperature_regulation", tempRegulation);
  conn->sendValue ("temperature_setpoint", tempSet);
  conn->sendValue ("air_temperature", tempAir);
  conn->sendValue ("ccd_temperature", tempCCD);
  conn->sendValue ("cooling_power", coolingPower);
  conn->sendValue ("fan", fan);
  conn->sendValue ("filter", filter);
  return 0;
}

int
Rts2DevCamera::camBaseInfo (Rts2Conn * conn)
{
  int ret;
  ret = camBaseInfo ();
  if (ret)
    {
      conn->sendCommandEnd (DEVDEM_E_HW, "camera not ready");
      return -1;
    }
  conn->sendValue ("type", ccdType);
  conn->sendValue ("serial", serialNumber);
  conn->sendValue ("chips", chipNum);
  conn->sendValue ("can_df", canDF);
  return 0;
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
Rts2DevCamera::camExpose (Rts2Conn * conn, int chip, int light, float exptime)
{
  int ret;
  struct timeval tv;

  maskState (chip, CAM_MASK_EXPOSE | CAM_MASK_DATA, CAM_EXPOSING | CAM_NODATA,
	     "exposure chip started");
  ret = camExpose (chip, light, exptime);
  if (!ret)
    {
      chips[chip]->setExposure (exptime);
      return 0;
    }
  maskState (chip, CAM_MASK_EXPOSE, CAM_NOEXPOSURE, "exposure error");
  conn->sendCommandEnd (DEVDEM_E_HW, "cannot exposure on chip");
  return -1;
}

int
Rts2DevCamera::camWaitExpose (int chip)
{
  if (chips[chip]->checkExposure () == 0)
    {
      return -2;
    }
  return -1;
}

int
Rts2DevCamera::camStopExpose (Rts2Conn * conn, int chip)
{
  if (chips[chip]->checkExposure () >= 0)
    {
      maskState (chip, CAM_MASK_EXPOSE, CAM_NOEXPOSURE, "exposure canceled");
      chips[chip]->endExposure ();
      return camStopExpose (chip);
    }
}

int
Rts2DevCamera::camBox (Rts2Conn * conn, int chip, int x, int y, int width,
		       int height)
{

}

int
Rts2DevCamera::camReadout (Rts2Conn * conn, int chip)
{
  int ret;
  int i;
  // open data connection - wait socket

  Rts2DevConnData *data_conn;
  data_conn = new Rts2DevConnData (this, conn);

  ret = data_conn->init ();
  // add data connection
  for (i = 0; i < MAX_CONN; i++)
    {
      if (!connections[i])
	{
	  printf ("add data conn: data_conn\n");
	  connections[i] = data_conn;
	  break;
	}
    }

  if (i == MAX_CONN)
    {
      delete data_conn;
      conn->sendCommandEnd (DEVDEM_E_SYSTEM,
			    "cannot create data connection for readout");
      return -1;
    }

  struct sockaddr_in our_addr;

  if (conn->getName (&our_addr) < 0)
    {
      delete data_conn;
      conn->sendCommandEnd (DEVDEM_E_SYSTEM, "cannot get our address");
      return -1;
    }

  data_conn->setAddress (&our_addr.sin_addr);

  maskState (chip, CAM_MASK_READING | CAM_MASK_DATA, CAM_READING | CAM_NODATA,
	     "chip readout started");
  ret = chips[chip]->startReadout (data_conn, conn);
  if (!ret)
    {
      return 0;
    }
  maskState (chip, CAM_MASK_READING, CAM_NOTREADING, "chip readout failed");
  conn->sendCommandEnd (DEVDEM_E_HW, "cannot read chip");
  return -1;
}

int
Rts2DevCamera::camBinning (Rts2Conn * conn, int chip, int x_bin, int y_bin)
{

}

int
Rts2DevCamera::camStopRead (Rts2Conn * conn, int chip)
{

}

int
Rts2DevCamera::camCoolMax (Rts2Conn * conn)
{
  return camCoolMax ();
}

int
Rts2DevCamera::camCoolHold (Rts2Conn * conn)
{
  return camCoolHold ();
}

int
Rts2DevCamera::camCoolTemp (Rts2Conn * conn, float new_temp)
{
  return camCoolTemp (new_temp);
}

int
Rts2DevCamera::camCoolShutdown (Rts2Conn * conn)
{
  return camCoolShutdown ();
}

int
Rts2DevCamera::camFilter (Rts2Conn * conn, int new_filter)
{

}

Rts2DevConnCamera::Rts2DevConnCamera (int in_sock,
				      Rts2DevCamera * in_master_device):
Rts2DevConn (in_sock, in_master_device)
{
  master = in_master_device;
}

int
Rts2DevConnCamera::paramNextChip (int *in_chip)
{
  int ret;
  ret = paramNextInteger (in_chip);
  if (ret)
    return ret;
  if (*in_chip < 0 || *in_chip >= MAX_CHIPS)
    {
      return -1;
    }
  return 0;
}

int
Rts2DevConnCamera::commandAuthorized ()
{
  int chip;

  if (isCommand ("ready"))
    {
      return master->camReady (this);
    }
  else if (isCommand ("info"))
    {
      return master->camInfo (this);
    }
  else if (isCommand ("base_info"))
    {
      return master->camBaseInfo (this);
    }
  else if (isCommand ("chipinfo"))
    {
      if (paramNextChip (&chip) || !paramEnd ())
	return -2;
      return master->camChipInfo (this, chip);
    }
  else if (isCommand ("expose"))
    {
      float exptime;
      int light;
      if (paramNextChip (&chip)
	  || paramNextInteger (&light)
	  || paramNextFloat (&exptime) || !paramEnd ())
	return -2;
      return master->camExpose (this, chip, light, exptime);
    }
  else if (isCommand ("stopexpo"))
    {
      if (paramNextChip (&chip) || !paramEnd ())
	return -2;
      return master->camStopExpose (this, chip);
    }
  else if (isCommand ("box"))
    {
      int x, y, w, h;
      if (paramNextChip (&chip)
	  || paramNextInteger (&x)
	  || paramNextInteger (&y)
	  || paramNextInteger (&w) || paramNextInteger (&h) || !paramEnd ())
	return -2;
      return master->camBox (this, chip, x, y, w, h);
    }
  else if (isCommand ("readout"))
    {
      if (paramNextChip (&chip) || !paramEnd ())
	return -2;
      return master->camReadout (this, chip);
    }
  else if (isCommand ("binning"))
    {
      int vertical, horizontal;
      if (paramNextChip (&chip)
	  || paramNextInteger (&vertical)
	  || paramNextInteger (&horizontal) || !paramEnd ())
	return -2;
      return master->camBinning (this, chip, vertical, horizontal);
    }
  else if (isCommand ("stopread"))
    {
      if (paramNextChip (&chip) || !paramEnd ())
	return -2;
      return master->camStopRead (this, chip);
    }
  else if (isCommand ("coolmax"))
    {
      return master->camCoolMax (this);
    }
  else if (isCommand ("coolhold"))
    {
      return master->camCoolHold (this);
    }
  else if (isCommand ("cooltemp"))
    {
      float new_temp;
      if (paramNextFloat (&new_temp) || !paramEnd ())
	return -2;
      return master->camCoolTemp (this, new_temp);
    }
  else if (isCommand ("filter"))
    {
      int new_filter;
      if (paramNextInteger (&new_filter) || !paramEnd ())
	return -2;
      return master->camFilter (this, new_filter);
    }
  else if (isCommand ("exit"))
    {
      close (sock);
      return -1;
    }
  else if (isCommand ("help"))
    {
      send ("ready - is camera ready?");
      send ("info - information about camera");
      send ("chipinfo <chip> - information about chip");
      send ("expose <chip> <exposure> - start exposition on given chip");
      send ("stopexpo <chip> - stop exposition on given chip");
      send ("progexpo <chip> - query exposition progress");
      send ("readout <chip> - start reading given chip");
      send ("focus <chip> - try to focus image");
      send ("mirror <open|close> - open/close mirror");
      send
	("binning <chip> <binning_id> - set new binning; actual from next readout on");
      send ("stopread <chip> - stop reading given chip");
      send ("cooltemp <temp> - cooling temperature");
      send ("focus <steps> - change focus to the given steps");
      send ("autofocus - try to autofocus picture");
      send ("filter <filter number> - set camera filter");
      send ("exit - exit from connection");
      send ("help - print, what you are reading just now");
      return 0;
    }
  sendCommandEnd (DEVDEM_E_COMMAND, "camd unknow command");
  return -1;
}
