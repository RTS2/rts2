#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "../utils/rts2block.h"
#include "../utils/rts2device.h"

#include "mirror.h"

Rts2DevMirror::Rts2DevMirror (int argc, char **argv):
Rts2Device (argc, argv, DEVICE_TYPE_MIRROR, 5561, "M0")
{
  char *states_names[1] = { "mirror" };
  setStateNames (1, states_names);
}

Rts2DevMirror::~Rts2DevMirror (void)
{

}

int
Rts2DevMirror::init ()
{
  return Rts2Device::init ();
}

int
Rts2DevMirror::idle ()
{
  int ret;
  ret = Rts2Device::idle ();
  switch (getState (0) & MIRROR_MASK)
    {
    case MIRROR_A_B:
      ret = isOpened ();
      if (ret == -2)
	return endOpen ();
      break;
    case MIRROR_B_A:
      ret = isClosed ();
      if (ret == -2)
	return endClose ();
      break;
    default:
      return ret;
    }
  // set timeouts..
  setTimeoutMin (1000);
}

Rts2Conn *
Rts2DevMirror::createConnection (int in_sock, int conn_num)
{
  return new Rts2DevConnMirror (in_sock, this);
}

int
Rts2DevMirror::ready (Rts2Conn * conn)
{
  int ret;
  ret = ready ();
  if (ret)
    {
      conn->sendCommandEnd (DEVDEM_E_HW, "mirror not ready");
      return -1;
    }
  return 0;
}

int
Rts2DevMirror::info (Rts2Conn * conn)
{
  int ret;
  ret = info ();
  if (ret)
    {
      conn->sendCommandEnd (DEVDEM_E_HW, "mirror not ready");
      return -1;
    }
  return 0;
}

int
Rts2DevMirror::baseInfo (Rts2Conn * conn)
{
  int ret;
  ret = baseInfo ();
  if (ret)
    {
      conn->sendCommandEnd (DEVDEM_E_HW, "mirror not ready");
      return -1;
    }
  return 0;
}


int
Rts2DevConnMirror::commandAuthorized ()
{
  if (isCommand ("mirror"))
    {
      char *str_dir;
      int ret = 0;
      CHECK_PRIORITY;
      if (paramNextString (&str_dir) || !paramEnd ())
	return -2;
      if (!strcasecmp (str_dir, "open"))
	ret = master->startOpen ();
      if (!strcasecmp (str_dir, "close"))
	ret = master->startClose ();
      if (ret)
	{
	  sendCommandEnd (DEVDEM_E_HW, "cannot open/close mirror");
	  return -1;
	}
      return 0;
    }
  return Rts2DevConn::commandAuthorized ();
}
