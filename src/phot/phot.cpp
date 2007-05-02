/*! 
 * @file Photometer deamon. 
 *
 * @author petr
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "phot.h"
#include "../utils/rts2device.h"
#include "kernel/phot.h"
#include "status.h"

#include <fcntl.h>
#include <errno.h>
#include <sys/io.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <syslog.h>
#include <time.h>

Rts2DevPhot::Rts2DevPhot (int in_argc, char **in_argv):
Rts2Device (in_argc, in_argv, DEVICE_TYPE_PHOT, "PHOT")
{
  createValue (filter, "filter", "used filter", false);

  photType = NULL;
  serial = NULL;

  req_count = -1;
  setReqTime (1);
}

void
Rts2DevPhot::checkFilterMove ()
{
  long ret;
  if ((getState () & PHOT_MASK_FILTER) == PHOT_FILTER_MOVE)
    {
      ret = isFilterMoving ();
      if (ret > 0)
	{
	  setTimeoutMin (ret);
	  return;
	}
      // when it's -1 or -2..end filter move
      endFilterMove ();
    }
}

int
Rts2DevPhot::initValues ()
{
  addConstValue ("type", photType);
  addConstValue ("serial", serial);

  return Rts2Device::initValues ();
}

int
Rts2DevPhot::idle ()
{
  long ret;
  struct timeval now;
  gettimeofday (&now, NULL);
  if (now.tv_sec > nextCountDue.tv_sec
      || (now.tv_sec == nextCountDue.tv_sec
	  && now.tv_usec > nextCountDue.tv_usec))
    {
      ret = getCount ();
      if (ret >= 0)
	{
	  setTimeout (ret);
	  nextCountDue.tv_sec = now.tv_sec + ret / USEC_SEC;
	  nextCountDue.tv_usec = now.tv_usec + ret % USEC_SEC;
	  if (nextCountDue.tv_usec >= USEC_SEC)
	    {
	      nextCountDue.tv_sec += nextCountDue.tv_usec / USEC_SEC;
	      nextCountDue.tv_usec %= USEC_SEC;
	    }
	}
      if (ret < 0)
	{
	  endIntegrate ();
	}
    }
  else
    {
      setTimeout ((nextCountDue.tv_sec - now.tv_sec) * USEC_SEC +
		  nextCountDue.tv_usec - now.tv_usec);
    }
  // check filter moving..
  checkFilterMove ();
  return Rts2Device::idle ();
}

int
Rts2DevPhot::homeFilter ()
{
  return -1;
}

int
Rts2DevPhot::startFilterMove (int new_filter)
{
  maskState (PHOT_MASK_FILTER, PHOT_FILTER_MOVE);
  return 0;
}

long
Rts2DevPhot::isFilterMoving ()
{
  return -2;
}

int
Rts2DevPhot::endFilterMove ()
{
  infoAll ();
  maskState (PHOT_MASK_FILTER, PHOT_FILTER_IDLE);
  return 0;
}

int
Rts2DevPhot::startIntegrate ()
{
  return -1;
}

int
Rts2DevPhot::startIntegrate (Rts2Conn * conn, float in_req_time,
			     int in_req_count)
{
  int ret;
  req_count = in_req_count;
  setReqTime (in_req_time);
  ret = startIntegrate ();
  if (ret)
    {
      conn->sendCommandEnd (DEVDEM_E_HW, "cannot start integration");
      return -1;
    }
  maskState (PHOT_MASK_INTEGRATE, PHOT_INTEGRATE, "integration started");
  return 0;
}

int
Rts2DevPhot::endIntegrate ()
{
  maskState (PHOT_MASK_INTEGRATE, PHOT_NOINTEGRATE, "integration finished");
  // keep us update in old time
  startIntegrate ();
  req_count = -1;
  return 0;
}

int
Rts2DevPhot::stopIntegrate ()
{
  maskState (PHOT_MASK_INTEGRATE, PHOT_NOINTEGRATE,
	     "Integration interrupted");
  startIntegrate ();
  return 0;
}

int
Rts2DevPhot::homeFilter (Rts2Conn * conn)
{
  int ret;
  ret = homeFilter ();
  if (ret)
    return -1;
  filter->setValueInteger (0);
  infoAll ();
  return ret;
}

int
Rts2DevPhot::enableMove ()
{
  return -1;
}

int
Rts2DevPhot::disableMove ()
{
  return -1;
}

int
Rts2DevPhot::moveFilter (Rts2Conn * conn, int new_filter)
{
  int ret;
  ret = startFilterMove (new_filter);
  if (ret)
    return -1;
  return 0;
}

int
Rts2DevPhot::enableFilter (Rts2Conn * conn)
{
  int ret;
  ret = enableMove ();
  if (ret)
    return -1;
  infoAll ();
  return 0;
}

void
Rts2DevPhot::cancelPriorityOperations ()
{
  stopIntegrate ();
  clearStatesPriority ();
  Rts2Device::cancelPriorityOperations ();
}

int
Rts2DevPhot::changeMasterState (int new_state)
{
  switch (new_state & SERVERD_STATUS_MASK)
    {
    case SERVERD_NIGHT:
      enableMove ();
      break;
    default:			/* other - SERVERD_DAY, SERVERD_DUSK, SERVERD_MAINTANCE, SERVERD_OFF, etc */
      disableMove ();
      break;
    }
  return 0;
}

