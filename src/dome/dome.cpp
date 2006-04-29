#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <math.h>

#include "status.h"

#include "dome.h"

Rts2DevDome::Rts2DevDome (int in_argc, char **in_argv, int in_device_type):
Rts2Device (in_argc, in_argv, in_device_type, "DOME")
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
  windspeed = nan ("f");	// as soon as we get update from meteo, we will solve it. We have rain now, so dome will remain closed at start

  maxWindSpeed = 50;
  maxPeekWindspeed = 50;
  weatherCanOpenDome = false;
  ignoreMeteo = false;


  addOption ('W', "max_windspeed", 1, "maximal allowed windspeed (in km/h)");
  addOption ('P', "max_peek_windspeed", 1,
	     "maximal allowed windspeed (in km/h");
  addOption ('O', "weather_can_open", 0,
	     "specified that option if weather signal is allowed to open dome");
  addOption ('I', "ignore_meteo", 0, "whenever to ignore meteo station");

  observingPossible = 0;

  time (&nextGoodWeather);

  nextGoodWeather += DEF_WEATHER_TIMEOUT;
}

int
Rts2DevDome::processOption (int in_opt)
{
  switch (in_opt)
    {
    case 'W':
      maxWindSpeed = atoi (optarg);
      break;
    case 'P':
      maxPeekWindspeed = atoi (optarg);
      break;
    case 'O':
      weatherCanOpenDome = true;
      break;
    case 'I':
      ignoreMeteo = true;
      break;
    default:
      return Rts2Device::processOption (in_opt);
    }
  return 0;
}

void
Rts2DevDome::domeWeatherGood ()
{
  if (weatherCanOpenDome)
    {
      sendMaster ("on");
    }
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
  // if we are back in idle state..beware of copula state (bit non-structural, but I 
  // cannot find better solution)
  if ((getState (0) & DOME_COP_MASK_MOVE) == DOME_COP_NOT_MOVE)
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
  conn->sendValue ("ignoreMeteo", (ignoreMeteo ? 2 : 1));
  return 0;
}

int
Rts2DevDome::sendBaseInfo (Rts2Conn * conn)
{
  conn->sendValue ("type", domeModel);
  return 0;
}

int
Rts2DevDome::closeDomeWeather ()
{
  int ret;
  if (ignoreMeteo == false)
    {
      ret = closeDome ();
      setMasterStandby ();
      return ret;
    }
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

void
Rts2DevDome::setWeatherTimeout (time_t wait_time)
{
  time_t next;
  time (&next);
  next += wait_time;
  if (next > nextGoodWeather)
    nextGoodWeather = next;
}

int
Rts2DevConnDome::commandAuthorized ()
{
  if (isCommand ("open"))
    {
      return master->openDome ();
    }
  else if (isCommand ("close"))
    {
      return master->closeDome ();
    }
  else if (isCommand ("ignore"))
    {
      char *ignore;
      bool newIgnore = false;
      if (paramNextString (&ignore) || !paramEnd ())
	return -2;
      if (!strcasecmp (ignore, "on"))
	{
	  newIgnore = true;
	}
      return master->setIgnoreMeteo (newIgnore);
    }
  return Rts2DevConn::commandAuthorized ();
}
