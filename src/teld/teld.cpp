#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <libnova/libnova.h>

#include "../utils/rts2device.h"
#include "../utils/rts2block.h"
#include "../utils/libnova_cpp.h"

#include "telescope.h"
#include "rts2devclicop.h"

#include "model/telmodel.h"

Rts2DevTelescope::Rts2DevTelescope (int in_argc, char **in_argv):
Rts2Device (in_argc, in_argv, DEVICE_TYPE_MOUNT, "T0")
{
  createValue (telRa, "MNT_RA", "mount RA (read from sensors)");
  createValue (telDec, "MNT_DEC", "mount DEC (read from sensors)");
  createValue (telAlt, "ALT", "mount ALT", true);
  createValue (telAz, "AZ", "mount AZIMUTH", true);
  createValue (telSiderealTime, "siderealtime",
	       "telescope local sidereal time", false);
  createValue (telLocalTime, "localtime", "telescope local time", false);
  createValue (lastRa, "LAST_RA", "last target RA");
  createValue (lastDec, "LAST_DEC", "last target DEC");
  createValue (lastTarRa, "RASC", "target RA");
  createValue (lastTarDec, "DECL", "target DEC");
  createValue (ax1, "MNT_AX0", "mount axis 0 counts");
  createValue (ax2, "MNT_AX1", "mount axis 1 counts");

  move_fixed = 0;

  move_connection = NULL;
  moveInfoCount = 0;
  moveInfoMax = 100;

  createValue (moveMark, "MNT_MARK", "correction mark");
  moveMark->setValueInteger (0);

  createValue (numCorr, "num_corr", "number of corrections send to mount",
	       false);
  numCorr->setValueInteger (0);

  locCorNum = -1;
  locCorRa = 0;
  locCorDec = 0;

  createValue (raCorr, "RA_CORR", "correction in RA");
  createValue (decCorr, "DEC_CORR", "corrections in DEC");
  createValue (posErr, "pos_err", "error in degrees", false);

  createValue (sepLimit, "seplimit", "separation limit", false);
  sepLimit->setValueDouble (5.0);

  createValue (minGood, "mingood", "minimal good distance (FOV)", false);
  minGood->setValueDouble (180.0);

  modelFile = NULL;
  model = NULL;

  modelFile0 = NULL;
  model0 = NULL;

  standbyPark = false;

  addOption ('n', "max_corr_num", 1,
	     "maximal number of corections aplied to mount during night (defaults to 1; -1 if unlimited)");
  addOption ('m', "model_file", 1,
	     "name of file holding RTS2-calculated model parameters (for flip = 1, or for both when o is not specified)");
  addOption ('o', "model_file_flip_0", 1,
	     "name of file holding RTS2-calculated model parameters for flip = 0");
  addOption ('l', "separation limit", 1,
	     "separation limit (corrections above that limit will be ignored)");
  addOption ('g', "min_good", 1,
	     "minimal good separation. Correction above that number will be aplied immediately. Default to 180 deg");

  addOption ('s', "standby-park", 0, "park when switched to standby");

  maxCorrNum = 1;

  createValue (knowPosition, "know_position", "if position is know or not",
	       false);
  knowPosition->setValueInteger (0);

  createValue (rasc, "CUR_RA", "current RA (real or from astrometry");
  createValue (desc, "CUR_DEC", "current DEC (real or from astrometry");

  nextReset = RESET_RESTART;

  unsetTarget ();

  for (int i = 0; i < 4; i++)
    {
      timerclear (dir_timeouts + i);
    }
  createValue (telGuidingSpeed, "guiding_speed", "telescope guiding speed",
	       false);

  // default is to aply model corrections
  createValue (corrections, "RTS_COR", "RTS2 corrections bitmask", true);
  corrections->setValueInteger (COR_MODEL);

  // send telescope position every 60 seconds
  setIdleInfoInterval (60);
}

Rts2DevTelescope::~Rts2DevTelescope (void)
{
  delete model;
  delete model0;
}

int
Rts2DevTelescope::processOption (int in_opt)
{
  switch (in_opt)
    {
    case 'n':
      maxCorrNum = atoi (optarg);
      break;
    case 'm':
      modelFile = optarg;
      break;
    case 'o':
      modelFile0 = optarg;
      break;
    case 'l':
      sepLimit->setValueDouble (atof (optarg));
      break;
    case 'g':
      minGood->setValueDouble (atof (optarg));
      break;
    case 's':
      standbyPark = true;
      break;
    default:
      return Rts2Device::processOption (in_opt);
    }
  return 0;
}

int
Rts2DevTelescope::setTarget (double tar_ra, double tar_dec)
{
  // either we know position, and we move to exact position which we know the center is (from correction..)
  if (knowPosition->getValueInteger ())
    {
      // should replace that with distance check..
      if (lastRa->getValueDouble () == tar_ra
	  && lastDec->getValueDouble () == tar_dec)
	return 0;
    }
  // or we don't know our position yet, but we issue move to same position..
  else
    {
      if (lastTar.ra == tar_ra && lastTar.dec == tar_dec)
	return 0;
    }
  lastTar.ra = tar_ra;
  lastTar.dec = tar_dec;
  return 1;
}

void
Rts2DevTelescope::getTargetCorrected (struct ln_equ_posn *out_tar, double JD)
{
  getTarget (out_tar);
  applyCorrections (out_tar, JD);
}

