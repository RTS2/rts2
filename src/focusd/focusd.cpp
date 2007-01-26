/*!
 * $Id$
 *
 * @author standa
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>

#include "../utils/rts2device.h"
#include "../utils/rts2block.h"

#include "focuser.h"

Rts2DevFocuser::Rts2DevFocuser (int in_argc, char **in_argv):
Rts2Device (in_argc, in_argv, DEVICE_TYPE_FOCUS, "F0")
{
  focCamera[0] = '\0';
  focStepSec = 100;
  homePos = 750;
  startPosition = INT_MIN;

  focSwitches = new Rts2ValueInteger ("switches");
  addValue (focSwitches);

  focTemp = new Rts2ValueFloat ("FOC_TEMP", "focuser temperature");
  addValue (focTemp);

  switchNum = 0;		// zero switches

  focPos = new Rts2ValueInteger ("FOC_POS", "focuser position");
  addValue (focPos);

  addOption ('x', "camera_name", 1, "associated camera name (ussualy B0x)");
  addOption ('o', "home", 1, "home position (default to 750!)");
  addOption ('p', "start_position", 1,
	     "focuser start position (focuser will be set to this one, if initial position is detected");
}

int
Rts2DevFocuser::processOption (int in_opt)
{
  switch (in_opt)
    {
    case 'x':
      strncpy (focCamera, optarg, 20);
      focCamera[19] = '\0';
      break;
    case 'o':
      homePos = atoi (optarg);
      break;
    case 'p':
      startPosition = atoi (optarg);
      break;
    default:
      return Rts2Device::processOption (in_opt);
    }
  return 0;
}

Rts2DevConn *
Rts2DevFocuser::createConnection (int in_sock)
{
  return new Rts2DevConnFocuser (in_sock, this);
}

void
Rts2DevFocuser::checkState ()
{
  if ((getState () & FOC_MASK_FOCUSING) == FOC_FOCUSING)
    {
      int ret;
      ret = isFocusing ();

      if (ret >= 0)
	setTimeout (ret);
      else
	{
	  ret = endFocusing ();
	  infoAll ();
	  setTimeout (USEC_SEC);
	  if (ret)
	    maskState (DEVICE_ERROR_MASK | FOC_MASK_FOCUSING,
		       DEVICE_ERROR_HW | FOC_SLEEPING,
		       "focusing finished with error");
	  else
	    maskState (FOC_MASK_FOCUSING, FOC_SLEEPING,
		       "focusing finished without errror");
	}
    }
}

int
Rts2DevFocuser::initValues ()
{
  addConstValue ("FOC_TYPE", "focuser type", focType);
  addConstValue ("camera", focCamera);
  addConstValue ("switch_num", switchNum);

  return Rts2Device::initValues ();
}

int
Rts2DevFocuser::idle ()
{
  checkState ();
  return Rts2Device::idle ();
}

int
Rts2DevFocuser::ready (Rts2Conn * conn)
{
  int ret;

  ret = ready ();
  if (ret)
    {
      conn->sendCommandEnd (DEVDEM_E_HW, "focuser not ready");
      return -1;
    }
  return 0;
}

int
Rts2DevFocuser::setTo (int num)
{
  int ret;
  int steps;
  ret = info ();
  if (ret)
    return ret;

  focPositionNew = num;
  steps = num - getFocPos ();
  setFocusTimeout ((int) ceil (steps / focStepSec) + 5);

  return stepOut (steps);
}

int
Rts2DevFocuser::home ()
{
  return setTo (homePos);
}

int
Rts2DevFocuser::stepOut (Rts2Conn * conn, int num)
{
  int ret;
  ret = info ();
  if (ret)
    return ret;

  focPositionNew = getFocPos () + num;
  setFocusTimeout ((int) ceil (abs (num) / focStepSec) + 5);

  ret = stepOut (num);
  if (ret)
    {
      if (conn)
	{
	  conn->sendCommandEnd (DEVDEM_E_HW, "cannot step out");
	}
    }
  else
    {
      maskState (FOC_MASK_FOCUSING, FOC_FOCUSING, "focusing started");
    }
  return ret;
}

int
Rts2DevFocuser::setTo (Rts2Conn * conn, int num)
{
  int ret;
  ret = setTo (num);
  if (ret)
    conn->sendCommandEnd (DEVDEM_E_HW, "cannot step out");
  else
    maskState (FOC_MASK_FOCUSING, FOC_FOCUSING, "focusing started");
  return ret;
}

int
Rts2DevFocuser::home (Rts2Conn * conn)
{
  int ret;
  ret = home ();
  if (ret)
    conn->sendCommandEnd (DEVDEM_E_HW, "cannot home focuser");
  else
    maskState (FOC_MASK_FOCUSING, FOC_FOCUSING, "homing started");
  return ret;
}

int
Rts2DevFocuser::autoFocus (Rts2Conn * conn)
{
  /* ask for priority */

  maskState (FOC_MASK_FOCUSING, FOC_FOCUSING, "autofocus started");

  // command ("priority 50");

  return 0;
}

int
Rts2DevFocuser::isFocusing ()
{
  int ret;
  time_t now;
  time (&now);
  if (now > focusTimeout)
    return -1;
  ret = info ();
  if (ret)
    return -1;
  if (getFocPos () != focPositionNew)
    return USEC_SEC;
  return -2;
}

int
Rts2DevFocuser::endFocusing ()
{
  return 0;
}

void
Rts2DevFocuser::setFocusTimeout (int timeout)
{
  time (&focusTimeout);
  focusTimeout += timeout;
}

bool
Rts2DevFocuser::isAtStartPosition ()
{
  return false;
}

int
Rts2DevFocuser::checkStartPosition ()
{
  if (startPosition != INT_MIN && isAtStartPosition ())
    {
      return setTo (NULL, startPosition);
    }
  return 0;
}

Rts2DevConnFocuser::Rts2DevConnFocuser (int in_sock, Rts2DevFocuser * in_master_device):
Rts2DevConn (in_sock, in_master_device)
{
  master = in_master_device;
}

int
Rts2DevConnFocuser::commandAuthorized ()
{
  if (isCommand ("exit"))
    {
      close (sock);
      return -1;
    }
  else if (isCommand ("help"))
    {
      send ("ready - is focuser ready?");
      send ("info  - information about focuser");
      send ("step  - move by given steps offset");
      send ("set   - set to given position");
      send ("focus - auto focusing");
      send ("exit  - exit from connection");
      send ("help  - print, what you are reading just now");
      return 0;
    }
  else if (isCommand ("step"))
    {
      int num;
      // CHECK_PRIORITY;

      if (paramNextInteger (&num) || !paramEnd ())
	return -2;

      return master->stepOut (this, num);
    }
  else if (isCommand ("set"))
    {
      int num;
      // CHECK_PRIORITY;

      if (paramNextInteger (&num) || !paramEnd ())
	return -2;

      return master->setTo (this, num);
    }
  else if (isCommand ("focus"))
    {
      // CHECK_PRIORITY;

      return master->autoFocus (this);
    }
  else if (isCommand ("home"))
    {
      if (!paramEnd ())
	return -2;
      return master->home (this);
    }
  else if (isCommand ("switch"))
    {
      int switch_num;
      int new_state;
      if (paramNextInteger (&switch_num) || paramNextInteger (&new_state)
	  || !paramEnd ())
	return -2;
      return master->setSwitch (switch_num, new_state);
    }
  return Rts2DevConn::commandAuthorized ();
}
