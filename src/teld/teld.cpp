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
  createValue (telRa, "MNT_RA", "mount RA (read from sensors)", true,
	       RTS2_DT_RA);
  createValue (telDec, "MNT_DEC", "mount DEC (read from sensors)", true,
	       RTS2_DT_DEC);
  createValue (telAlt, "ALT", "mount ALT", true, RTS2_DT_DEC);
  createValue (telAz, "AZ", "mount AZIMUTH", true, RTS2_DT_DEGREES);
  createValue (telSiderealTime, "siderealtime",
	       "telescope local sidereal time", false);
  createValue (telLocalTime, "localtime", "telescope local time", false);
  createValue (lastRa, "LAST_RA", "last target RA", true, RTS2_DT_RA);
  createValue (lastDec, "LAST_DEC", "last target DEC", true, RTS2_DT_DEC);
  createValue (lastTarRa, "RASC", "best estimate of RA", true, RTS2_DT_RA);
  createValue (lastTarDec, "DECL", "best estimate of DEC", true, RTS2_DT_DEC);
  createValue (targetRa, "TAR_RA", "target RA", true, RTS2_DT_RA);
  createValue (targetDec, "TAR_DEC", "target DEC", true, RTS2_DT_DEC);
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

  createValue (raCorr, "RA_CORR", "correction in RA", true, RTS2_DT_DEG_DIST);
  createValue (decCorr, "DEC_CORR", "corrections in DEC", true,
	       RTS2_DT_DEG_DIST);
  createValue (posErr, "pos_err", "error in degrees", false,
	       RTS2_DT_DEG_DIST);

  createValue (sepLimit, "seplimit", "separation limit", false,
	       RTS2_DT_DEG_DIST);
  sepLimit->setValueDouble (5.0);

  createValue (minGood, "mingood", "minimal good distance (FOV)", false,
	       RTS2_DT_DEG_DIST);
  minGood->setValueDouble (180.0);

  createValue (quickEnabled, "quick_enabled",
	       "if on-line corrections are enabled", false, 0, true);
  quickEnabled->setValueBool (true);

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

  createValue (rasc, "CUR_RA", "current RA (real or from astrometry", true,
	       RTS2_DT_RA);
  createValue (desc, "CUR_DEC", "current DEC (real or from astrometry", true,
	       RTS2_DT_DEC);

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
  applyLocCorr (out_tar);
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