double
Rts2DevTelescope::getMoveTargetSep ()
{
  struct ln_equ_posn curr;
  int ret;
  ret = info ();
  if (ret)
    return 0;
  if (knowPosition->getValueInteger ())
    {
      curr.ra = lastRa->getValueDouble ();
      curr.dec = lastDec->getValueDouble ();
    }
  else
    {
      curr.ra = telRa->getValueDouble ();
      curr.dec = telDec->getValueDouble ();
    }
  return ln_get_angular_separation (&curr, &lastTar);
}

void
Rts2DevTelescope::getTargetAltAz (struct ln_hrz_posn *hrz)
{
  getTargetAltAz (hrz, ln_get_julian_from_sys ());
}

void
Rts2DevTelescope::getTargetAltAz (struct ln_hrz_posn *hrz, double jd)
{
  struct ln_lnlat_posn observer;
  observer.lng = telLongtitude->getValueDouble ();
  observer.lat = telLatitude->getValueDouble ();
  ln_get_hrz_from_equ (&lastTar, &observer, jd, hrz);
}

double
Rts2DevTelescope::getLocSidTime ()
{
  return getLocSidTime (ln_get_julian_from_sys ());
}

double
Rts2DevTelescope::getLocSidTime (double JD)
{
  double ret;
  ret =
    ln_get_apparent_sidereal_time (JD) * 15.0 +
    telLongtitude->getValueDouble ();
  return ln_range_degrees (ret) / 15.0;
}

double
Rts2DevTelescope::getSidTime (double JD)
{
  return ln_get_apparent_sidereal_time (JD);
}

double
Rts2DevTelescope::getLstDeg (double JD)
{
  return ln_range_degrees (15 * ln_get_apparent_sidereal_time (JD) +
			   telLongtitude->getValueDouble ());
}

void
Rts2DevTelescope::applyAberation (struct ln_equ_posn *pos, double JD)
{
  ln_get_equ_aber (pos, JD, pos);
}

void
Rts2DevTelescope::applyPrecession (struct ln_equ_posn *pos, double JD)
{
  ln_get_equ_prec (pos, JD, pos);
}

void
Rts2DevTelescope::applyRefraction (struct ln_equ_posn *pos, double JD)
{
  struct ln_hrz_posn hrz;
  struct ln_lnlat_posn obs;
  double ref;

  obs.lng = telLongtitude->getValueDouble ();
  obs.lat = telLatitude->getValueDouble ();

  ln_get_hrz_from_equ (pos, &obs, JD, &hrz);
  ref = ln_get_refraction_adj (hrz.alt, 1010, 10);
  hrz.alt += ref;
  ln_get_equ_from_hrz (&hrz, &obs, JD, pos);
}

void
Rts2DevTelescope::dontKnowPosition ()
{
  knowPosition->setValueInteger (0);
  locCorRa = 0;
  locCorDec = 0;
  locCorNum = -1;
}

void
Rts2DevTelescope::applyModel (struct ln_equ_posn *pos,
			      struct ln_equ_posn *model_change, int flip,
			      double JD)
{
  struct ln_equ_posn hadec;
  double ra;
  double lst;
  if ((!model && !model0) || !(corrections->getValueInteger () & COR_MODEL))
    {
      model_change->ra = 0;
      model_change->dec = 0;
      return;
    }
  lst = getLstDeg (JD);
  hadec.ra = ln_range_degrees (lst - pos->ra);
  hadec.dec = pos->dec;
  if (flip && model)
    model->reverse (&hadec);
  else if (!flip && model0)
    model0->reverse (&hadec);
  // fallback - use whenever model is available
  else if (model)
    model->reverse (&hadec);
  else
    model0->reverse (&hadec);

  // get back from model - from HA
  ra = ln_range_degrees (lst - hadec.ra);

  // calculate change
  model_change->ra = pos->ra - ra;
  model_change->dec = pos->dec - hadec.dec;

  // change above 5 degrees are strange - reject them
  if (fabs (model_change->ra) > 5 || fabs (model_change->dec) > 5)
    {
      logStream (MESSAGE_WARNING)
	<< "telescope applyModel big change - rejecting "
	<< model_change->ra << " " << model_change->dec << sendLog;
      model_change->ra = 0;
      model_change->dec = 0;
      return;
    }

  logStream (MESSAGE_DEBUG)
    << "Rts2DevTelescope::applyModel offsets ra: "
    << model_change->ra << " dec: " << model_change->dec << sendLog;

  pos->ra = ra;
  pos->dec = hadec.dec;
}

int
Rts2DevTelescope::init ()
{
  int ret;
  ret = Rts2Device::init ();
  if (ret)
    return ret;

  if (modelFile)
    {
      model = new Rts2TelModel (this, modelFile);
      ret = model->load ();
      if (ret)
	return ret;
    }
  if (modelFile0)
    {
      model0 = new Rts2TelModel (this, modelFile0);
      ret = model0->load ();
      if (ret)
	return ret;
    }

  createConstValue (telLongtitude, "LONG", "telescope longtitude");
  createConstValue (telLatitude, "LAT", "telescope latitude");
  createConstValue (telAltitude, "ALTI", "telescope altitude");

  createValue (telFlip, "MNT_FLIP", "telescope flip");

  return 0;
}

int
Rts2DevTelescope::initValues ()
{
  addConstValue ("MNT_TYPE", "mount telescope", telType);
  addConstValue ("MNT_SER", "mount serial", telSerialNumber);
  return Rts2Device::initValues ();
}

