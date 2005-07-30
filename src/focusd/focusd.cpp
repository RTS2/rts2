/*!
 * $Id$
 *
 * @author standa
 */

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

#include "focuser.h"

Rts2DevFocuser::Rts2DevFocuser (int argc, char **argv):
Rts2Device (argc, argv, DEVICE_TYPE_FOCUS, 5566, "F0")
{
  char *states_names[1] = { "focuser" };
  setStateNames (1, states_names);

  focCamera[0] = '\0';

  addOption ('x', "camera_name", 1, "associated camera name (ussualy B0x)");
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
    default:
      return Rts2Device::processOption (in_opt);
    }
  return 0;
}

Rts2DevConn *
Rts2DevFocuser::createConnection (int in_sock, int conn_num)
{
  return new Rts2DevConnFocuser (in_sock, this);
}

int
Rts2DevFocuser::checkState ()
{
  if ((getState (0) & FOC_MASK_FOCUSING) == FOC_FOCUSING)
    {
      int ret;
      ret = isFocusing ();

      if (ret >= 0)
	setTimeout (ret);
    }
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
Rts2DevFocuser::sendInfo (Rts2Conn * conn)
{
  conn->sendValue ("temp", focTemp);
  conn->sendValue ("pos", focPos);
  return 0;
}

int
Rts2DevFocuser::baseInfo (Rts2Conn * conn)
{
  int ret;

  ret = baseInfo ();
  if (ret)
    {
      conn->sendCommandEnd (DEVDEM_E_HW, "telescope not ready");
      return -1;
    }

  conn->sendValue ("type", focType);
  conn->sendValue ("camera", focCamera);
  return 0;
}

int
Rts2DevFocuser::stepOut (Rts2Conn * conn, int num, int direction)
{
  int ret;

  ret = stepOut (num, direction);
  if (ret)
    conn->sendCommandEnd (DEVDEM_E_HW, "cannot step out");
  return ret;
}

int
Rts2DevFocuser::autoFocus (Rts2Conn * conn)
{
  /* ask for priority */

  maskState (0, FOC_MASK_FOCUSING, FOC_FOCUSING, "autofocus started");

  // command ("priority 50");

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
      send ("focus - auto focusing");
      send ("exit  - exit from connection");
      send ("help  - print, what you are reading just now");
      return 0;
    }
  else if (isCommand ("step"))
    {
      int num;
      int direction;

      // CHECK_PRIORITY;

      if (paramNextInteger (&num) || paramNextInteger (&direction)
	  || !paramEnd ())
	return -2;

      return master->stepOut (this, num, direction);
    }
  else if (isCommand ("focus"))
    {
      // CHECK_PRIORITY;

      return master->autoFocus (this);
    }

  return Rts2DevConn::commandAuthorized ();
}
