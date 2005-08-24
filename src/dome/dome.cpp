#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <math.h>

#include "status.h"

#include "dome.h"

Rts2DevDome::Rts2DevDome (int argc, char **argv):
Rts2Device (argc, argv, DEVICE_TYPE_DOME, 5552, "DOME")
{
  char *states_names[1] = { "dome" };
  setStateNames (1, states_names);

  sw_state = -1;
  temperature = nan ("f");
  humidity = nan ("f");
  power_telescope = 0;
  power_cameras = 0;
  nextOpen = -1;
  rain = 1;
  windspeed = nan ("f");

  observingPossible = 0;
}

int
Rts2DevDome::init ()
{
  return Rts2Device::init ();
}

Rts2DevConn *
Rts2DevDome::createConnection (int in_sock, int conn_num)
{
  return new Rts2DevConnDome (in_sock, this);
}

int
Rts2DevDome::checkOpening ()
{
  if ((getState (0) & DOME_DOME_MASK) == DOME_OPENING)
    {
      long ret;
      ret = isOpened ();
      syslog (LOG_DEBUG, "isOPenede ret:%li", ret);
      if (ret >= 0)
	{
	  setTimeout (ret);
	  return 0;
	}
      if (ret == -1)
	{
	  endOpen ();
	  infoAll ();
	  maskState (0, DOME_DOME_MASK, DOME_OPENED,
		     "opening finished with error");
	}
      if (ret == -2)
	{
	  ret = endOpen ();
	  infoAll ();
	  if (ret)
	    {
	      maskState (0, DOME_DOME_MASK, DOME_OPENED,
			 "dome opened with error");
	    }
	  else
	    {
	      maskState (0, DOME_DOME_MASK, DOME_OPENED, "dome opened");
	    }
	}
    }
  else if ((getState (0) & DOME_DOME_MASK) == DOME_CLOSING)
    {
      long ret;
      ret = isClosed ();
      if (ret >= 0)
	{
	  setTimeout (ret);
	  return 0;
	}
      if (ret == -1)
	{
	  endClose ();
	  infoAll ();
	  maskState (0, DOME_DOME_MASK, DOME_CLOSED,
		     "closing finished with error");
	}
      if (ret == -2)
	{
	  ret = endClose ();
	  infoAll ();
	  if (ret)
	    {
	      maskState (0, DOME_DOME_MASK, DOME_CLOSED,
			 "dome closed with error");
	    }
	  else
	    {
	      maskState (0, DOME_DOME_MASK, DOME_CLOSED, "dome closed");
	    }
	}
    }
  setTimeout (10 * USEC_SEC);
  return 0;
}

int
Rts2DevDome::idle ()
{
  checkOpening ();
  return Rts2Device::idle ();
}

int
Rts2DevDome::ready (Rts2Conn * conn)
{
  int ret;
  ret = ready ();
  if (ret)
    {
      conn->sendCommandEnd (DEVDEM_E_HW, "dome not ready");
      return -1;
    }
  return 0;
}

int
Rts2DevDome::sendInfo (Rts2Conn * conn)
{
  conn->sendValue ("dome", sw_state);
  conn->sendValue ("temperature", temperature);
  conn->sendValue ("humidity", humidity);
  conn->sendValue ("power_telescope", power_telescope);
  conn->sendValue ("power_cameras", power_cameras);
  conn->sendValueTime ("next_open", &nextOpen);
  conn->sendValue ("rain", rain);
  conn->sendValue ("windspeed", windspeed);
  conn->sendValue ("observingPossible", observingPossible);
  return 0;
}

int
Rts2DevDome::sendBaseInfo (Rts2Conn * conn)
{
  conn->sendValue ("type", domeModel);
  return 0;
}

int
Rts2DevDome::observing ()
{
  observingPossible = 1;
  if ((getState (0) & DOME_DOME_MASK) != DOME_OPENED)
    return openDome ();
  return 0;
}

int
Rts2DevDome::standby ()
{
  if ((getState (0) & DOME_DOME_MASK) != DOME_CLOSED)
    return closeDome ();
  return 0;
}

int
Rts2DevDome::off ()
{
  if ((getState (0) & DOME_DOME_MASK) != DOME_CLOSED)
    return closeDome ();
  return 0;
}

int
Rts2DevDome::setMasterStandby ()
{
  int serverState;
  serverState = getMasterState ();
  if ((serverState != SERVERD_OFF)
      && ((getMasterState () & SERVERD_STANDBY_MASK) != SERVERD_STANDBY))
    {
      return sendMaster ("standby");
    }
  return 0;
}

int
Rts2DevDome::setMasterOn ()
{
  int serverState;
  serverState = getMasterState ();
  if ((serverState != SERVERD_OFF)
      && ((getMasterState () & SERVERD_STANDBY_MASK) == SERVERD_STANDBY))
    {
      return sendMaster ("on");
    }
  return 0;
}

int
Rts2DevDome::changeMasterState (int new_state)
{
  observingPossible = 0;
  if ((new_state & SERVERD_STANDBY_MASK) == SERVERD_STANDBY)
    {
      switch (new_state & SERVERD_STATUS_MASK)
	{
	case SERVERD_EVENING:
	case SERVERD_DUSK:
	case SERVERD_NIGHT:
	case SERVERD_DAWN:
	  return standby ();
	default:
	  return off ();
	}
    }
  switch (new_state)
    {
    case SERVERD_DUSK:
    case SERVERD_NIGHT:
    case SERVERD_DAWN:
      return observing ();
    case SERVERD_EVENING:
    case SERVERD_MORNING:
      return standby ();
    default:
      return off ();
    }
}

int
Rts2DevConnDome::commandAuthorized ()
{
  if (isCommand ("open"))
    {
      CHECK_PRIORITY;
      return master->openDome ();
    }
  else if (isCommand ("close"))
    {
      CHECK_PRIORITY;
      return master->closeDome ();
    }
  return Rts2DevConn::commandAuthorized ();
}