Rts2DevConn *
Rts2DevTelescope::createConnection (int in_sock)
{
  return new Rts2DevConnTelescope (in_sock, this);
}

void
Rts2DevTelescope::checkMoves ()
{
  int ret;
  if ((getState () & TEL_MASK_SEARCHING) == TEL_SEARCH)
    {
      ret = isSearching ();
      if (ret >= 0)
	{
	  setTimeout (ret);
	  if (moveInfoCount == moveInfoMax)
	    {
	      info ();
	      if (move_connection)
		sendInfo (move_connection);
	      moveInfoCount = 0;
	    }
	  else
	    {
	      moveInfoCount++;
	    }
	}
      else if (ret == -1)
	{
	  stopSearch ();
	}
      else if (ret == -2)
	{
	  endSearch ();
	}
    }
  else if ((getState () & TEL_MASK_MOVING) == TEL_MOVING)
    {
      if (move_fixed)
	ret = isMovingFixed ();
      else
	ret = isMoving ();
      if (ret >= 0)
	{
	  setTimeout (ret);
	  if (moveInfoCount == moveInfoMax)
	    {
	      info ();
	      if (move_connection)
		sendInfo (move_connection);
	      moveInfoCount = 0;
	    }
	  else
	    {
	      moveInfoCount++;
	    }
	}
      else if (ret == -1)
	{
	  if (move_connection)
	    sendInfo (move_connection);
	  maskState (DEVICE_ERROR_MASK | TEL_MASK_MOVING,
		     DEVICE_ERROR_HW | TEL_OBSERVING,
		     "move finished with error");
	  unsetTarget ();
	  move_connection = NULL;
	  dontKnowPosition ();
	}
      else if (ret == -2)
	{
	  infoAll ();
	  if (move_fixed)
	    ret = endMoveFixed ();
	  else
	    ret = endMove ();
	  if (ret)
	    {
	      maskState (DEVICE_ERROR_MASK | TEL_MASK_MOVING,
			 DEVICE_ERROR_HW | TEL_OBSERVING,
			 "move finished with error");
	      dontKnowPosition ();
	    }
	  else
	    maskState (TEL_MASK_MOVING, TEL_OBSERVING,
		       "move finished without error");
	  if (move_connection)
	    {
	      sendInfo (move_connection);
	    }
	  move_connection = NULL;
	}
    }
  else if ((getState () & TEL_MASK_MOVING) == TEL_PARKING)
    {
      ret = isParking ();
      if (ret >= 0)
	{
	  setTimeout (ret);
	  if (moveInfoCount == moveInfoMax)
	    {
	      info ();
	      if (move_connection)
		sendInfo (move_connection);
	      moveInfoCount = 0;
	    }
	  else
	    {
	      moveInfoCount++;
	    }
	}
      if (ret == -1)
	{
	  infoAll ();
	  maskState (DEVICE_ERROR_MASK | TEL_MASK_MOVING,
		     DEVICE_ERROR_HW | TEL_PARKED,
		     "park command finished with error");
	}
      else if (ret == -2)
	{
	  infoAll ();
	  ret = endPark ();
	  if (ret)
	    maskState (DEVICE_ERROR_MASK | TEL_MASK_MOVING,
		       DEVICE_ERROR_HW | TEL_PARKED,
		       "park command finished with error");
	  else
	    maskState (TEL_MASK_MOVING, TEL_PARKED,
		       "park command finished without error");
	  if (move_connection)
	    {
	      sendInfo (move_connection);
	    }
	}
    }
}

void
Rts2DevTelescope::checkGuiding ()
{
  struct timeval now;
  gettimeofday (&now, NULL);
  if (dir_timeouts[0].tv_sec > 0 && timercmp (&now, dir_timeouts + 0, >))
    stopGuide (DIR_NORTH);
  if (dir_timeouts[1].tv_sec > 0 && timercmp (&now, dir_timeouts + 1, >))
    stopGuide (DIR_EAST);
  if (dir_timeouts[2].tv_sec > 0 && timercmp (&now, dir_timeouts + 2, >))
    stopGuide (DIR_SOUTH);
  if (dir_timeouts[3].tv_sec > 0 && timercmp (&now, dir_timeouts + 3, >))
    stopGuide (DIR_WEST);
}

int
Rts2DevTelescope::idle ()
{
  checkMoves ();
  checkGuiding ();
  return Rts2Device::idle ();
}

void
Rts2DevTelescope::postEvent (Rts2Event * event)
{
  switch (event->getType ())
    {
    case EVENT_COP_SYNCED:
      maskState (TEL_MASK_COP, TEL_NO_WAIT_COP);
      break;
    }
  Rts2Device::postEvent (event);
}

int
Rts2DevTelescope::willConnect (Rts2Address * in_addr)
{
  if (in_addr->getType () == DEVICE_TYPE_COPULA)
    return 1;
  return Rts2Device::willConnect (in_addr);
}

Rts2DevClient *
Rts2DevTelescope::createOtherType (Rts2Conn * conn, int other_device_type)
{
  switch (other_device_type)
    {
    case DEVICE_TYPE_COPULA:
      return new Rts2DevClientCupolaTeld (conn);
    }
  return Rts2Device::createOtherType (conn, other_device_type);
}

int
Rts2DevTelescope::changeMasterState (int new_state)
{
  int status = new_state & SERVERD_STATUS_MASK;
  if (status == SERVERD_MORNING || status == SERVERD_DAY
      || new_state == SERVERD_OFF)
    {
      moveMark->setValueInteger (0);
      numCorr->setValueInteger (0);
      dontKnowPosition ();
    }
  // park us during day..
  if (status == SERVERD_DAY || new_state == SERVERD_OFF
      || ((new_state & SERVERD_STANDBY_MASK) && standbyPark))
    startPark (NULL);
  return 0;
}

