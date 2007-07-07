#include "ir.h"

#define BLIND_SIZE 1.0

class Rts2DevTelescopeIr:public Rts2TelescopeIr
{
private:
  double rotatorOffset;
  struct ln_equ_posn target;
  int irTracking;

  int startMoveReal (double ra, double dec);
protected:
    virtual int processOption (int in_opt);
public:
    Rts2DevTelescopeIr (int in_arcg, char **in_argv);
  virtual int startMove (double tar_ra, double tar_dec);
  virtual int isMoving ();
  virtual int stopMove ();

  virtual int stopWorm ();
  virtual int startWorm ();

  virtual int changeMasterState (int new_state);
};

int
Rts2DevTelescopeIr::processOption (int in_opt)
{
  switch (in_opt)
    {
    case 't':
      irTracking = atoi (optarg);
      break;
    case 'r':
      rotatorOffset = atof (optarg);
      break;
    default:
      return Rts2TelescopeIr::processOption (in_opt);
    }
  return 0;
}

int
Rts2DevTelescopeIr::startMoveReal (double ra, double dec)
{
  int status = 0;
  status = setTrack (irTracking, domeAutotrack->getValueBool ());
  status = tpl_set ("POINTING.TARGET.RA", ra / 15.0, &status);
  status = tpl_set ("POINTING.TARGET.DEC", dec, &status);
  if (!getDerotatorPower ())
    {
      status = tpl_set ("DEROTATOR[3].POWER", 1, &status);
//      status = tpl_set ("CABINET.POWER", 1, &status);
    }

#ifdef DEBUG_EXTRA
  logStream (MESSAGE_DEBUG) << "IR startMove TRACK status " << status <<
    sendLog;
#endif
  if (rotatorOffset != 0)
    status = tpl_set ("DEROTATOR[3].OFFSET", rotatorOffset, &status);

  return status;
}

int
Rts2DevTelescopeIr::startMove (double ra, double dec)
{
  int status = 0;
  double sep;

  target.ra = ra;
  target.dec = dec;

  // move to zenit - move to different dec instead
  if (fabs (dec - telLatitude->getValueDouble ()) <= BLIND_SIZE)
    {
      if (fabs (ra / 15.0 - getLocSidTime ()) <= BLIND_SIZE / 15.0)
	{
	  target.dec = telLatitude->getValueDouble () - BLIND_SIZE;
	}
    }

  double az_off = 0;
  double alt_off = 0;
  status = tpl_set ("AZ.OFFSET", az_off, &status);
  status = tpl_set ("ZD.OFFSET", alt_off, &status);
  if (status)
    {
      logStream (MESSAGE_ERROR) << "IR startMove cannot zero offset" <<
	sendLog;
      return -1;
    }

  status = startMoveReal (ra, dec);
  if (status)
    return -1;

  // wait till we get it processed
  sep = getMoveTargetSep ();
  if (sep > 2)
    sleep (3);
  else if (sep > 2 / 60.0)
    usleep (USEC_SEC / 10);
  else
    usleep (USEC_SEC / 10);

  time (&timeout);
  timeout += 120;
  return 0;
}

int
Rts2DevTelescopeIr::isMoving ()
{
  int status = 0;
  double poin_dist;
  time_t now;
  status = tpl_get ("POINTING.TARGETDISTANCE", poin_dist, &status);
  time (&now);
  // 0.01 = 36 arcsec
  if (fabs (poin_dist) <= 0.01)
    {
#ifdef DEBUG_EXTRA
      logStream (MESSAGE_DEBUG) << "IR isMoving target distance " << poin_dist
	<< sendLog;
#endif
      return -2;
    }
  // finish due to timeout
  if (timeout < now)
    {
      logStream (MESSAGE_ERROR) << "IR isMoving target distance in timeout "
	<< poin_dist << "(" << status << ")" << sendLog;
      return -1;
    }
  return USEC_SEC / 100;
}

int
Rts2DevTelescopeIr::stopMove ()
{
  int status = 0;
  double zd;
  Rts2DevTelescope::stopMove ();
  info ();
  // ZD check..
  status = tpl_get ("ZD.CURRPOS", zd, &status);
  if (status)
    {
      logStream (MESSAGE_DEBUG) << "IR stopMove cannot get ZD! (" << status <<
	")" << sendLog;
      return -1;
    }
  if (fabs (zd) < 1)
    {
      logStream (MESSAGE_DEBUG) << "IR stopMove suspicious ZD.. " << zd <<
	sendLog;
      status = setTrack (0);
      if (status)
	{
	  logStream (MESSAGE_DEBUG) << "IR stopMove cannot set track: " <<
	    status << sendLog;
	  return -1;
	}
      return 0;
    }
  startMoveReal (telRa->getValueDouble (), telDec->getValueDouble ());
  return 0;
}

Rts2DevTelescopeIr::Rts2DevTelescopeIr (int in_argc, char **in_argv):Rts2TelescopeIr (in_argc,
		 in_argv)
{
  rotatorOffset = 0;
  irTracking = 4;

  addOption ('r', "rotator_offset", 1, "rotator offset, default to 0");
  addOption ('t', "ir_tracking", 1,
	     "IR tracking (1, 2, 3 or 4 - read OpenTCI doc; default 4");
}

int
Rts2DevTelescopeIr::startWorm ()
{
  int status = TPL_OK;
  status = setTrack (irTracking, domeAutotrack->getValueBool ());
  if (status != TPL_OK)
    return -1;
  return 0;
}

int
Rts2DevTelescopeIr::stopWorm ()
{
  int status = TPL_OK;
  status = setTrack (0);
  if (status)
    return -1;
  return 0;
}

int
Rts2DevTelescopeIr::changeMasterState (int new_state)
{
  switch (new_state)
    {
    case SERVERD_DUSK:
    case SERVERD_NIGHT:
    case SERVERD_DAWN:
      coverOpen ();
      break;
    default:
      coverClose ();
      break;
    }
  return Rts2DevTelescope::changeMasterState (new_state);
}

int
main (int argc, char **argv)
{
  Rts2DevTelescopeIr device = Rts2DevTelescopeIr (argc, argv);
  return device.run ();
}
