#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <mcheck.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>

#include "../utils/rts2device.h"
#include "../utils/rts2block.h"

#include "telescope.h"

Rts2DevTelescope::Rts2DevTelescope (int argc, char **argv):
Rts2Device (argc, argv, DEVICE_TYPE_MOUNT, 5553, "T0")
{
  char *states_names[1] = { "telescope" };
  setStateNames (1, states_names);
  telRa = nan ("f");
  telDec = nan ("f");
  telSiderealTime = nan ("f");
  telLocalTime = nan ("f");
  telAxis[0] = telAxis[1] = nan ("f");
  telLongtitude = nan ("f");
  telLatitude = nan ("f");
  telAltitude = nan ("f");
  telParkDec = nan ("f");
}

int
Rts2DevTelescope::init ()
{
  return Rts2Device::init ();
}

Rts2Conn *
Rts2DevTelescope::createConnection (int in_sock, int conn_num)
{
  return new Rts2DevConnTelescope (in_sock, this);
}

int
Rts2DevTelescope::checkMoves ()
{
  if ((getState (0) & TEL_MASK_MOVING) != TEL_OBSERVING)
    {
      int ret;
      ret = isMoving ();
      if (ret >= 0)
	setTimeout (ret);
      if (ret == -1)
	{
	  maskState (0, TEL_MASK_MOVING, TEL_OBSERVING,
		     "move finished with error");
	}
      if (ret == -2)
	{
	  if (endMove ())
	    maskState (0, TEL_MASK_MOVING, TEL_OBSERVING,
		       "move finished with error");
	  else
	    maskState (0, TEL_MASK_MOVING, TEL_OBSERVING,
		       "move finished without error");
	}
    }
  return 0;
}

int
Rts2DevTelescope::idle ()
{
  checkMoves ();
}

int
Rts2DevTelescope::ready (Rts2Conn * conn)
{
  int ret;
  ret = ready ();
  if (ret)
    {
      conn->sendCommandEnd (DEVDEM_E_HW, "telescope not ready");
      return -1;
    }
  return 0;
}

int
Rts2DevTelescope::info (Rts2Conn * conn)
{
  int ret;
  ret = info ();
  if (ret)
    {
      conn->sendCommandEnd (DEVDEM_E_HW, "telescope not ready");
      return -1;
    }
  conn->sendValue ("ra", telRa);
  conn->sendValue ("dec", telDec);
  conn->sendValue ("siderealtime", telSiderealTime);
  conn->sendValue ("localtime", telLocalTime);
  conn->sendValue ("flip", telFlip);
  conn->sendValue ("axis0_counts", telAxis[0]);
  conn->sendValue ("axis1_counts", telAxis[1]);
  return 0;
}

int
Rts2DevTelescope::baseInfo (Rts2Conn * conn)
{
  int ret;
  ret = baseInfo ();
  if (ret)
    {
      conn->sendCommandEnd (DEVDEM_E_HW, "telescope not ready");
      return -1;
    }
  conn->sendValue ("type", telType);
  conn->sendValue ("serial", telSerialNumber);
  conn->sendValue ("longtitude", telLongtitude);
  conn->sendValue ("latitude", telLatitude);
  conn->sendValue ("altitude", telAltitude);
  conn->sendValue ("park_dec", telParkDec);
  return 0;
}

int
Rts2DevTelescope::startMove (Rts2Conn * conn, double tar_ra, double tar_dec)
{
  int ret;
  ret = startMove (tar_ra, tar_dec);
  if (ret)
    conn->sendCommandEnd (DEVDEM_E_HW, "cannot perform move op");
  else
    maskState (0, TEL_MASK_MOVING, TEL_MOVING, "move started");
  return ret;
}

int
Rts2DevTelescope::setTo (Rts2Conn * conn, double set_ra, double set_dec)
{
  int ret;
  ret = setTo (set_ra, set_dec);
  if (ret)
    conn->sendCommandEnd (DEVDEM_E_HW, "cannot set to");
  return ret;
}

int
Rts2DevTelescope::correct (Rts2Conn * conn, double cor_ra, double cor_dec)
{
  int ret;
  ret = correct (cor_ra, cor_dec);
  if (ret)
    conn->sendCommandEnd (DEVDEM_E_HW, "cannot perform correction");
  return ret;
}

int
Rts2DevTelescope::park (Rts2Conn * conn)
{
  int ret;
  ret = park ();
  if (ret)
    conn->sendCommandEnd (DEVDEM_E_HW, "cannot park");
  else
    maskState (0, TEL_MASK_MOVING, TEL_MOVING, "parking started");
  return ret;
}

int
Rts2DevTelescope::change (Rts2Conn * conn, double chng_ra, double chng_dec)
{
  int ret;
  ret = change (chng_ra, chng_dec);
  if (ret)
    {
      conn->sendCommandEnd (DEVDEM_E_HW, "cannot change");
    }
  return ret;
}

Rts2DevConnTelescope::Rts2DevConnTelescope (int in_sock, Rts2DevTelescope * in_master_device):
Rts2DevConn (in_sock, in_master_device)
{
  master = in_master_device;
}

int
Rts2DevConnTelescope::commandAuthorized ()
{
  if (isCommand ("ready"))
    {
      return master->ready (this);
    }
  else if (isCommand ("info"))
    {
      return master->info (this);
    }
  else if (isCommand ("base_info"))
    {
      return master->baseInfo (this);
    }
  else if (isCommand ("exit"))
    {
      close (sock);
      return -1;
    }
  else if (isCommand ("help"))
    {
      send ("ready - is telescope ready?");
      send ("info - information about telescope");
      send ("exit - exit from connection");
      send ("help - print, what you are reading just now");
      return 0;
    }
  else if (isCommand ("move"))
    {
      double tar_ra;
      double tar_dec;
      CHECK_PRIORITY;
      if (paramNextDouble (&tar_ra) || paramNextDouble (&tar_dec)
	  || !paramEnd ())
	return -2;
      return master->startMove (this, tar_ra, tar_dec);
    }
  else if (isCommand ("correct"))
    {
      double set_ra;
      double set_dec;
      CHECK_PRIORITY;
      if (paramNextDouble (&set_ra) || paramNextDouble (&set_dec)
	  || !paramEnd ())
	return -2;
      return master->setTo (this, set_ra, set_dec);
    }
  else if (isCommand ("correct"))
    {
      double cor_ra;
      double cor_dec;
      CHECK_PRIORITY;
      if (paramNextDouble (&cor_ra) || paramNextDouble (&cor_dec)
	  || !paramEnd ())
	return -2;
      return master->correct (this, cor_ra, cor_dec);
    }
  else if (isCommand ("park"))
    {
      CHECK_PRIORITY;
      if (!paramEnd ())
	return -2;
      return master->park (this);
    }
  else if (isCommand ("change"))
    {
      double chng_ra;
      double chng_dec;
      CHECK_PRIORITY;
      if (paramNextDouble (&chng_ra) || paramNextDouble (&chng_dec)
	  || !paramEnd ())
	return -2;
      return master->change (this, chng_ra, chng_dec);
    }
  return Rts2DevConn::commandAuthorized ();
}