int
Rts2DevTelescope::stopMove ()
{
  unsetTarget ();
  return 0;
}

int
Rts2DevTelescope::stopSearch ()
{
  maskState (TEL_MASK_SEARCHING, TEL_NOSEARCH);
  infoAll ();
  return 0;
}

int
Rts2DevTelescope::endSearch ()
{
  return maskState (TEL_MASK_SEARCHING, TEL_NOSEARCH);
}

int
Rts2DevTelescope::startGuide (char dir, double dir_dist)
{
  struct timeval *tv;
  struct timeval tv_add;
  int state_dir;
  switch (dir)
    {
    case DIR_NORTH:
      tv = dir_timeouts + 0;
      state_dir = TEL_GUIDE_NORTH;
      break;
    case DIR_EAST:
      tv = dir_timeouts + 1;
      state_dir = TEL_GUIDE_EAST;
      break;
    case DIR_SOUTH:
      tv = dir_timeouts + 2;
      state_dir = TEL_GUIDE_SOUTH;
      break;
    case DIR_WEST:
      tv = dir_timeouts + 3;
      state_dir = TEL_GUIDE_WEST;
      break;
    default:
      return -1;
    }
  double dir_timeout = (dir_dist / 15.0) * telGuidingSpeed->getValueDouble ();
  logStream (MESSAGE_INFO)
    << "telescope startGuide dir "
    << dir_dist << " timeout " << dir_timeout << sendLog;
  gettimeofday (&tv_add, NULL);
  tv_add.tv_sec = (int) (floor (dir_timeout));
  tv_add.tv_usec = (int) ((dir_timeout - tv_add.tv_sec) * USEC_SEC);
  timeradd (tv, &tv_add, tv);
  maskState (state_dir, state_dir, "started guiding");
  return 0;
}

int
Rts2DevTelescope::stopGuide (char dir)
{
  int state_dir;
  switch (dir)
    {
    case DIR_NORTH:
      dir_timeouts[0].tv_sec = 0;
      state_dir = TEL_GUIDE_NORTH;
      break;
    case DIR_EAST:
      dir_timeouts[1].tv_sec = 0;
      state_dir = TEL_GUIDE_EAST;
      break;
    case DIR_SOUTH:
      dir_timeouts[2].tv_sec = 0;
      state_dir = TEL_GUIDE_SOUTH;
      break;
    case DIR_WEST:
      dir_timeouts[3].tv_sec = 0;
      state_dir = TEL_GUIDE_WEST;
      break;
    default:
      return -1;
    }
  logStream (MESSAGE_INFO) << "telescope stopGuide dir " << dir << sendLog;
  maskState (state_dir, TEL_NOGUIDE, "guiding ended");
  return 0;
}

int
Rts2DevTelescope::stopGuideAll ()
{
  logStream (MESSAGE_INFO) << "telescope stopGuideAll" << sendLog;
  maskState (TEL_GUIDE_MASK, TEL_NOGUIDE, "guiding stoped");
  return 0;
}

int
Rts2DevTelescope::getAltAz ()
{
  struct ln_equ_posn telpos;
  struct ln_lnlat_posn observer;
  struct ln_hrz_posn hrz;

  telpos.ra = telRa->getValueDouble ();
  telpos.dec = telDec->getValueDouble ();

  observer.lng = telLongtitude->getValueDouble ();
  observer.lat = telLatitude->getValueDouble ();

  ln_get_hrz_from_equ_sidereal_time (&telpos, &observer,
				     getSidTime (ln_get_julian_from_sys ()),
				     &hrz);

  telAlt->setValueDouble (hrz.alt);
  telAz->setValueDouble (hrz.az);

  return 0;
}

int
Rts2DevTelescope::info ()
{

  telSiderealTime->setValueDouble (getLocSidTime ());

  // calculate alt+az
  getAltAz ();

  if (knowPosition->getValueInteger ())
    {
      rasc->setValueDouble (lastRa->getValueDouble ());
      desc->setValueDouble (lastDec->getValueDouble ());
    }
  else
    {
      rasc->setValueDouble (telRa->getValueDouble ());
      desc->setValueDouble (telDec->getValueDouble ());
    }
  lastTarRa->setValueDouble (lastTar.ra);
  lastTarDec->setValueDouble (lastTar.dec);
  return Rts2Device::info ();
}

void
Rts2DevTelescope::applyCorrections (struct ln_equ_posn *pos, double JD)
{
  // apply all posible corrections
  if (corrections->getValueInteger () & COR_ABERATION)
    applyAberation (pos, JD);
  if (corrections->getValueInteger () & COR_PRECESSION)
    applyPrecession (pos, JD);
  if (corrections->getValueInteger () & COR_REFRACTION)
    applyRefraction (pos, JD);
}

void
Rts2DevTelescope::applyCorrections (double &tar_ra, double &tar_dec)
{
  struct ln_equ_posn pos;
  pos.ra = tar_ra;
  pos.dec = tar_dec;

  applyCorrections (&pos, ln_get_julian_from_sys ());

  tar_ra = pos.ra;
  tar_dec = pos.dec;
}

