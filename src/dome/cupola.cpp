#include "copula.h"
#include "../utils/rts2config.h"

#include <math.h>

Rts2DevCopula::Rts2DevCopula (int in_argc, char **in_argv):
Rts2DevDome (in_argc, in_argv, DEVICE_TYPE_COPULA)
{
  targetPos.ra = nan ("f");
  targetPos.dec = nan ("f");
  currentAz = 0;		// pointing to south
  targetDistance = 0;

  configFile = NULL;

  addOption ('c', "config", 1, "configuration file");
}

int
Rts2DevCopula::processOption (int in_opt)
{
  switch (in_opt)
    {
    case 'c':
      configFile = optarg;
      break;
    default:
      return Rts2DevDome::processOption (in_opt);
    }
  return 0;
}

int
Rts2DevCopula::init ()
{
  int ret;
  ret = Rts2DevDome::init ();
  if (ret)
    return ret;

  // get config values..
  Rts2Config *config;
  config = Rts2Config::instance ();
  config->loadFile ();
  observer = config->getObserver ();
  return 0;
}

Rts2DevConn *
Rts2DevCopula::createConnection (int in_sock, int conn_num)
{
  return new Rts2DevConnCopula (in_sock, this);
}

int
Rts2DevCopula::idle ()
{
  long ret;
  if ((getState (0) & DOME_COP_MASK_MOVE) == DOME_COP_MOVE)
    {
      ret = isMoving ();
      if (ret >= 0)
	setTimeout (ret);
      else
	{
	  infoAll ();
	  if (ret == -2)
	    moveEnd ();
	  else
	    moveStop ();
	}
    }
  else if (needSplitChange () > 0)
    {
      moveStart ();
      setTimeout (USEC_SEC);
    }
  else
    {
      setTimeout (10 * USEC_SEC);
    }
  return Rts2DevDome::idle ();
}

int
Rts2DevCopula::sendBaseInfo (Rts2Conn * conn)
{
  return Rts2DevDome::sendBaseInfo (conn);
}

int
Rts2DevCopula::sendInfo (Rts2Conn * conn)
{
  struct ln_hrz_posn hrz;
  // target ra+dec
  conn->sendValue ("tar_ra", targetPos.ra);
  conn->sendValue ("tar_dec", targetPos.dec);
  getTargetAltAz (&hrz);
  conn->sendValue ("tar_alt", hrz.alt);
  conn->sendValue ("tar_az", hrz.az);
  conn->sendValue ("az", currentAz);
  return Rts2DevDome::sendInfo (conn);
}

int
Rts2DevCopula::moveTo (Rts2Conn * conn, double ra, double dec)
{
  int ret;
  targetPos.ra = ra;
  targetPos.dec = dec;
  infoAll ();
  ret = moveStart ();
  if (ret)
    return ret;
  maskState (0, DOME_COP_MASK_SYNC, DOME_COP_NOT_SYNC);
  return 0;
}

int
Rts2DevCopula::moveStart ()
{
  maskState (0, DOME_COP_MASK_MOVE, DOME_COP_MOVE);
  return 0;
}

int
Rts2DevCopula::moveStop ()
{
  maskState (0, DOME_COP_MASK, DOME_COP_NOT_MOVE | DOME_COP_NOT_SYNC);
  infoAll ();
  return 0;
}

void
Rts2DevCopula::synced ()
{
  infoAll ();
  maskState (0, DOME_COP_MASK_SYNC, DOME_COP_SYNC);
}

int
Rts2DevCopula::moveEnd ()
{
  maskState (0, DOME_COP_MASK, DOME_COP_NOT_MOVE | DOME_COP_SYNC);
  infoAll ();
  return 0;
}

void
Rts2DevCopula::getTargetAltAz (struct ln_hrz_posn *hrz)
{
  double JD;
  JD = ln_get_julian_from_sys ();
  ln_get_hrz_from_equ (&targetPos, observer, JD, hrz);
}

int
Rts2DevCopula::needSplitChange ()
{
  int ret;
  struct ln_hrz_posn targetHrz;
  double splitWidth;
  getTargetAltAz (&targetHrz);
  splitWidth = getSplitWidth (targetHrz.alt);
  if (splitWidth < 0)
    return -1;
  // get current az
  ret = info ();
  if (ret)
    return -1;
  // simple check; can be repleaced by some more complicated for more complicated setups
  targetDistance = getCurrentAz () - targetHrz.az;
  if (targetDistance > 180)
    targetDistance = (targetDistance - 360);
  else if (targetDistance < -180)
    targetDistance = (targetDistance + 360);
  if (fabs (currentAz - targetHrz.az) > getSplitWidth (targetHrz.alt))
    {
      if ((getState (0) & DOME_COP_MASK_SYNC) == DOME_COP_NOT_SYNC)
	synced ();
      return 1;
    }
  return 0;
}

int
Rts2DevConnCopula::commandAuthorized ()
{
  if (isCommand ("move"))
    {
      double tar_ra;
      double tar_dec;
      if (paramNextDouble (&tar_ra) || paramNextDouble (&tar_dec)
	  || !paramEnd ())
	return -2;
      return master->moveTo (this, tar_ra, tar_dec);
    }
  return Rts2DevConnDome::commandAuthorized ();
}