void
Rts2DevPhot::setReqTime (float in_req_time)
{
  req_time = in_req_time;
  gettimeofday (&nextCountDue, NULL);
  nextCountDue.tv_sec += (long) floor (in_req_time);
  nextCountDue.tv_usec +=
    (long) ((in_req_time - floor (in_req_time)) * USEC_SEC);
  if (nextCountDue.tv_usec >= USEC_SEC)
    {
      nextCountDue.tv_sec += nextCountDue.tv_usec / USEC_SEC;
      nextCountDue.tv_usec %= USEC_SEC;
    }
}

int
Rts2DevPhot::sendCount (int count, float exp, int is_ov)
{
  char *msg;
  int ret;
  asprintf (&msg, "%i %f %i", count, exp, is_ov);
  sendValueRawAll ("count", msg);
  if (req_count > 0)
    req_count--;
  if (req_count == 0)
    endIntegrate ();
  free (msg);
  return ret;
}

int
Rts2DevPhot::commandAuthorized (Rts2Conn * conn)
{
  int ret;
  if (conn->isCommand ("home"))
    {
      return homeFilter ();
    }
  else if (conn->isCommand ("integrate"))
    {
      float new_req_time;
      int new_req_count;
      if (conn->paramNextFloat (&new_req_time)
	  || conn->paramNextInteger (&new_req_count) || !conn->paramEnd ())
	return -2;

      return startIntegrate (conn, new_req_time, new_req_count);
    }

  else if (conn->isCommand ("intfil"))
    {
      int new_filter;
      float new_req_time;
      int new_req_count;
      if (conn->paramNextInteger (&new_filter)
	  || conn->paramNextFloat (&new_req_time)
	  || conn->paramNextInteger (&new_req_count) || !conn->paramEnd ())
	return -2;

      ret = moveFilter (conn, new_filter);
      if (ret)
	return ret;
      return startIntegrate (conn, new_req_time, new_req_count);
    }

  else if (conn->isCommand ("stop"))
    {
      return stopIntegrate ();
    }

  else if (conn->isCommand ("filter"))
    {
      int new_filter;
      if (conn->paramNextInteger (&new_filter) || !conn->paramEnd ())
	return -2;
      return moveFilter (conn, new_filter);
    }
  else if (conn->isCommand ("enable"))
    {
      if (!conn->paramEnd ())
	return -2;
      return enableFilter (conn);
    }
  else if (conn->isCommand ("help"))
    {
      conn->send ("info - phot informations");
      conn->send ("exit - exit from main loop");
      conn->send ("help - print, what you are reading just now");
      conn->send ("integrate <time> <count> - start integration");
      conn->send ("enable - enable filter movements");
      conn->send ("stop - stop any running integration");
      return 0;
    }
  return Rts2Device::commandAuthorized (conn);
}