int
Rts2DevTelescope::startMove (Rts2Conn * conn, double tar_ra, double tar_dec)
{
  int ret;
  struct ln_equ_posn pos;
  double JD;

  pos.ra = tar_ra;
  pos.dec = tar_dec;

  JD = ln_get_julian_from_sys ();

  applyCorrections (&pos, JD);

  logStream (MESSAGE_DEBUG) <<
    "start telescope move (interesting val 1) target " << tar_ra << " " <<
    tar_dec << " last " << lastRa->getValueDouble () << " " << lastDec->
    getValueDouble () << " known position " << knowPosition->
    getValueInteger () << " correction " << locCorNum << " " << locCorRa <<
    " " << locCorDec << " last target " << lastTar.ra << " " << lastTar.
    dec << " correction " << corrections->getValueInteger () << sendLog;
  // target is without corrections
  ret = setTarget (tar_ra, tar_dec);

  tar_ra = pos.ra;
  tar_dec = pos.dec;

  // we know our position and we are on it..don't move
  if (ret == 0)
    {
      conn->sendCommandEnd (DEVDEM_E_IGNORE, "move will not be performed");
      return -1;
    }
  if (knowPosition->getValueInteger ())
    {
      double sep;
      sep = getMoveTargetSep ();
      logStream (MESSAGE_DEBUG) << "start telescope move sep " << sep <<
	sendLog;
      if (sep > sepLimit->getValueDouble ())
	dontKnowPosition ();
    }
  tar_ra += locCorRa;
  tar_dec += locCorDec;
  logStream (MESSAGE_DEBUG) <<
    "start telescope move (interesting val 2) target " << tar_ra << " " <<
    tar_dec << " last " << lastRa->getValueDouble () << " " << lastDec->
    getValueDouble () << " known position " << knowPosition->
    getValueInteger () << " correction " << locCorNum << " " << locCorRa <<
    " " << locCorDec << sendLog;
  moveInfoCount = 0;
  ret = startMove (tar_ra, tar_dec);
  if (ret)
    conn->sendCommandEnd (DEVDEM_E_HW, "cannot perform move op");
  else
    {
      move_fixed = 0;
      moveMark->inc ();
      // try to sync dome..
      postEvent (new Rts2Event (EVENT_COP_START_SYNC, &pos));
      // somebody cared about it..
      if (isnan (pos.ra))
	{
	  maskState (TEL_MASK_COP_MOVING, TEL_MOVING | TEL_WAIT_COP,
		     "move started");
	}
      else
	{
	  maskState (TEL_MASK_MOVING | TEL_MASK_NEED_STOP, TEL_MOVING,
		     "move started");
	}
      move_connection = conn;
    }
  infoAll ();
  logStream (MESSAGE_INFO) << "start telescope move " << telRa->
    getValueDouble () << " " << telDec->
    getValueDouble () << " target " << tar_ra << " " << tar_dec << sendLog;
  return ret;
}

int
Rts2DevTelescope::endMove ()
{
  logStream (MESSAGE_INFO) << "telescope end move " << telRa->
    getValueDouble () << " " << telDec->
    getValueDouble () << " target " << lastTar.ra << " " << lastTar.
    dec << sendLog;
  return 0;
}

int
Rts2DevTelescope::startMoveFixed (double tar_ha, double tar_dec)
{
  logStream (MESSAGE_INFO) << "telescope start move fixed " << sendLog;
  return 0;
}

int
Rts2DevTelescope::endMoveFixed ()
{
  logStream (MESSAGE_INFO) << "telescope end move fixed " << sendLog;
  return 0;
}

int
Rts2DevTelescope::startMoveFixed (Rts2Conn * conn, double tar_ha,
				  double tar_dec)
{
  int ret;
  ret = startMoveFixed (tar_ha, tar_dec);
  if (ret)
    conn->sendCommandEnd (DEVDEM_E_HW, "cannot perform move op");
  else
    {
      move_fixed = 1;
      moveMark->inc ();
      maskState (TEL_MASK_MOVING | TEL_MASK_NEED_STOP, TEL_MOVING,
		 "move started");
      dontKnowPosition ();
      move_connection = conn;
    }
  return ret;
}

int
Rts2DevTelescope::startSearch (Rts2Conn * conn, double radius,
			       double in_searchSpeed)
{
  int ret;
  searchRadius = radius;
  searchSpeed = in_searchSpeed;
  ret = startSearch ();
  if (ret)
    return ret;
  move_connection = conn;
  maskState (TEL_MASK_SEARCHING, TEL_SEARCH);
  return 0;
}