int
Rts2DevTelescope::setValue (Rts2Value * old_value, Rts2Value * new_value)
{
  if (old_value == quickEnabled)
    {
      return 0;
    }
  if (old_value == sepLimit)
    {
      return 0;
    }
  if (old_value == minGood)
    {
      return 0;
    }
  return Rts2Device::setValue (old_value, new_value);
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
  return Rts2Device::initValues ();
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
	  maskState (DEVICE_ERROR_MASK | TEL_MASK_CORRECTING |
		     TEL_MASK_MOVING,
		     DEVICE_ERROR_HW | TEL_NOT_CORRECTING | TEL_OBSERVING,
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
	      maskState (DEVICE_ERROR_MASK | TEL_MASK_CORRECTING |
			 TEL_MASK_MOVING,
			 DEVICE_ERROR_HW | TEL_NOT_CORRECTING | TEL_OBSERVING,
			 "move finished with error");
	      dontKnowPosition ();
	    }
	  else
	    maskState (TEL_MASK_CORRECTING | TEL_MASK_MOVING,
		       TEL_NOT_CORRECTING | TEL_OBSERVING,
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
  maskState (TEL_MASK_SEARCHING, TEL_NOSEARCH);
  return 0;
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

int
Rts2DevTelescope::scriptEnds ()
{
  quickEnabled->setValueBool (true);
  return Rts2Device::scriptEnds ();
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
Rts2DevTelescope::startMove (Rts2Conn * conn, double tar_ra, double tar_dec,
			     bool onlyCorrect)
{
  int ret;
  struct ln_equ_posn pos;
  double JD;

  targetRa->setValueDouble (tar_ra);
  targetDec->setValueDouble (tar_dec);

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
      if (isnan (sep) || sep > sepLimit->getValueDouble ())
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
	  if (onlyCorrect)
	    maskState (TEL_MASK_COP_MOVING | TEL_MASK_CORRECTING,
		       TEL_MOVING | TEL_CORRECTING | TEL_WAIT_COP,
		       "move started");
	  else
	    maskState (TEL_MASK_COP_MOVING, TEL_MOVING | TEL_WAIT_COP,
		       "move started");
	}
      else
	{
	  if (onlyCorrect)
	    maskState (TEL_MASK_MOVING | TEL_MASK_NEED_STOP |
		       TEL_MASK_CORRECTING, TEL_MOVING | TEL_CORRECTING,
		       "move started");
	  else
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
				  double tar_dec, bool onlyCorrect)
{
  int ret;
  ret = startMoveFixed (tar_ha, tar_dec);
  if (ret)
    conn->sendCommandEnd (DEVDEM_E_HW, "cannot perform move op");
  else
    {
      targetRa->setValueDouble (tar_ha);
      targetDec->setValueDouble (tar_dec);
      move_fixed = 1;
      moveMark->inc ();
      if (onlyCorrect)
	maskState (TEL_MASK_MOVING | TEL_MASK_CORRECTING | TEL_MASK_NEED_STOP,
		   TEL_MOVING | TEL_CORRECTING, "move started");
      else
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
				   double tar_dec, bool onlyCorrect)
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
      return startMove (conn, tar_ra, tar_dec, onlyCorrect);
    }
  if (knowPosition->getValueInteger ())
    {
      double sep;
      sep = getMoveTargetSep ();
      logStream (MESSAGE_DEBUG) << "telescope startResyncMove sep " << sep <<
	sendLog;
      if (isnan (sep) || sep > sepLimit->getValueDouble ())
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
      if (onlyCorrect)
	maskState (TEL_MASK_CORRECTING | TEL_MASK_MOVING | TEL_MASK_NEED_STOP,
		   TEL_CORRECTING | TEL_MOVING, "correction move started");
      else
	maskState (TEL_MASK_MOVING | TEL_MASK_NEED_STOP, TEL_MOVING,
		   "correction move started");
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
	<< "big separation " << posErr->getValueDouble () << " " << sepLimit->
	getValueDouble () << sendLog;
      conn->sendCommandEnd (DEVDEM_E_IGNORE,
			    "separation greater then separation limit, ignoring");
      return -1;
    }
  if (moveMark->getValueInteger () == cor_mark)
    {
      // it's so big that we need resync now
      if (posErr->getValueDouble () >= minGood->getValueDouble ()
	  && quickEnabled->getValueBool ())
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
		  ret = startMove (conn, tar_ra, tar_dec, true);
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
		  ret = startResyncMove (conn, lastTar.ra, lastTar.dec, true);
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
      if (fabs (locCorRa) < sepLimit->getValueDouble ()
	  && fabs (locCorRa) < sepLimit->getValueDouble ())
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
      maskState (TEL_MASK_MOVING | TEL_MASK_NEED_STOP | TEL_MASK_CORRECTING,
		 TEL_MOVING | TEL_CORRECTING, "move started");
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

void
Rts2DevTelescope::signaledHUP ()
{
  int ret;
  if (modelFile)
    {
      delete model;
      model = new Rts2TelModel (this, modelFile);
      ret = model->load ();
      if (ret)
	{
	  logStream (MESSAGE_ERROR) << "Failed to reload model from file " <<
	    modelFile << sendLog;
	  delete model;
	  model = NULL;
	}
      else
	{
	  logStream (MESSAGE_INFO) << "Reloaded model from file " <<
	    modelFile << sendLog;
	}
    }
  if (modelFile0)
    {
      delete model0;
      model0 = new Rts2TelModel (this, modelFile0);
      ret = model0->load ();
      if (ret)
	{
	  delete model0;
	  logStream (MESSAGE_ERROR) << "Failed to reload model from file " <<
	    modelFile0 << sendLog;
	  model0 = NULL;
	}
      else
	{
	  logStream (MESSAGE_INFO) << "Reloaded model from file " <<
	    modelFile0 << sendLog;
	}
    }
  Rts2Device::signaledHUP ();
}

int
Rts2DevTelescope::commandAuthorized (Rts2Conn * conn)
{
  double tar_ra;
  double tar_dec;

  if (conn->isCommand ("help"))
    {
      conn->send ("ready - is telescope ready?");
      conn->send ("info - information about telescope");
      conn->send ("exit - exit from connection");
      conn->send ("help - print, what you are reading just now");
      return 0;
    }
  else if (conn->isCommand ("move"))
    {
      CHECK_PRIORITY;
      if (conn->paramNextDouble (&tar_ra) || conn->paramNextDouble (&tar_dec)
	  || !conn->paramEnd ())
	return -2;
      modelOn ();
      return startMove (conn, tar_ra, tar_dec, false);
    }
  else if (conn->isCommand ("move_not_model"))
    {
      CHECK_PRIORITY;
      if (conn->paramNextDouble (&tar_ra) || conn->paramNextDouble (&tar_dec)
	  || !conn->paramEnd ())
	return -2;
      modelOff ();
      return startMove (conn, tar_ra, tar_dec, false);
    }
  else if (conn->isCommand ("resync"))
    {
      CHECK_PRIORITY;
      if (conn->paramNextDouble (&tar_ra) || conn->paramNextDouble (&tar_dec)
	  || !conn->paramEnd ())
	return -2;
      return startResyncMove (conn, tar_ra, tar_dec, true);
    }
  else if (conn->isCommand ("fixed"))
    {
      CHECK_PRIORITY;
      // tar_ra hold HA (Hour Angle)
      if (conn->paramNextDouble (&tar_ra) || conn->paramNextDouble (&tar_dec)
	  || !conn->paramEnd ())
	return -2;
      modelOff ();
      return startMoveFixed (conn, tar_ra, tar_dec, false);
    }
  else if (conn->isCommand ("setto"))
    {
      CHECK_PRIORITY;
      if (conn->paramNextDouble (&tar_ra) || conn->paramNextDouble (&tar_dec)
	  || !conn->paramEnd ())
	return -2;
      modelOn ();
      return setTo (conn, tar_ra, tar_dec);
    }
  else if (conn->isCommand ("correct"))
    {
      int cor_mark;
      struct ln_equ_posn realPos;
      double cor_ra;
      double cor_dec;
      if (conn->paramNextInteger (&cor_mark)
	  || conn->paramNextDouble (&cor_ra)
	  || conn->paramNextDouble (&cor_dec)
	  || conn->paramNextDouble (&realPos.ra)
	  || conn->paramNextDouble (&realPos.dec) || !conn->paramEnd ())
	return -2;
      return correct (conn, cor_mark, cor_ra, cor_dec, &realPos);
    }
  else if (conn->isCommand ("park"))
    {
      CHECK_PRIORITY;
      if (!conn->paramEnd ())
	return -2;
      modelOn ();
      return startPark (conn);
    }
  else if (conn->isCommand ("change"))
    {
      CHECK_PRIORITY;
      if (conn->paramNextDouble (&tar_ra) || conn->paramNextDouble (&tar_dec)
	  || !conn->paramEnd ())
	return -2;
      return change (conn, tar_ra, tar_dec);
    }
  else if (conn->isCommand ("save_model"))
    {
      return saveModel (conn);
    }
  else if (conn->isCommand ("load_model"))
    {
      return loadModel (conn);
    }
  else if (conn->isCommand ("worm_stop"))
    {
      return stopWorm (conn);
    }
  else if (conn->isCommand ("worm_start"))
    {
      return startWorm (conn);
    }
  else if (conn->isCommand ("reset"))
    {
      char *param;
      resetStates reset_state;
      CHECK_PRIORITY;
      if (conn->paramEnd ())
	{
	  reset_state = RESET_RESTART;
	}
      else
	{
	  if (conn->paramNextString (&param) || !conn->paramEnd ())
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
      return resetMount (conn, reset_state);
    }
  else if (conn->isCommand ("start_dir"))
    {
      char *dir;
      if (conn->paramNextString (&dir) || !conn->paramEnd ())
	return -2;
      return startDir (dir);
    }
  else if (conn->isCommand ("stop_dir"))
    {
      char *dir;
      if (conn->paramNextString (&dir) || !conn->paramEnd ())
	return -2;
      return stopDir (dir);
    }
  else if (conn->isCommand ("start_guide"))
    {
      char *dir;
      double dir_timeout;
      if (conn->paramNextString (&dir)
	  || conn->paramNextDouble (&dir_timeout) || !conn->paramEnd ())
	return -2;
      return startGuide (*dir, dir_timeout);
    }
  else if (conn->isCommand ("stop_guide"))
    {
      char *dir;
      if (conn->paramNextString (&dir) || !conn->paramEnd ())
	return -2;
      return stopGuide (*dir);
    }
  else if (conn->isCommand ("stop_guide_all"))
    {
      if (!conn->paramEnd ())
	return -2;
      return stopGuideAll ();
    }
  else if (conn->isCommand ("search"))
    {
      double in_radius;
      double in_searchSpeed;
      if (conn->paramNextDouble (&in_radius)
	  || conn->paramNextDouble (&in_searchSpeed) || !conn->paramEnd ())
	return -2;
      return startSearch (conn, in_radius, in_searchSpeed);
    }
  else if (conn->isCommand ("searchstop"))
    {
      if (!conn->paramEnd ())
	return -2;
      return stopSearch ();
    }
  return Rts2Device::commandAuthorized (conn);
}
