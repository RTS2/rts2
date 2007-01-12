#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "../utils/rts2block.h"
#include "../utils/rts2device.h"

#include "mirror.h"

Rts2DevMirror::Rts2DevMirror (int in_argc, char **in_argv):
Rts2Device (in_argc, in_argv, DEVICE_TYPE_MIRROR, "M0")
{
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
  switch (getState () & MIRROR_MASK)
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
  setTimeoutMin (100000);
  return ret;
}

Rts2DevConn *
Rts2DevMirror::createConnection (int in_sock)
{
  return new Rts2DevConnMirror (in_sock, this);
}

int
Rts2DevMirror::startOpen (Rts2Conn * conn)
{
  int ret;
  ret = startOpen ();
  if (ret)
    {
      conn->sendCommandEnd (DEVDEM_E_HW, "cannot open mirror");
      return -1;
    }
  return 0;
}

int
Rts2DevMirror::startClose (Rts2Conn * conn)
{
  int ret;
  ret = startClose ();
  if (ret)
    {
      conn->sendCommandEnd (DEVDEM_E_HW, "cannot close mirror");
      return -1;
    }
  return 0;
}

int
Rts2DevMirror::sendInfo (Rts2Conn * conn)
{
  return 0;
}

int
Rts2DevMirror::sendBaseInfo (Rts2Conn * conn)
{
  return 0;
}

int
Rts2DevConnMirror::commandAuthorized ()
{
  if (isCommand ("mirror"))
    {
      char *str_dir;
      if (paramNextString (&str_dir) || !paramEnd ())
	return -2;
      if (!strcasecmp (str_dir, "open"))
	return master->startOpen (this);
      if (!strcasecmp (str_dir, "close"))
	return master->startClose (this);
    }
  if (isCommand ("set"))
    {
      char *str_dir;
      if (paramNextString (&str_dir) || !paramEnd () ||
	  (strcasecmp (str_dir, "A") && strcasecmp (str_dir, "B")))
	return -2;
      if (!strcasecmp (str_dir, "A"))
	if ((master->getState () & MIRROR_MASK) != MIRROR_A)
	  {
	    return master->startClose (this);
	  }
	else
	  {
	    sendCommandEnd (DEVDEM_E_IGNORE, "already in A");
	    return -1;
	  }
      else if (!strcasecmp (str_dir, "B"))
	if ((master->getState () & MIRROR_MASK) != MIRROR_B)
	  {
	    return master->startOpen (this);
	  }
	else
	  {
	    sendCommandEnd (DEVDEM_E_IGNORE, "already in B");
	    return -1;
	  }
    }
  return Rts2DevConn::commandAuthorized ();
}