int
Rts2DevTelescope::startResyncMove (Rts2Conn * conn, double tar_ra,
				   double tar_dec)
{
  int ret;

  logStream (MESSAGE_DEBUG) <<
    "telescope startResyncMove (interesting val 1) target " << tar_ra << " "
    << tar_dec << " last " << lastRa->getValueDouble () << " " << lastDec->
    getValueDouble () << " known position " << knowPosition->
    getValueInteger () << " correction " << locCorNum << " " << locCorRa <<
    " " << locCorDec << " last target " << lastTar.ra << " " << lastTar.
    dec << sendLog;

  if (tar_ra != lastTar.ra || tar_dec != lastTar.dec)
    {
      logStream (MESSAGE_DEBUG) <<
	"telescope startResyncMove called wrong - calling startMove!" <<
	sendLog;
      return startMove (conn, tar_ra, tar_dec);
    }
  if (knowPosition->getValueInteger ())
    {
      double sep;
      sep = getMoveTargetSep ();
      logStream (MESSAGE_DEBUG) << "telescope startResyncMove sep " << sep <<
	sendLog;
      if (sep > sepLimit->getValueDouble ())
	dontKnowPosition ();
    }
  infoAll ();
  if (knowPosition->getValueInteger () == 2)
    {
      // we apply continuus corrections
      conn->sendCommandEnd (DEVDEM_E_IGNORE, "continuus corrections");
      return -1;
    }
  tar_ra += locCorRa;
  tar_dec += locCorDec;
  if (isBellowResolution (locCorRa, locCorDec))
    {
      logStream (MESSAGE_DEBUG) <<
	"Rts2DevTelescope::startResyncMove isBellowResolution" << sendLog;
      conn->sendCommandEnd (DEVDEM_E_IGNORE,
			    "position change is bellow telescope resolution");
      return -1;
    }
  applyCorrections (tar_ra, tar_dec);
  logStream (MESSAGE_DEBUG) <<
    "telescope startResyncMove (interesting val 2) target " << tar_ra << " "
    << tar_dec << " last " << lastRa->getValueDouble () << " " << lastDec->
    getValueDouble () << " known position " << knowPosition->
    getValueInteger () << " correction " << locCorNum << " " << locCorRa <<
    " " << locCorDec << sendLog;
  moveInfoCount = 0;
  ret = startMove (tar_ra, tar_dec);
  if (ret)
    conn->sendCommandEnd (DEVDEM_E_HW, "cannot perform move op");
  else
    {
      move_fixed = 0;
      if (knowPosition->getValueInteger () != 2)
	moveMark->inc ();
      maskState (TEL_MASK_MOVING | TEL_MASK_NEED_STOP, TEL_MOVING,
		 "move started");
      move_connection = conn;
    }
  return ret;
}


int
Rts2DevTelescope::setTo (Rts2Conn * conn, double set_ra, double set_dec)
{
  int ret;
  ret = setTo (set_ra, set_dec);
  if (ret)
    conn->sendCommandEnd (DEVDEM_E_HW, "cannot set to");
  return ret;
}

int
Rts2DevTelescope::correct (Rts2Conn * conn, int cor_mark, double cor_ra,
			   double cor_dec, struct ln_equ_posn *realPos)
{
  int ret = -1;
  struct ln_equ_posn targetPos;
  logStream (MESSAGE_DEBUG)
    << "telescope correct (interesting val 1) "
    << lastRa->getValueDouble ()
    << " "
    << lastDec->getValueDouble ()
    << " known position "
    << knowPosition->getValueInteger ()
    << " numCorr "
    << numCorr->getValueInteger ()
    << " correction "
    << locCorNum
    << " "
    << locCorRa
    << " "
    << locCorDec
    << " real "
    << realPos->ra
    << " "
    << realPos->dec
    << " move mark "
    << moveMark->getValueInteger ()
    << " cor_mark "
    << cor_mark << " cor_ra " << cor_ra << " cor_dec " << cor_dec << sendLog;
  // not moved yet
  raCorr->setValueDouble (cor_ra);
  decCorr->setValueDouble (cor_dec);
  targetPos.ra = realPos->ra - cor_ra;
  targetPos.dec = realPos->dec - cor_dec;
  posErr->setValueDouble (ln_get_angular_separation (&targetPos, realPos));
  if (posErr->getValueDouble () > sepLimit->getValueDouble ())
    {
      logStream (MESSAGE_WARNING)
	<< "big separation " << posErr << " " << sepLimit->
	getValueDouble () << sendLog;
      conn->sendCommandEnd (DEVDEM_E_IGNORE,
			    "separation greater then separation limit, ignoring");
      return -1;
    }
  if (moveMark->getValueInteger () == cor_mark)
    {
      // it's so big that we need resync now
      if (posErr->getValueDouble () >= minGood->getValueDouble ())
	{
	  if (numCorr->getValueInteger () == 0
	      && (numCorr->getValueInteger () < maxCorrNum || maxCorrNum < 0))
	    {
	      ret =
		correctOffsets (cor_ra, cor_dec, realPos->ra, realPos->dec);
	      if (!ret)
		{
		  numCorr->inc ();
		  double tar_ra = lastTar.ra;
		  double tar_dec = lastTar.dec;
		  unsetTarget ();
		  ret = startMove (conn, tar_ra, tar_dec);
		}
	    }
	  else
	    {
	      ret =
		Rts2DevTelescope::correctOffsets (cor_ra, cor_dec,
						  realPos->ra, realPos->dec);
	      if (!ret)
		{
		  numCorr->inc ();
		  lastRa->setValueDouble (realPos->ra);
		  lastDec->setValueDouble (realPos->dec);
		  ret = startResyncMove (conn, lastTar.ra, lastTar.dec);
		}
	    }

	  if (!ret)
	    {
	      locCorNum = moveMark->getValueInteger ();
	      return 0;
	    }
	  // if immediatel resync failed, use correction
	}
      if (numCorr->getValueInteger () < maxCorrNum || maxCorrNum < 0)
	{
	  ret = correct (cor_ra, cor_dec, realPos->ra, realPos->dec);
	  switch (ret)
	    {
	    case 1:
	      numCorr->inc ();
	      locCorRa += cor_ra;
	      locCorDec += cor_dec;
	      knowPosition->setValueInteger (2);
	      break;
	    case 0:
	      numCorr->inc ();
	      locCorRa = 0;
	      locCorDec = 0;
	      knowPosition->setValueInteger (1);
	      break;
	    }
	}
      else
	{
	  ret = 0;
	  // not yet corrected
	  if (locCorNum != moveMark->getValueInteger ())
	    {
	      // change scope
	      locCorRa += cor_ra;
	      locCorDec += cor_dec;
	      locCorNum = moveMark->getValueInteger ();
	      // next correction with same mark will be ignored
	    }
	}
      if (fabs (locCorRa) < 5 && fabs (locCorRa) < 5)
	{
	  if (!knowPosition->getValueInteger ())
	    {
	      knowPosition->setValueInteger (1);
	    }
	  info ();
	  if (knowPosition->getValueInteger () != 2 || locCorNum == -1)
	    {
	      lastRa->setValueDouble (realPos->ra);
	      lastDec->setValueDouble (realPos->dec);
	    }
	  locCorNum = moveMark->getValueInteger ();
	}
      else
	{
	  locCorNum = -1;
	  locCorRa = 0;
	  locCorDec = 0;
	}
    }
  else
    {
      // first change - set offsets
      if (numCorr->getValueInteger () == 0)
	{
	  ret = correctOffsets (cor_ra, cor_dec, realPos->ra, realPos->dec);
	  if (ret == 0)
	    numCorr->inc ();
	}
      // ignore changes - they will be (possibly) deleted at dontKnowPosition
    }
  logStream (MESSAGE_DEBUG)
    << "telescope correct (interesting val 2) "
    << lastRa->getValueDouble ()
    << " "
    << lastDec->getValueDouble ()
    << " known position "
    << knowPosition->getValueInteger ()
    << " correction "
    << locCorNum << " " << locCorRa << " " << locCorDec << sendLog;
  if (ret)
    conn->sendCommandEnd (DEVDEM_E_HW, "cannot perform correction");
  return ret;
}

