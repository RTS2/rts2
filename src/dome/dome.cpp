#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "status.h"

#include "dome.h"

Rts2DevDome::Rts2DevDome (int argc, char **argv):
Rts2Device (argc, argv, DEVICE_TYPE_DOME, 5552, "DOME")
{
  char *states_names[1] = { "dome" };
  setStateNames (1, states_names);
}

int
Rts2DevDome::init ()
{
  return Rts2Device::init ();
}

Rts2Conn *
Rts2DevDome::createConnection (int in_sock, int conn_num)
{
  return new Rts2DevConnDome (in_sock, this);
}

int
Rts2DevDome::checkOpening ()
{
  syslog (LOG_DEBUG, "dome opening: %i %i", getState (0),
	  (getState (0) & DOME_DOME_MASK) == DOME_OPENING);
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
	  maskState (0, DOME_DOME_MASK, DOME_OPENED,
		     "opening finished with error");
	}
      if (ret == -2)
	{
	  if (endOpen ())
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
	  maskState (0, DOME_DOME_MASK, DOME_CLOSED,
		     "closing finished with error");
	}
      if (ret == -2)
	{
	  if (endOpen ())
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
  setTimeout (1000000);
  return 0;
}

int
Rts2DevDome::idle ()
{
  checkOpening ();
}

int
Rts2DevDome::ready (Rts2Conn * conn)
{
  return ready ();
}

int
Rts2DevDome::info (Rts2Conn * conn)
{
  return info ();
}

int
Rts2DevDome::baseInfo (Rts2Conn * conn)
{
  return baseInfo ();
}

int
Rts2DevDome::observing ()
{
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
Rts2DevDome::setMasterState (int new_state)
{
  if (new_state & SERVERD_STANDBY_MASK == SERVERD_STANDBY)
    {
      switch (new_state & SERVERD_STATUS_MASK)
	{
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