int
Rts2DevTelescope::startPark (Rts2Conn * conn)
{
  int ret;
  unsetTarget ();
  ret = startPark ();
  if (ret)
    {
      if (conn)
	conn->sendCommandEnd (DEVDEM_E_HW, "cannot park");
    }
  else
    {
      move_fixed = 0;
      moveMark->inc ();
      maskState (TEL_MASK_MOVING | TEL_MASK_NEED_STOP, TEL_PARKING,
		 "parking started");
    }
  return ret;
}

int
Rts2DevTelescope::change (Rts2Conn * conn, double chng_ra, double chng_dec)
{
  int ret;
  logStream (MESSAGE_INFO)
    << "telescope change " << chng_ra << " " << chng_dec << sendLog;
  if (lastTar.ra < 0)
    {
      ret = info ();
      if (ret)
	return ret;
      ret =
	setTarget (telRa->getValueDouble () + chng_ra,
		   telDec->getValueDouble () + chng_dec);
    }
  else
    {
      ret = setTarget (lastTar.ra + chng_ra, lastTar.dec + chng_dec);
    }
  if (ret == 0)
    {
      conn->sendCommandEnd (DEVDEM_E_IGNORE, "move will not be performed");
      return -1;
    }
  ret = change (chng_ra, chng_dec);
  if (ret)
    {
      conn->sendCommandEnd (DEVDEM_E_HW, "cannot change");
    }
  else
    {
      // move_fixed = 0;
      moveMark->inc ();
      maskState (TEL_MASK_MOVING | TEL_MASK_NEED_STOP, TEL_MOVING,
		 "move started");
      move_connection = conn;
    }
  return ret;
}

int
Rts2DevTelescope::saveModel (Rts2Conn * conn)
{
  int ret;
  ret = saveModel ();
  if (ret)
    {
      conn->sendCommandEnd (DEVDEM_E_HW, "cannot save model");
    }
  return ret;
}

int
Rts2DevTelescope::loadModel (Rts2Conn * conn)
{
  int ret;
  ret = loadModel ();
  if (ret)
    {
      conn->sendCommandEnd (DEVDEM_E_HW, "cannot load model");
    }
  return ret;
}

int
Rts2DevTelescope::stopWorm (Rts2Conn * conn)
{
  int ret;
  ret = stopWorm ();
  if (ret)
    {
      conn->sendCommandEnd (DEVDEM_E_HW, "cannot stop worm");
    }
  return ret;
}

int
Rts2DevTelescope::startWorm (Rts2Conn * conn)
{
  int ret;
  ret = startWorm ();
  if (ret)
    {
      conn->sendCommandEnd (DEVDEM_E_HW, "cannot start worm");
    }
  return ret;
}

int
Rts2DevTelescope::resetMount (Rts2Conn * conn, resetStates reset_state)
{
  int ret;
  ret = resetMount (reset_state);
  if (ret)
    {
      conn->sendCommandEnd (DEVDEM_E_HW, "cannot reset");
    }
  return ret;
}

int
Rts2DevTelescope::getFlip ()
{
  int ret;
  ret = info ();
  if (ret)
    return ret;
  return telFlip->getValueInteger ();
}

int
Rts2DevTelescope::grantPriority (Rts2Conn * conn)
{
  // grant to imgproc, so it can use correct..
  if (conn->getOtherType () == DEVICE_TYPE_IMGPROC)
    return 1;
  return Rts2Device::grantPriority (conn);
}

Rts2DevConnTelescope::Rts2DevConnTelescope (int in_sock, Rts2DevTelescope * in_master_device):
Rts2DevConn (in_sock, in_master_device)
{
  master = in_master_device;
}

int
Rts2DevConnTelescope::commandAuthorized ()
{
  double tar_ra;
  double tar_dec;

  if (isCommand ("exit"))
    {
      close (sock);
      return -1;
    }
  else if (isCommand ("help"))
    {
      send ("ready - is telescope ready?");
      send ("info - information about telescope");
      send ("exit - exit from connection");
      send ("help - print, what you are reading just now");
      return 0;
    }
  else if (isCommand ("move"))
    {
      CHECK_PRIORITY;
      if (paramNextDouble (&tar_ra) || paramNextDouble (&tar_dec)
	  || !paramEnd ())
	return -2;
      master->modelOn ();
      return master->startMove (this, tar_ra, tar_dec);
    }
  else if (isCommand ("move_not_model"))
    {
      CHECK_PRIORITY;
      if (paramNextDouble (&tar_ra) || paramNextDouble (&tar_dec)
	  || !paramEnd ())
	return -2;
      master->modelOff ();
      return master->startMove (this, tar_ra, tar_dec);
    }
  else if (isCommand ("resync"))
    {
      CHECK_PRIORITY;
      if (paramNextDouble (&tar_ra) || paramNextDouble (&tar_dec)
	  || !paramEnd ())
	return -2;
      return master->startResyncMove (this, tar_ra, tar_dec);
    }
  else if (isCommand ("fixed"))
    {
      CHECK_PRIORITY;
      // tar_ra hold HA (Hour Angle)
      if (paramNextDouble (&tar_ra) || paramNextDouble (&tar_dec)
	  || !paramEnd ())
	return -2;
      master->modelOff ();
      return master->startMoveFixed (this, tar_ra, tar_dec);
    }
  else if (isCommand ("setto"))
    {
      CHECK_PRIORITY;
      if (paramNextDouble (&tar_ra) || paramNextDouble (&tar_dec)
	  || !paramEnd ())
	return -2;
      master->modelOn ();
      return master->setTo (this, tar_ra, tar_dec);
    }
  else if (isCommand ("correct"))
    {
      int cor_mark;
      struct ln_equ_posn realPos;
      double cor_ra;
      double cor_dec;
      if (paramNextInteger (&cor_mark)
	  || paramNextDouble (&cor_ra)
	  || paramNextDouble (&cor_dec)
	  || paramNextDouble (&realPos.ra)
	  || paramNextDouble (&realPos.dec) || !paramEnd ())
	return -2;
      return master->correct (this, cor_mark, cor_ra, cor_dec, &realPos);
    }
  else if (isCommand ("park"))
    {
      CHECK_PRIORITY;
      if (!paramEnd ())
	return -2;
      master->modelOn ();
      return master->startPark (this);
    }
  else if (isCommand ("change"))
    {
      CHECK_PRIORITY;
      if (paramNextDouble (&tar_ra) || paramNextDouble (&tar_dec)
	  || !paramEnd ())
	return -2;
      return master->change (this, tar_ra, tar_dec);
    }
  else if (isCommand ("save_model"))
    {
      return master->saveModel (this);
    }
  else if (isCommand ("load_model"))
    {
      return master->loadModel (this);
    }
  else if (isCommand ("worm_stop"))
    {
      return master->stopWorm (this);
    }
  else if (isCommand ("worm_start"))
    {
      return master->startWorm (this);
    }
  else if (isCommand ("reset"))
    {
      char *param;
      resetStates reset_state;
      CHECK_PRIORITY;
      if (paramEnd ())
	{
	  reset_state = RESET_RESTART;
	}
      else
	{
	  if (paramNextString (&param) || !paramEnd ())
	    return -2;
	  // switch param cases
	  if (!strcmp (param, "restart"))
	    reset_state = RESET_RESTART;
	  else if (!strcmp (param, "warm_start"))
	    reset_state = RESET_WARM_START;
	  else if (!strcmp (param, "cold_start"))
	    reset_state = RESET_COLD_START;
	  else if (!strcmp (param, "init"))
	    reset_state = RESET_INIT_START;
	  else
	    return -2;
	}
      return master->resetMount (this, reset_state);
    }
  else if (isCommand ("start_dir"))
    {
      char *dir;
      if (paramNextString (&dir) || !paramEnd ())
	return -2;
      return master->startDir (dir);
    }
  else if (isCommand ("stop_dir"))
    {
      char *dir;
      if (paramNextString (&dir) || !paramEnd ())
	return -2;
      return master->stopDir (dir);
    }
  else if (isCommand ("start_guide"))
    {
      char *dir;
      double dir_timeout;
      if (paramNextString (&dir)
	  || paramNextDouble (&dir_timeout) || !paramEnd ())
	return -2;
      return master->startGuide (*dir, dir_timeout);
    }
  else if (isCommand ("stop_guide"))
    {
      char *dir;
      if (paramNextString (&dir) || !paramEnd ())
	return -2;
      return master->stopGuide (*dir);
    }
  else if (isCommand ("stop_guide_all"))
    {
      if (!paramEnd ())
	return -2;
      return master->stopGuideAll ();
    }
  else if (isCommand ("search"))
    {
      double in_radius;
      double in_searchSpeed;
      if (paramNextDouble (&in_radius) || paramNextDouble (&in_searchSpeed)
	  || !paramEnd ())
	return -2;
      return master->startSearch (this, in_radius, in_searchSpeed);
    }
  else if (isCommand ("searchstop"))
    {
      if (!paramEnd ())
	return -2;
      return master->stopSearch ();
    }
  return Rts2DevConn::commandAuthorized ();
}
