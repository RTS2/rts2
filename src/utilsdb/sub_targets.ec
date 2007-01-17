#include <libnova/libnova.h>
#include "target.h"
#include "rts2plan.h"
#include "../utils/rts2config.h"
#include "../utils/timestamp.h"
#include "../utils/infoval.h"

#include <iomanip>
#include <sstream>

EXEC SQL include sqlca;

// ConstTarget
ConstTarget::ConstTarget () : Target ()
{
}

ConstTarget::ConstTarget (int in_tar_id, struct ln_lnlat_posn *in_obs):
Target (in_tar_id, in_obs)
{
}

ConstTarget::ConstTarget (int in_tar_id, struct ln_lnlat_posn *in_obs, struct ln_equ_posn *pos):
Target (in_tar_id, in_obs)
{
  position.ra = pos->ra;
  position.dec = pos->dec;
}

int
ConstTarget::load ()
{
  EXEC SQL BEGIN DECLARE SECTION;
  double d_ra;
  double d_dec;
  int db_tar_id = getObsTargetID ();
  EXEC SQL END DECLARE SECTION;

  EXEC SQL
  SELECT 
    tar_ra,
    tar_dec
  INTO
    :d_ra,
    :d_dec
  FROM
    targets
  WHERE
    tar_id = :db_tar_id;
  if (sqlca.sqlcode)
  {
    logMsgDb ("ConstTarget::load");
    return -1;
  }
  position.ra = d_ra;
  position.dec = d_dec;

  return Target::load ();
}

int
ConstTarget::save (int tar_id)
{
  EXEC SQL BEGIN DECLARE SECTION;
  double d_tar_ra;
  double d_tar_dec;
  int d_tar_id;
  EXEC SQL END DECLARE SECTION;

  int ret;

  ret = Target::save (tar_id);
  if (ret)
    return ret;

  d_tar_ra = position.ra;
  d_tar_dec = position.dec;

  d_tar_id = tar_id;

  EXEC SQL
  UPDATE
    targets
  SET
    tar_ra = :d_tar_ra,
    tar_dec = :d_tar_dec
  WHERE
    tar_id = :d_tar_id;

  if (sqlca.sqlcode)
  {
    logMsgDb ("ConstTarget::save");
    EXEC SQL ROLLBACK;
    return -1;
  }
  EXEC SQL COMMIT;
  return 0;
}

int
ConstTarget::getPosition (struct ln_equ_posn *pos, double JD)
{
  *pos = position;
  return 0;
}

int
ConstTarget::getRST (struct ln_rst_time *rst, double JD, double horizon)
{
  struct ln_equ_posn pos;
  int ret;
  
  ret = getPosition (&pos, JD);
  if (ret)
    return ret;
  return ln_get_object_next_rst_horizon (JD, observer, &pos, horizon, rst);
}

int
ConstTarget::selectedAsGood ()
{
  EXEC SQL BEGIN DECLARE SECTION;
  int d_tar_id = target_id;
  double d_tar_ra;
  double d_tar_dec;
  EXEC SQL END DECLARE SECTION;
  // check if we are still enabled..
  EXEC SQL
  SELECT
    tar_ra,
    tar_dec
  INTO
    :d_tar_ra,
    :d_tar_dec
  FROM
    targets
  WHERE
    tar_id = :d_tar_id;
  if (sqlca.sqlcode)
  {
    logMsgDb ("ConstTarget::selectedAsGood");
    return -1;
  }
  position.ra = d_tar_ra;
  position.dec = d_tar_dec;
  return Target::selectedAsGood ();
}

int
ConstTarget::compareWithTarget (Target * in_target, double in_sep_limit)
{
  struct ln_equ_posn other_position;
  in_target->getPosition (&other_position);
  return (getDistance (&other_position) < in_sep_limit);
}

void
ConstTarget::printExtra (std::ostream &_os, double JD)
{
  Target::printExtra (_os, JD);
}

// EllTarget - good for commets and so on
EllTarget::EllTarget (int in_tar_id, struct ln_lnlat_posn *in_obs):Target (in_tar_id, in_obs)
{
}

int
EllTarget::load ()
{
  EXEC SQL BEGIN DECLARE SECTION;
  double ell_minpause;
  double ell_a;
  double ell_e;
  double ell_i;
  double ell_w;
  double ell_omega;
  double ell_n;
  double ell_JD;
//  double min_m;			// minimal magnitude
  int db_tar_id = getTargetID ();
  EXEC SQL END DECLARE SECTION;

  EXEC SQL SELECT
    EXTRACT(EPOCH FROM ell_minpause),
    ell_a,
    ell_e,
    ell_i,
    ell_w,
    ell_omega,
    ell_n,
    ell_JD
  INTO
    :ell_minpause,
    :ell_a,
    :ell_e,
    :ell_i,
    :ell_w,
    :ell_omega,
    :ell_n,
    :ell_JD
  FROM
    ell
  WHERE
    ell.tar_id = :db_tar_id;
  if (sqlca.sqlcode)
  {
    logMsgDb ("EllTarget::load");
    return -1;
  }
  orbit.a = ell_a;
  orbit.e = ell_e;
  orbit.i = ell_i;
  orbit.w = ell_w;
  orbit.omega = ell_omega;
  orbit.n = ell_n;
  orbit.JD = ell_JD;
  return Target::load ();
}

int
EllTarget::getPosition (struct ln_equ_posn *pos, double JD)
{
  if (orbit.e == 1.0)
    {
      struct ln_par_orbit par_orbit;
      par_orbit.q = orbit.a;
      par_orbit.i = orbit.i;
      par_orbit.w = orbit.w;
      par_orbit.omega = orbit.omega;
      par_orbit.JD = orbit.JD;
      ln_get_par_body_equ_coords (JD, &par_orbit, pos);
      return 0;
    }
  else if (orbit.e > 1.0)
    {
      struct ln_hyp_orbit hyp_orbit;
      hyp_orbit.q = orbit.a;
      hyp_orbit.e = orbit.e;
      hyp_orbit.i = orbit.i;
      hyp_orbit.w = orbit.w;
      hyp_orbit.omega = orbit.omega;
      hyp_orbit.JD = orbit.JD;
      ln_get_hyp_body_equ_coords (JD, &hyp_orbit, pos);
      return 0;
    }
  ln_get_ell_body_equ_coords (JD, &orbit, pos);
  return 0;
}

int
EllTarget::getRST (struct ln_rst_time *rst, double JD, double horizon)
{
  if (orbit.e == 1.0)
    {
      struct ln_par_orbit par_orbit;
      par_orbit.q = orbit.a;
      par_orbit.i = orbit.i;
      par_orbit.w = orbit.w;
      par_orbit.omega = orbit.omega;
      par_orbit.JD = orbit.JD;
      return ln_get_par_body_next_rst_horizon (JD, observer, &par_orbit, horizon, rst);
    }
  else if (orbit.e > 1.0)
    {
      struct ln_hyp_orbit hyp_orbit;
      hyp_orbit.q = orbit.a;
      hyp_orbit.e = orbit.e;
      hyp_orbit.i = orbit.i;
      hyp_orbit.w = orbit.w;
      hyp_orbit.omega = orbit.omega;
      hyp_orbit.JD = orbit.JD;
      return ln_get_hyp_body_next_rst_horizon (JD, observer, &hyp_orbit, horizon, rst);
    }
  return ln_get_ell_body_next_rst_horizon (JD, observer, &orbit, horizon, rst);
}

void
EllTarget::printExtra (std::ostream & _os, double JD)
{
  Target::printExtra (_os, JD);
  _os
    << InfoVal<TimeJD> ("EPOCH", TimeJD (orbit.JD));
  if (orbit.e < 1.0)
  {
    _os 
      << InfoVal<double> ("n", orbit.n)
      << InfoVal<double> ("a", orbit.a);
  }
  else if (orbit.e > 1.0)
  {
    _os
      << InfoVal<double> ("q", orbit.a);
  }  
  _os  
    << InfoVal<double> ("e", orbit.e)
    << InfoVal<double> ("Peri.", orbit.w)
    << InfoVal<double> ("Node", orbit.omega)
    << InfoVal<double> ("Incl.", orbit.i);
}

PossibleDarks::PossibleDarks (DarkTarget *in_target, const char *in_deviceName)
{
  deviceName = new char [strlen (in_deviceName) + 1];
  strcpy (deviceName, in_deviceName);
  target = in_target;
}

PossibleDarks::~PossibleDarks ()
{
  delete[] deviceName;
  dark_exposures.clear ();
}

void
PossibleDarks::addDarkExposure (float exp)
{
  std::list < float >::iterator dark_iter;
  for (dark_iter = dark_exposures.begin (); dark_iter != dark_exposures.end (); dark_iter++)
  {
    if (*dark_iter == exp)
      return;
  }
  dark_exposures.push_back (exp);
}

int
PossibleDarks::defaultDark ()
{
  char dark_exps [1000];
  char *tmp_c;
  char *tmp_s;
  float exp;
  int ret;

  Rts2Config *config;
  config = Rts2Config::instance ();
  ret = config->getString (deviceName, "darks", dark_exps, 1000);
  if (ret)
    {
      return 0;
    }
  // get the getCalledNum th observation
  // count how many exposures are in dark list..
  tmp_s = dark_exps;
  while (*tmp_s)
  {
    // skip blanks..
    while (*tmp_s && isblank (*tmp_s))
      tmp_s++;
    if (!*tmp_s)
      break;
    exp = strtod (tmp_s, &tmp_c);
    if (tmp_s == tmp_c)
    {
      // error occured
      target->logMsg ("PossibleDarks::defaultDark invalid entry");
    }
    else
    {
      // test if it exists..
      addDarkExposure (exp);
    }
    tmp_s = tmp_c;
    if (!*tmp_s)
      break;
    tmp_s++;
  }
  if (dark_exposures.size () == 0)
    dark_exposures.push_back (17);
  return 0;
}

int
PossibleDarks::dbDark ()
{
  EXEC SQL BEGIN DECLARE SECTION;
  VARCHAR d_camera_name[DEVICE_NAME_SIZE];
  float d_img_exposure;
  int d_dark_count;
  EXEC SQL END DECLARE SECTION;

  strncpy (d_camera_name.arr, deviceName, DEVICE_NAME_SIZE);
  d_camera_name.len = strlen (deviceName);

  // find which dark image we should optimally take..

  EXEC SQL DECLARE dark_target CURSOR FOR
    SELECT
        img_exposure,
        (SELECT
          count (*)
        FROM
          darks
        WHERE
            darks.camera_name = images.camera_name
          AND now () - dark_date < '18 hour'
        ) AS dark_count
      FROM
        images,
        darks
      WHERE
	  images.camera_name = :d_camera_name
	AND now () - img_date < '1 day'
	AND now () - dark_date < '1 day'
	AND dark_exposure = img_exposure
      GROUP BY
        img_exposure,
        images.camera_name
    UNION
      SELECT
        img_exposure,
        0
      FROM
        images
      WHERE
          images.camera_name = :d_camera_name
        AND now () - img_date < '1 day'
        AND NOT EXISTS (SELECT *
          FROM
            darks
          WHERE
              darks.camera_name = images.camera_name
            AND dark_exposure = img_exposure
            AND now () - dark_date < '1 day'
	)
      ORDER BY
        img_exposure DESC,
	dark_count DESC;
  EXEC SQL OPEN dark_target;
  if (sqlca.sqlcode)
  {
    target->logMsgDb ("PossibleDarks::getDb cannot open cursor dark_target, get default 10 sec darks");
    EXEC SQL CLOSE dark_target;
    return defaultDark ();
  }
  while (true)
  {
    EXEC SQL FETCH next FROM dark_target INTO
      :d_img_exposure,
      :d_dark_count;
    if (sqlca.sqlcode)
    {
      EXEC SQL CLOSE dark_target;
      EXEC SQL ROLLBACK;
      if (dark_exposures.size () == 0)
      {
        target->logMsgDb ("PossibleDarks::getDb cannot get entry for darks (will use only defaults)");
      }
      break;
    }
    addDarkExposure (d_img_exposure);
  }
  // add default darks..
  defaultDark ();
  return 0;
}

int
PossibleDarks::getScript (char *buf)
{
  char *buf_top;
  std::list <float>::iterator dark_exp;
  if (dark_exposures.size () == 0)
  {
    dbDark ();
  }
  buf_top = buf;
  for (dark_exp = dark_exposures.begin (); dark_exp != dark_exposures.end (); dark_exp++)
  {
    float dark_ex = *dark_exp;
    int len = sprintf (buf_top, "D %f", dark_ex);
    buf_top += len;
    *buf_top = ' ';
    buf_top++;
  }
  if (buf_top != buf)
    *(--buf_top) = '\0';
  return 0;
}

int
PossibleDarks::isName (const char *in_deviceName)
{
  return !strcmp (in_deviceName, deviceName);
}

DarkTarget::DarkTarget (int in_tar_id, struct ln_lnlat_posn *in_obs): Target (in_tar_id, in_obs)
{
  currPos.ra = 0;
  currPos.dec = 0;
}

DarkTarget::~DarkTarget ()
{
  std::list <PossibleDarks *>::iterator dark_iter;
  for (dark_iter = darkList.begin (); dark_iter != darkList.end (); dark_iter++)
  {
    PossibleDarks *darkpos;
    darkpos = *dark_iter;
    delete darkpos;
  }
  darkList.clear ();
}

int
DarkTarget::getScript (const char *deviceName, char *buf)
{
  PossibleDarks *darkEntry = NULL;
  // try to find our script...
  std::list <PossibleDarks *>::iterator dark_iter;
  for (dark_iter = darkList.begin (); dark_iter != darkList.end (); dark_iter++)
  {
    PossibleDarks *darkpos;
    darkpos = *dark_iter;
    if (darkpos->isName (deviceName))
    {
      darkEntry = darkpos;
      break;
    }
  }
  if (!darkEntry)
  {
    darkEntry = new PossibleDarks (this, deviceName);
    darkList.push_back (darkEntry);
  }
  return darkEntry->getScript (buf);
}

int
DarkTarget::getPosition (struct ln_equ_posn *pos, double JD)
{
  *pos = currPos;
  return 0;
}

moveType
DarkTarget::startSlew (struct ln_equ_posn *position)
{
  currPos.ra = position->ra;
  currPos.dec = position->dec;
  // when telescope isn't initialized to point when we get ra | dec - put some defaults
  if (isnan (currPos.ra))
    currPos.ra = -999;
  if (isnan (currPos.dec))
    currPos.dec = -999;
  Target::startSlew (position);
  return OBS_DONT_MOVE;
}

FlatTarget::FlatTarget (int in_tar_id, struct ln_lnlat_posn *in_obs): ConstTarget (in_tar_id, in_obs)
{
}

void
FlatTarget::getAntiSolarPos (struct ln_equ_posn *pos, double JD)
{
  struct ln_equ_posn sun;
  struct ln_hrz_posn hrz;
  ln_get_solar_equ_coords (JD, &sun);
  ln_get_hrz_from_equ (&sun, observer, JD, &hrz);
  hrz.alt = 40;
  hrz.az = ln_range_degrees (hrz.az + 180);
  ln_get_equ_from_hrz (&hrz, observer, JD, pos);
}

int
FlatTarget::getScript (const char *deviceName, char *buf)
{
  int ret;
  ret = ConstTarget::getDBScript (deviceName, buf);
  if (!ret)
    return ret;
  strcpy (buf, "E 1");
  return 0;
}

// we will try to find target, that is among empty fields, and is at oposite location from sun
// that target will then become our target_id, so entries in observation log
// will refer to that id, not to generic flat target_id
int
FlatTarget::load ()
{
  EXEC SQL BEGIN DECLARE SECTION;
  double d_tar_ra;
  double d_tar_dec;
  int d_tar_id;
  const int db_target_flat = TARGET_FLAT;
  EXEC SQL END DECLARE SECTION;

  if (getTargetID () != TARGET_FLAT)
    return ConstTarget::load ();

  double minAntiDist = 1000;
  double curDist;
  struct ln_equ_posn d_tar;
  struct ln_equ_posn antiSolarPosition;
  struct ln_hrz_posn hrz;
  double JD;
  double lst;

  JD = ln_get_julian_from_sys ();
  lst = ln_get_mean_sidereal_time (JD) + observer->lng / 15.0;

  getAntiSolarPos (&antiSolarPosition, JD);

  EXEC SQL DECLARE flat_targets CURSOR FOR
  SELECT
    tar_ra,
    tar_dec,
    tar_id
  FROM
    targets
  WHERE
      type_id = 'f'
    AND (tar_bonus_time is NULL OR tar_bonus > 0)
    AND tar_enabled = true
    AND tar_id <> :db_target_flat;
  EXEC SQL OPEN flat_targets;
  while (1)
    {
      EXEC SQL FETCH next FROM flat_targets INTO
        :d_tar_ra,
	:d_tar_dec,
	:d_tar_id;
      if (sqlca.sqlcode)
        break;
      d_tar.ra = d_tar_ra;
      d_tar.dec = d_tar_dec;
      // we should be at least 10 deg above horizon to be considered..
      ln_get_hrz_from_equ (&d_tar, observer, JD, &hrz);
      if (hrz.alt < 10)
        continue;
      // and of course we should be above horizon..
      if (!isGood (lst, JD, &d_tar))
	continue;
      // test if we found the best target..
      curDist = ln_get_angular_separation (&d_tar, &antiSolarPosition);
      if (curDist < minAntiDist)
        {
          obs_target_id = d_tar_id;
          minAntiDist = curDist;
        }
    }
  if ((sqlca.sqlcode && sqlca.sqlcode != ECPG_NOT_FOUND)
    || obs_target_id <= 0)
    {
      logMsgDb ("FlatTarget::load");
      EXEC SQL CLOSE flat_targets;
      //in that case, we will simply use generic flat target..
      return 0;
    }
  EXEC SQL CLOSE flat_targets;
  return ConstTarget::load ();
}

int
FlatTarget::getPosition (struct ln_equ_posn *pos, double JD)
{
  // we were loaded, or we aren't generic flat target
  if (obs_target_id > 0 || getTargetID () != TARGET_FLAT)
    return ConstTarget::getPosition (pos, JD);
  // generic flat target observations
  getAntiSolarPos (pos, JD);
  return 0;
}

int
FlatTarget::considerForObserving (double JD)
{
  // get new position..when new is available..
  load ();
  // still return considerForObserving..is there is nothing, then get -1 so selector will delete us
  return ConstTarget::considerForObserving (JD);
}

CalibrationTarget::CalibrationTarget (int in_tar_id, struct ln_lnlat_posn *in_obs):ConstTarget (in_tar_id, in_obs)
{
  airmassPosition.ra = airmassPosition.dec = 0;
  time (&lastImage);
  needUpdate = 1;
}

// the idea is to cover uniformly whole sky.
// in airmass_cal_images table we have recorded previous observations
// for frames with astrometry which contains targeted airmass
int
CalibrationTarget::load ()
{
  EXEC SQL BEGIN DECLARE SECTION;
  double d_airmass_start;
  double d_airmass_end;
  long d_airmass_last_image;

  double db_tar_ra;
  double db_tar_dec;
  int db_tar_id;
  char db_type_id;
  VARCHAR db_tar_name[TARGET_NAME_LEN];
  EXEC SQL END DECLARE SECTION;

  double JD = ln_get_julian_from_sys ();

  double lst = ln_get_mean_sidereal_time (JD) + observer->lng / 15.0;

  time_t now;
  time_t valid;

  PosCalibration *fallback_obs_calib = NULL;
  time_t fallback_last_image;

  std::list <PosCalibration *> cal_list;
  std::list <PosCalibration *> bad_list;
  std::list <PosCalibration *>::iterator cal_iter, cal_iter2;
  if (getTargetID () != TARGET_CALIBRATION)
    return ConstTarget::load ();

  // create airmass & target_id pool (I dislike idea of creating
  // target object, as that will cost me a lot of resources
  EXEC SQL DECLARE pos_calibration CURSOR FOR
  SELECT
    tar_ra,
    tar_dec,
    tar_id,
    type_id,
    tar_name
  FROM
    targets
  WHERE
      tar_enabled = true
    AND (tar_bonus_time is NULL OR tar_bonus > 0)
    AND ((tar_next_observable is null) OR (tar_next_observable < now()))
    AND tar_id != 6
    AND (
        type_id = 'c'
      OR type_id = 'M'
    )
  ORDER BY
    tar_priority + tar_bonus desc;
  EXEC SQL OPEN pos_calibration;
  while (1)
  {
    EXEC SQL FETCH next FROM pos_calibration
    INTO
      :db_tar_ra,
      :db_tar_dec,
      :db_tar_id,
      :db_type_id,
      :db_tar_name;
    if (sqlca.sqlcode)
      break;
    if (Rts2Config::instance ()->getObjectChecker ()->is_good (lst, db_tar_ra, db_tar_dec))
    {
      PosCalibration *newCal = new PosCalibration (db_tar_id, db_tar_ra, db_tar_dec, db_type_id,
        db_tar_name.arr, observer, JD);
      cal_list.push_back (newCal);
    }
  }
  if (sqlca.sqlcode != ECPG_NOT_FOUND || cal_list.size () == 0)
  {
    // free cal_list..
    for (cal_iter = cal_list.begin (); cal_iter != cal_list.end ();)
    {
      cal_iter2 = cal_iter;
      cal_iter++;
      delete *cal_iter2;
    }
    cal_list.clear ();
    logMsgDb ("CalibrationTarget::load cannot load any possible target");
    EXEC SQL CLOSE pos_calibration;
    EXEC SQL ROLLBACK;
    return -1;
  }
  EXEC SQL CLOSE pos_calibration;
  EXEC SQL COMMIT;

  // center airmass is 1.5 - when we don't have any images in airmass_cal_images,
  // order us by distance from such center distance
  EXEC SQL DECLARE cur_airmass_cal_images CURSOR FOR
  SELECT
    air_airmass_start,
    air_airmass_end,
    EXTRACT (EPOCH FROM air_last_image)
  FROM
    airmass_cal_images
  ORDER BY
    air_last_image asc,
    abs (1.5 - (air_airmass_start + air_airmass_end) / 2) asc;
  EXEC SQL OPEN cur_airmass_cal_images;
  obs_target_id = -1;
  time (&now);
  valid = now - Rts2Config::instance()->getCalibrationValidTime();
  while (1)
  {
    EXEC SQL FETCH next FROM cur_airmass_cal_images
    INTO
      :d_airmass_start,
      :d_airmass_end,
      :d_airmass_last_image;
    if (sqlca.sqlcode)
      break;
    // find any target which lies within requested airmass range
    for (cal_iter = cal_list.begin (); cal_iter != cal_list.end (); cal_iter++)
    {
      PosCalibration *calib = *cal_iter;
      if (calib->getCurrAirmass () >= d_airmass_start 
	&& calib->getCurrAirmass () < d_airmass_end)
      {
	if (calib->getLunarDistance (JD) < Rts2Config::instance()->getCalibrationLunarDist())
	{
          bad_list.push_back (calib);
	}
	// if that target was already observerd..
	else if (calib->getNumObs (&valid, &now) > 0)
	{
	  // if we do not have any target, pick that one
	  if (fallback_obs_calib == NULL)
	  {
	    fallback_obs_calib = calib;
	    fallback_last_image = (time_t) d_airmass_last_image;
	  }
	}
	else
	{
	  // switch current target coordinates..
	  obs_target_id = calib->getTargetID ();
	  calib->getPosition (&airmassPosition, JD);
	  lastImage = (time_t) d_airmass_last_image;
	  break;
	}
      }
    }
    if (obs_target_id != -1)
      break;
    char *logmsg;
    asprintf (&logmsg, "CalibrationTarget::load cannot find any target for airmass between %f and %f", d_airmass_start, d_airmass_end);
    logMsgDb (logmsg);
    free (logmsg);
  }
  // change priority for bad targets..
  for (cal_iter = bad_list.begin (); cal_iter != bad_list.end (); cal_iter++)
  {
    (*cal_iter)->setNextObservable (JD + 1.0/24.0);
  }
  bad_list.clear ();
  // no target found..try fallback
  if (obs_target_id == -1 && fallback_obs_calib != NULL)
  {
    obs_target_id = fallback_obs_calib->getTargetID ();
    fallback_obs_calib->getPosition (&airmassPosition, JD);
    lastImage = fallback_last_image;
  }
  // free cal_list..
  for (cal_iter = cal_list.begin (); cal_iter != cal_list.end ();)
  {
    cal_iter2 = cal_iter;
    cal_iter++;
    delete *cal_iter2;
  }
  cal_list.clear ();

  // SQL test..
  if (sqlca.sqlcode)
  {
    logMsgDb ("CalibrationTarget::load cannot find any airmass_cal_images entry");
    EXEC SQL CLOSE cur_airmass_cal_images;
    EXEC SQL ROLLBACK;
    return -1;
  }
  EXEC SQL CLOSE cur_airmass_cal_images;
  EXEC SQL COMMIT;
  if (obs_target_id != -1)
  {
    needUpdate = 0;
    return ConstTarget::load ();
  }
  return -1;
}

int
CalibrationTarget::beforeMove ()
{
  // as calibration target can change between time we select it, let's reload us
  if (getTargetID () == TARGET_CALIBRATION && needUpdate)
    load ();
  return ConstTarget::beforeMove ();
}

int
CalibrationTarget::endObservation (int in_next_id)
{
  needUpdate = 1;
  return ConstTarget::endObservation (in_next_id);
}

int
CalibrationTarget::getPosition (struct ln_equ_posn *pos, double JD)
{
  if (obs_target_id <= 0)
  {
    // no target..
    if (target_id == TARGET_CALIBRATION)
      return -1;
    return ConstTarget::getPosition (pos, JD);
  }
  *pos = airmassPosition;
  return 0;
}

int
CalibrationTarget::considerForObserving (double JD)
{
  // load (possibly new target) before considering us..
  load ();
  return ConstTarget::considerForObserving (JD);
}

float
CalibrationTarget::getBonus (double JD)
{
  time_t now;
  time_t t_diff;

  int validTime = Rts2Config::instance()->getCalibrationValidTime();
  int maxDelay = Rts2Config::instance()->getCalibrationMaxDelay();
  float minBonus = Rts2Config::instance()->getCalibrationMinBonus();
  float maxBonus = Rts2Config::instance()->getCalibrationMaxBonus();
  if (obs_target_id <= 0)
    return -1;
  ln_get_timet_from_julian (JD, &now);
  t_diff = now - lastImage;
  // smaller then valid_time is not interesting..
  if (t_diff < validTime)
    return minBonus;
  // greater then MaxDelay is interesting
  else if (t_diff > maxDelay)
    return maxBonus;
  // otherwise linear increase
  return minBonus + ((maxBonus - minBonus) * t_diff / (maxDelay - validTime));
}

int
FocusingTarget::getScript (const char *device_name, char *buf)
{
  buf[0] = COMMAND_FOCUSING;
  buf[1] = 0;
  return 0;
}

ModelTarget::ModelTarget (int in_tar_id, struct ln_lnlat_posn *in_obs):ConstTarget (in_tar_id, in_obs)
{
  modelStepType = 2;
  Rts2Config::instance ()->getInteger ("observatory", "model_step_type", modelStepType);
}

int
ModelTarget::load ()
{
  EXEC SQL BEGIN DECLARE SECTION;
  int d_tar_id = getTargetID ();
  float d_alt_start;
  float d_alt_stop;
  float d_alt_step;
  float d_az_start;
  float d_az_stop;
  float d_az_step;
  float d_noise;
  int d_step;
  EXEC SQL END DECLARE SECTION;

  EXEC SQL
  SELECT
    alt_start,
    alt_stop,
    alt_step,
    az_start,
    az_stop,
    az_step,
    noise,
    step
  INTO
    :d_alt_start,
    :d_alt_stop,
    :d_alt_step,
    :d_az_start,
    :d_az_stop,
    :d_az_step,
    :d_noise,
    :d_step
  FROM
    target_model
  WHERE
    tar_id = :d_tar_id
  ;
  if (sqlca.sqlcode)
  {
    logMsgDb ("ModelTarget::ModelTarget");
    return -1;
  }
  alt_start = d_alt_start;
  alt_stop = d_alt_stop;
  alt_step = d_alt_step;
  az_start = d_az_start;
  az_stop = d_az_stop;
  az_step = d_az_step;
  noise = d_noise;
  step = d_step;

  alt_size = (int) (fabs (alt_stop - alt_start) / alt_step);
  alt_size++;

  srandom (time (NULL));
  calPosition ();
  return ConstTarget::load ();
}

ModelTarget::~ModelTarget (void)
{
}

int
ModelTarget::writeStep ()
{
  EXEC SQL BEGIN DECLARE SECTION;
  int d_tar_id = getTargetID ();
  int d_step = step;
  EXEC SQL END DECLARE SECTION;
  EXEC SQL
  UPDATE
    target_model
  SET
    step = :d_step
  WHERE
    tar_id = :d_tar_id;
  if (sqlca.sqlcode)
  {
    logMsgDb ("ModelTarget::writeStep");
    EXEC SQL ROLLBACK;
    return -1;
  }
  EXEC SQL COMMIT;
  return 0;
}

int
ModelTarget::getNextPosition ()
{
  switch (modelStepType)
  {
    case -1:
      // linear model
      step++;
      break;
    case 0:
      // random model
      step += (int) (((double) random () * ((fabs (360.0 / az_step) + 1) * fabs((alt_stop - alt_start) / alt_step) + 1)) / RAND_MAX);
      break;
    default:  
      step += modelStepType * ((int) fabs ((alt_stop - alt_start) / alt_step) + 1) + 1;
  }
  return calPosition ();
}

int
ModelTarget::calPosition ()
{
  hrz_poz.az = az_start + az_step * (step / alt_size);
  hrz_poz.alt = alt_start + alt_step * (step % alt_size);
  ra_noise = 2 * noise * ((double) random () / RAND_MAX);
  ra_noise -= noise;
  dec_noise = 2 * noise * ((double) random () / RAND_MAX);
  dec_noise -= noise;
  // null ra + dec .. for recurent call do getPosition (JD..)
  equ_poz.ra = -1000;
  equ_poz.dec = -1000;
  return 0;
}

int
ModelTarget::beforeMove ()
{
  endObservation (-1); // we will not observe same model target twice
  nullAcquired ();
  return getNextPosition ();
}

moveType
ModelTarget::afterSlewProcessed ()
{
  EXEC SQL BEGIN DECLARE SECTION;
  int d_obs_id;
  int d_step = step;
  EXEC SQL END DECLARE SECTION;

  d_obs_id = getObsId ();

  EXEC SQL
  INSERT INTO
    model_observation
  (
    obs_id,
    step
  ) VALUES (
    :d_obs_id,
    :d_step
  );
  if (sqlca.sqlcode)
  {
    logMsgDb ("ModelTarget::endObservation");
    EXEC SQL ROLLBACK;
  }
  else
  {
    EXEC SQL COMMIT;
  }
  return OBS_MOVE_UNMODELLED;
}

int
ModelTarget::endObservation (int in_next_id)
{
  if (getObsId () > 0)
    writeStep ();
  return ConstTarget::endObservation (in_next_id);
}

int
ModelTarget::getPosition (struct ln_equ_posn *pos, double JD)
{
  if (equ_poz.ra < -10)
  {
    ln_get_equ_from_hrz (&hrz_poz, observer, JD, &equ_poz);
    equ_poz.ra += ra_noise;
    equ_poz.dec += dec_noise;
  }
  *pos = equ_poz;
  return 0;
}

// pick up some opportunity target; don't pick it too often
OportunityTarget::OportunityTarget (int in_tar_id, struct ln_lnlat_posn *in_obs):ConstTarget (in_tar_id, in_obs)
{
}

float
OportunityTarget::getBonus (double JD)
{
  double retBonus = 0;
  struct ln_hrz_posn hrz;
  double lunarDist;
  double ha;
  double lastObs;
  time_t now;
  time_t start_t;
  getAltAz (&hrz, JD);
  lunarDist = getLunarDistance (JD);
  ha = getHourAngle ();
  lastObs = getLastObsTime ();
  ln_get_timet_from_julian (JD, &now);
  start_t = now - 86400;
  if (lastObs > 3600 && lastObs < (3 * 3600))
    retBonus -= log (lastObs / 3600) * 50;
  else if (lastObs < 86400)
    retBonus -= log (3) * 50;
  if (lunarDist < 60.0)
    retBonus -= log (61 - lunarDist) * 10;
  // bonus for south
  if (ha < 165)
    retBonus += log ((180 - ha) / 15.0);
  if (ha > 195)
    retBonus += log ((ha - 180) / 15.0);
  retBonus += hrz.alt * 2;
  retBonus -= sin (getNumObs (&start_t, &now) * M_PI_4) * 5;
  return ConstTarget::getBonus (JD) + retBonus;
}

// will pickup the Moon
LunarTarget::LunarTarget (int in_tar_id, struct ln_lnlat_posn *in_obs):Target (in_tar_id, in_obs)
{
}

int
LunarTarget::getPosition (struct ln_equ_posn *pos, double JD)
{
  ln_get_lunar_equ_coords (JD, pos);
  return 0;
}

int
LunarTarget::getRST (struct ln_rst_time *rst, double JD, double horizon)
{
  return ln_get_body_rst_horizon (JD, observer, ln_get_lunar_equ_coords, horizon, rst);
}

int
LunarTarget::getScript (const char *deviceName, char *buf)
{
  strcpy (buf, "E 1");
  return 0;
}

TargetGRB::TargetGRB (int in_tar_id, struct ln_lnlat_posn *in_obs, int in_maxBonusTimeout, int in_dayBonusTimeout, int in_fiveBonusTimeout) : 
ConstTarget (in_tar_id, in_obs)
{
  shouldUpdate = 1;
  grb_is_grb = true;
  gcnPacketType = -1;
  maxBonusTimeout = in_maxBonusTimeout;
  dayBonusTimeout = in_dayBonusTimeout;
  fiveBonusTimeout = in_fiveBonusTimeout;

  grb.ra = nan("f");
  grb.dec = nan("f");
  errorbox = nan("f");
}

int
TargetGRB::load ()
{
  EXEC SQL BEGIN DECLARE SECTION;
  double  db_grb_date;
  double  db_grb_last_update;
  int db_grb_type;
  int db_grb_id;
  bool db_grb_is_grb;
  int db_tar_id = getTargetID ();
  double db_grb_ra;
  double db_grb_dec;
  float db_grb_errorbox;
  int db_grb_errorbox_ind;
  EXEC SQL END DECLARE SECTION;

  EXEC SQL SELECT
    EXTRACT (EPOCH FROM grb_date),
    EXTRACT (EPOCH FROM grb_last_update),
    grb_type,
    grb_id,
    grb_is_grb,
    grb_ra,
    grb_dec,
    grb_errorbox
  INTO
    :db_grb_date,
    :db_grb_last_update,
    :db_grb_type,
    :db_grb_id,
    :db_grb_is_grb,
    :db_grb_ra,
    :db_grb_dec,
    :db_grb_errorbox :db_grb_errorbox_ind
  FROM
    grb
  WHERE
    tar_id = :db_tar_id;
  if (sqlca.sqlcode)
  {
    logMsgDb ("TargetGRB::load");
    return -1;
  }
  grbDate = db_grb_date; 
  // we don't expect grbDate to change much during observation,
  // so we will not update that in beforeMove (or somewhere else)
  lastUpdate = db_grb_last_update;
  gcnPacketType = db_grb_type;
  // switch of packet type - packet class
  if (gcnPacketType >= 40 && gcnPacketType <= 45)
  {
    // HETE burst
    gcnPacketMin = 40;
    gcnPacketMax = 45;
  }
  else if (gcnPacketType >= 50 && gcnPacketType <= 55)
  {
    // INTEGRAL burst
    gcnPacketMin = 50;
    gcnPacketMax = 55;
  }
  else if (gcnPacketType >= 60 && gcnPacketType <= 90)
  {
    // SWIFT burst
    gcnPacketMin = 60;
    gcnPacketMax = 90;
  }
  else if (gcnPacketType >= 110 && gcnPacketType <= 150)
  {
    // GLAST
    gcnPacketMin = 110;
    gcnPacketMax = 150;
  }
  else
  {
    // UNKNOW
    gcnPacketMin = 0;
    gcnPacketMax = 1000;
  }

  gcnGrbId = db_grb_id;
  grb_is_grb = db_grb_is_grb;
  grb.ra = db_grb_ra;
  grb.dec = db_grb_dec;
  if (db_grb_errorbox_ind)
    errorbox = nan("f");
  else
    errorbox = db_grb_errorbox;
  shouldUpdate = 0;
  return ConstTarget::load ();
}

int
TargetGRB::getPosition (struct ln_equ_posn *pos, double JD)
{
  time_t now;
  ln_get_timet_from_julian (JD, &now);
  // it's SWIFT ALERT, ra&dec are S/C ra&dec
  if (gcnPacketType == 60 && (now - lastUpdate < 3600))
  {
    struct ln_hrz_posn hrz;
    getAltAz (&hrz, JD);
    if (hrz.alt > -20 && hrz.alt < 35)
    {
      hrz.alt = 35;
      ln_get_equ_from_hrz (&hrz, observer, JD, pos);
      return 0;
    }
  }
  return ConstTarget::getPosition (pos, JD);
}

int
TargetGRB::compareWithTarget (Target *in_target, double in_sep_limit)
{
  // when it's SWIFT GRB and we are currently observing Swift burst, don't interrupt us
  // that's good only for WF instruments which observe Swift FOV
  if (in_target->getTargetID () == TARGET_SWIFT_FOV && gcnPacketType >= 60 && gcnPacketType <= 90)
  {
    return ConstTarget::compareWithTarget (in_target, 4 * in_sep_limit);
  }
  return ConstTarget::compareWithTarget (in_target, in_sep_limit);
}

int
TargetGRB::getDBScript (const char *camera_name, char *script)
{
  EXEC SQL BEGIN DECLARE SECTION;
  int d_post_sec;
  int d_pos_ind;
  VARCHAR sc_script[2000];
  VARCHAR d_camera_name[8];
  EXEC SQL END DECLARE SECTION;

  time_t now;
  int ret;
  int post_sec;

  // use target script, if such exist..
  ret = ConstTarget::getDBScript (camera_name, script);
  if (!ret)
    return ret;

  time (&now);

  d_camera_name.len = strlen (camera_name);
  strncpy (d_camera_name.arr, camera_name, d_camera_name.len);

  post_sec = now - (time_t) grbDate;

  // grb_script_script is NOT NULL

  EXEC SQL DECLARE find_grb_script CURSOR FOR
    SELECT
      grb_script_end,
      grb_script_script
    FROM
      grb_script
    WHERE
      camera_name = :d_camera_name
    ORDER BY
      grb_script_end ASC;
  EXEC SQL OPEN find_grb_script;
  while (1)
  {
    EXEC SQL FETCH next FROM find_grb_script INTO
      :d_post_sec :d_pos_ind,
      :sc_script;
    if (sqlca.sqlcode)
      break;
    if (post_sec < d_post_sec || d_pos_ind < 0)
    {
      strncpy (script, sc_script.arr, sc_script.len);
      script[sc_script.len] = '\0';
      break;
    }
  }
  if (sqlca.sqlcode)
    {
      logMsgDb ("TargetGRB::getDBScript database error");
      script[0] = '\0';
      EXEC SQL CLOSE find_grb_script;
      EXEC SQL ROLLBACK;
      return -1;
    }
  strncpy (script, sc_script.arr, sc_script.len);
  script[sc_script.len] = '\0';
  EXEC SQL CLOSE find_grb_script;
  EXEC SQL COMMIT;
  return 0;
}

int
TargetGRB::getScript (const char *deviceName, char *buf)
{
  int ret;
  time_t now;

  // try to find values first in DB..
  ret = getDBScript (deviceName, buf);
  if (!ret)
    return ret;

  time (&now);
  
  // switch based on time after burst..
  // that one has to be hard-coded, there is no way how to
  // specify it from config (there can be some, but it will be rather
  // horible way)

  if (now - (time_t) grbDate < 1000)
  {
    strcpy (buf, "E 10 E 20 E 30 E 40");
  }
  else if (now - (time_t) grbDate < 10000)
  {
    strcpy (buf, "E 100 E 200");
  }
  else
  {
    strcpy (buf, "E 300");
  }
  return 0;
}

int
TargetGRB::beforeMove ()
{
  // update our position..
  if (shouldUpdate)
    endObservation (-1);
  load ();
  return 0;
}

float
TargetGRB::getBonus (double JD)
{
  // time from GRB
  time_t now;
  float retBonus;
  ln_get_timet_from_julian (JD, &now);
  // special SWIFT handling..
  // last packet we have is SWIFT_ALERT..
  switch (gcnPacketType)
    {
      case 60:
        // we don't get ACK | position within 1 hour..drop our priority back to some mimimum value
        if (now - (time_t) grbDate > 3600)
          return 1;
        break;
      case 62:
        if (now - lastUpdate > 1800)
          return -1;
	break;
    }
  if (now - (time_t) grbDate < maxBonusTimeout)
  {
    // < 1 hour fixed boost to be sure to not miss it
    retBonus = ConstTarget::getBonus (JD) + 2000;
  }
  else if (now - (time_t) grbDate < dayBonusTimeout)
  {
    // < 24 hour post burst
    retBonus = ConstTarget::getBonus (JD)
      + 1000.0 * cos ((double)((double)(now - (time_t) grbDate) / (2.0*86400.0))) 
      + 10.0 * cos ((double)((double)(now - lastUpdate) / (2.0*86400.0)));
  }
  else if (now - (time_t) grbDate < fiveBonusTimeout)
  {
    // < 5 days post burst - add some time after last observation
    retBonus = ConstTarget::getBonus (JD)
      + 10.0 * sin (getLastObsTime () / 3600.0);
  }
  else
  {
    // default
    retBonus = ConstTarget::getBonus (JD);
  }
  // if it was not GRB..
  if (grb_is_grb == false)
    retBonus /= 2;
  return retBonus;
}

int
TargetGRB::isContinues ()
{
  EXEC SQL BEGIN DECLARE SECTION;
  int db_tar_id = getTargetID ();
  double db_tar_ra;
  double db_tar_dec;
  EXEC SQL END DECLARE SECTION;

  struct ln_equ_posn pos;

  // once we detect need to update, we need to update - stop the observation and start new
  if (shouldUpdate)
    return 0;

  EXEC SQL SELECT
    tar_ra,
    tar_dec
  INTO
    :db_tar_ra,
    :db_tar_dec
  FROM
    grb,
    targets
  WHERE
      targets.tar_id = :db_tar_id
    AND targets.tar_id = grb.tar_id;

  if (sqlca.sqlcode)
  {
    logMsgDb ("TargetGRB::isContinues");
    return -1;
  }
  getPosition (&pos, ln_get_julian_from_sys ());
  if (pos.ra == db_tar_ra && pos.dec == db_tar_dec)
    return 1;
  // we get big update, lets synchronize again..  
  shouldUpdate = 1;
  return 0;
}

double
TargetGRB::getFirstPacket ()
{
  EXEC SQL BEGIN DECLARE SECTION;
  int db_grb_id = gcnGrbId;
  int db_grb_type_min = gcnPacketMin;
  int db_grb_type_max = gcnPacketMax;
  long db_grb_update;
  long db_grb_update_usec;
  EXEC SQL END DECLARE SECTION;
  EXEC SQL DECLARE cur_grb_first_packet CURSOR FOR
  SELECT
    EXTRACT (EPOCH FROM grb_update),
    grb_update_usec
  FROM
    grb_gcn
  WHERE
      grb_id = :db_grb_id
    AND grb_type BETWEEN :db_grb_type_min AND :db_grb_type_max
  ORDER BY
    grb_update asc,
    grb_update_usec asc;
  EXEC SQL OPEN cur_grb_first_packet;
  EXEC SQL FETCH next FROM cur_grb_first_packet INTO
    :db_grb_update,
    :db_grb_update_usec;
  if (sqlca.sqlcode)
  {
    EXEC SQL CLOSE cur_grb_first_packet;
    EXEC SQL ROLLBACK;
    return nan("f");
  }
  EXEC SQL CLOSE cur_grb_first_packet;
  EXEC SQL ROLLBACK;
  return db_grb_update + (double) db_grb_update_usec / USEC_SEC;
}

const char *
TargetGRB::getSatelite ()
{
  // get satelite
  if (gcnPacketMin == 40)
    return "HETE BURST ";
  else if (gcnPacketMin == 50)
    return "INTEGRAL BURST ";
  else if (gcnPacketMin == 60)
  {
    if (gcnPacketType >= 81)
      return "SWIFT BURST (UVOT)";
    if (gcnPacketType >= 67)
      return "SWIFT BURST (XRT)";
    return "SWIFT BURST (BAT)";
  }
  else if (gcnPacketMin == 110)
  {
    return "GLAST";
  }
  return "Unknow type";
}

void
TargetGRB::printExtra (std::ostream &_os, double JD)
{
  double firstPacket = getFirstPacket ();
  double firstObs = getFirstObs ();
  ConstTarget::printExtra (_os, JD);
  _os 
    << getSatelite () << std::endl
    << InfoVal<int> ("TYPE", gcnPacketType)
    << (grb_is_grb ? "IS GRB flag is set" : "not GRB - is grb flag is not set") << std::endl
    << InfoVal<Timestamp> ("GRB DATE", Timestamp (grbDate))
    << InfoVal<Timestamp> ("GCN FIRST PACKET", Timestamp (firstPacket))
    << InfoVal<TimeDiff> ("GRB->GCN", TimeDiff (grbDate, firstPacket))
    << InfoVal<Timestamp> ("GCN LAST UPDATE", Timestamp (lastUpdate))
    << InfoVal<LibnovaRaJ2000> ("GRB RA", LibnovaRaJ2000 (grb.ra))
    << InfoVal<LibnovaDecJ2000> ("GRB DEC", LibnovaDecJ2000 (grb.dec))
    << InfoVal<LibnovaDeg> ("GRB ERR", LibnovaDeg (errorbox));
  // get information about obsering time..
  if (isnan (firstObs))
  {
    _os << "GRB without observation";
  }
  else
  {
    _os 
      << InfoVal<Timestamp> ("FIRST OBSERVATION", Timestamp (firstObs))
      << InfoVal<TimeDiff> ("GRB delta", TimeDiff (grbDate, firstObs))
      << InfoVal<TimeDiff> ("GCN delta", TimeDiff (firstPacket, firstObs));
  }
  _os << std::endl;
}

void
TargetGRB::printGrbList (std::ostream & _os)
{
  struct ln_equ_posn pos;
  double firstObs = getFirstObs ();
  getPosition (&pos, ln_get_julian_from_sys ());
  _os
    << std::setw(5) << getTargetID () << SEP
    << LibnovaRa (pos.ra) << SEP 
    << LibnovaDec (pos.dec) << SEP
    << Timestamp (grbDate) << SEP
    << std::setw(15) << TimeDiff (grbDate, firstObs) << SEP
    << std::setw(15) << TimeDiff (getFirstPacket (), firstObs) << SEP
    << Timestamp (getLastObs ())
    << std::endl;
}

void
TargetGRB::writeToImage (Rts2Image * image)
{
  ConstTarget::writeToImage (image);
  image->setValue ("GRB_RA", grb.ra, "GRB RA know at time of exposure");
  image->setValue ("GRB_DEC", grb.dec, "GRB DEC know at time of exposure");
  image->setValue ("GRB_ERR", errorbox, "GRB errorbox diameter");
}

TargetSwiftFOV::TargetSwiftFOV (int in_tar_id, struct ln_lnlat_posn *in_obs):Target (in_tar_id, in_obs)
{
  swiftOnBonus = 0;
  target_id = TARGET_SWIFT_FOV;
  swiftId = -1;
  oldSwiftId = -1;
  swiftName = NULL;
  target_name = new char[200];
}

TargetSwiftFOV::~TargetSwiftFOV (void)
{
  delete[] swiftName;
}

int
TargetSwiftFOV::load ()
{
  struct ln_hrz_posn testHrz;
  struct ln_equ_posn testEqu;
  double JD = ln_get_julian_from_sys ();
  int ret;

  EXEC SQL BEGIN DECLARE SECTION;
  int d_swift_id = -1;
  double d_swift_ra;
  double d_swift_dec;
  double d_swift_roll;
  int d_swift_roll_null;
  int d_swift_time;
  float d_swift_obstime;
  VARCHAR d_swift_name[70];
  EXEC SQL END DECLARE SECTION;

  swiftId = -1;

  ret = Target::load();
  if (ret)
    return ret;

  EXEC SQL DECLARE find_swift_poiniting CURSOR FOR
    SELECT
      swift_id,
      swift_ra,
      swift_dec,
      swift_roll,
      EXTRACT (EPOCH FROM swift_time),
      swift_obstime,
      swift_name
    FROM
      swift
    WHERE
        swift_time is not NULL
      AND swift_obstime is not NULL
    ORDER BY
      swift_id desc;
  EXEC SQL OPEN find_swift_poiniting;
  while (1)
  {
    EXEC SQL FETCH next FROM find_swift_poiniting INTO
      :d_swift_id,
      :d_swift_ra,
      :d_swift_dec,
      :d_swift_roll :d_swift_roll_null,
      :d_swift_time,
      :d_swift_obstime,
      :d_swift_name;
    if (sqlca.sqlcode)
      break;
    // check for our altitude..
    testEqu.ra = d_swift_ra;
    testEqu.dec = d_swift_dec;
    ln_get_hrz_from_equ (&testEqu, observer, JD, &testHrz);
    if (testHrz.alt > 0)
    {
      if (testHrz.alt < 30)
      {
        testHrz.alt = 30;
      // get equ coordinates we will observe..
        ln_get_equ_from_hrz (&testHrz, observer, JD, &testEqu);
      }	
      swiftFovCenter.ra = testEqu.ra;
      swiftFovCenter.dec = testEqu.dec;
      if (oldSwiftId == -1)
        oldSwiftId = d_swift_id;
      swiftId = d_swift_id;
      swiftTimeStart = d_swift_time;
      swiftTimeEnd = d_swift_time + (int) d_swift_obstime;
      break;
    }
  }
  EXEC SQL CLOSE find_swift_poiniting;
  if (swiftId < 0)
  {
    setTargetName ("Cannot find any Swift FOV");
    swiftFovCenter.ra = nan ("f");
    swiftFovCenter.dec = nan ("f");
    swiftTimeStart = 0;
    swiftTimeEnd = 0;
    swiftRoll = nan ("f");
    return 0;
  }
  if (d_swift_roll_null)
    swiftRoll = nan ("f");
  else
    swiftRoll = d_swift_roll;
  delete[] swiftName;

  swiftName = new char[d_swift_name.len + 1];
  strncpy (swiftName, d_swift_name.arr, d_swift_name.len);
  swiftName[d_swift_name.len] = '\0';

  std::ostringstream name;
  name << "SwiftFOV #" << swiftId 
  << " ( " << Timestamp(swiftTimeStart)
  << " - " << Timestamp(swiftTimeEnd) << " )";
  setTargetName (name.str().c_str());
  return 0;
}

int
TargetSwiftFOV::getPosition (struct ln_equ_posn *pos, double JD)
{
  *pos = swiftFovCenter;
  return 0;
}

int
TargetSwiftFOV::getRST (struct ln_rst_time *rst, double JD, double horizon)
{
  struct ln_equ_posn pos;
  int ret;
  
  ret = getPosition (&pos, JD);
  if (ret)
    return ret;
  return ln_get_object_next_rst_horizon (JD, observer, &pos, horizon, rst);
}

moveType
TargetSwiftFOV::afterSlewProcessed ()
{
  EXEC SQL BEGIN DECLARE SECTION;
  int d_obs_id;
  int d_swift_id = swiftId;
  EXEC SQL END DECLARE SECTION;

  d_obs_id = getObsId ();
  EXEC SQL
  INSERT INTO
    swift_observation
  (
    swift_id,
    obs_id
  ) VALUES (
    :d_swift_id,
    :d_obs_id
  );
  if (sqlca.sqlcode)
  {
    logMsgDb ("TargetSwiftFOV::startSlew SQL error");
    EXEC SQL ROLLBACK;
    return OBS_MOVE_FAILED;
  }
  EXEC SQL COMMIT;
  return OBS_MOVE;
}

int
TargetSwiftFOV::considerForObserving (double JD)
{
  // find pointing
  int ret;
  struct ln_equ_posn curr_position;
  double lst = ln_get_mean_sidereal_time (JD) + observer->lng / 15.0;

  load ();

  if (swiftId < 0)
  {
    // no pointing..expect that after 3 minutes, it will be better,
    // as we can get new pointing from GCN
    setNextObservable (JD + 1/1440.0/20.0);
    return -1;
  }

  ret = getPosition (&curr_position, JD);
  if (ret)
  {
    setNextObservable (JD + 1/1440.0/20.0);
    return -1;
  }

  ret = isGood (lst, JD, &curr_position);
  
  if (!ret)
  {
    time_t nextObs;
    time (&nextObs);
    if (nextObs > swiftTimeEnd) // we found pointing that already
    				  // happens..hope that after 2
				  // minutes, we will get better
				  // results
    {
      nextObs += 2 * 60;
    }
    else
    {
      nextObs = swiftTimeEnd;
    }
    setNextObservable (&nextObs);
    return -1;
  }

  return selectedAsGood ();
}

int
TargetSwiftFOV::beforeMove ()
{
  // are we still the best swiftId on planet?
  load ();
  if (oldSwiftId != swiftId)
    endObservation (-1);  // startSlew will be called after move suceeded and will write new observation..
  return 0;
}

float
TargetSwiftFOV::getBonus (double JD)
{
  EXEC SQL BEGIN DECLARE SECTION;
  int d_tar_id = target_id;
  double d_bonus;
  EXEC SQL END DECLARE SECTION;

  EXEC SQL
  SELECT
    tar_priority
  INTO
    :d_bonus
  FROM
    targets
  WHERE
    tar_id = :d_tar_id; 
  
  swiftOnBonus = d_bonus;

  time_t now;
  ln_get_timet_from_julian (JD, &now);
  if (now > swiftTimeStart - 120 && now < swiftTimeEnd + 120)
    return swiftOnBonus;
  if (now > swiftTimeStart - 300 && now < swiftTimeEnd + 300)
    return swiftOnBonus / 2.0;
  return 1;
}

int
TargetSwiftFOV::isContinues ()
{
  double ret;
  load ();
  if (oldSwiftId != swiftId)
    return 0;
  ret = Target::getBonus ();
  // FOV become uinteresting..
  if (ret == 1)
    return 0;
  return 1;
}

void
TargetSwiftFOV::printExtra (std::ostream &_os, double JD)
{
  Target::printExtra (_os, JD);
  _os 
  << InfoVal<const char *> ("NAME", swiftName)
  << InfoVal<int> ("SwiftFOW ID", swiftId)
  << InfoVal<Timestamp> ("FROM", Timestamp (swiftTimeStart))
  << InfoVal<Timestamp> ("TO", Timestamp (swiftTimeEnd))
  << InfoVal<double> ("ROLL", swiftRoll);
}

TargetIntegralFOV::TargetIntegralFOV (int in_tar_id, struct ln_lnlat_posn *in_obs):Target (in_tar_id, in_obs)
{
  target_id = TARGET_INTEGRAL_FOV;
  integralId = -1;
  oldIntegralId = -1;
  integralOnBonus = 0;
}

TargetIntegralFOV::~TargetIntegralFOV (void)
{
}

int
TargetIntegralFOV::load ()
{
  struct ln_hrz_posn testHrz;
  struct ln_equ_posn testEqu;
  double JD = ln_get_julian_from_sys ();

  EXEC SQL BEGIN DECLARE SECTION;
  int d_integral_id = -1;
  double d_integral_ra;
  double d_integral_dec;
  long d_integral_time;
  EXEC SQL END DECLARE SECTION;

  int ret;

  integralId = -1;

  ret = Target::load();
  if (ret)
    return ret;

  EXEC SQL DECLARE find_integral_poiniting CURSOR FOR
    SELECT
      integral_id,
      integral_ra,
      integral_dec,
      EXTRACT (EPOCH FROM integral_time)
    FROM
      integral
    WHERE
        integral_time is not NULL
    ORDER BY
      integral_id desc;
  EXEC SQL OPEN find_integral_poiniting;
  while (1)
  {
    EXEC SQL FETCH next FROM find_integral_poiniting INTO
      :d_integral_id,
      :d_integral_ra,
      :d_integral_dec,
      :d_integral_time;
    if (sqlca.sqlcode)
      break;
    // check for our altitude..
    testEqu.ra = d_integral_ra;
    testEqu.dec = d_integral_dec;
    ln_get_hrz_from_equ (&testEqu, observer, JD, &testHrz);
    if (testHrz.alt > 0)
    {
      if (testHrz.alt < 30)
      {
        testHrz.alt = 30;
      // get equ coordinates we will observe..
        ln_get_equ_from_hrz (&testHrz, observer, JD, &testEqu);
      }	
      integralFovCenter.ra = testEqu.ra;
      integralFovCenter.dec = testEqu.dec;
      if (oldIntegralId == -1)
        oldIntegralId = d_integral_id;
      integralId = d_integral_id;
      integralTimeStart = d_integral_time;
      break;
    }
  }
  EXEC SQL CLOSE find_integral_poiniting;
  if (integralId < 0)
  {
    setTargetName ("Cannot find any INTEGRAL FOV");
    integralFovCenter.ra = nan ("f");
    integralFovCenter.dec = nan ("f");
    integralTimeStart = 0;
    return 0;
  }

  std::ostringstream name;
  name << "IntegralFOV #" << integralId
  << " ( " << Timestamp(integralTimeStart) << " )";
  setTargetName (name.str().c_str());
  return 0;
}

int
TargetIntegralFOV::getPosition (struct ln_equ_posn *pos, double JD)
{
  *pos = integralFovCenter;
  return 0;
}

int
TargetIntegralFOV::getRST (struct ln_rst_time *rst, double JD, double horizon)
{
  struct ln_equ_posn pos;
  int ret;
  
  ret = getPosition (&pos, JD);
  if (ret)
    return ret;
  return ln_get_object_next_rst_horizon (JD, observer, &pos, horizon, rst);
}

moveType
TargetIntegralFOV::afterSlewProcessed ()
{
  EXEC SQL BEGIN DECLARE SECTION;
  int d_obs_id;
  int d_integral_id = integralId;
  EXEC SQL END DECLARE SECTION;

  d_obs_id = getObsId ();
  EXEC SQL
  INSERT INTO
    integral_observation
  (
    integral_id,
    obs_id
  ) VALUES (
    :d_integral_id,
    :d_obs_id
  );
  if (sqlca.sqlcode)
  {
    logMsgDb ("TargetIntegralFOV::startSlew SQL error");
    EXEC SQL ROLLBACK;
    return OBS_MOVE_FAILED;
  }
  EXEC SQL COMMIT;
  return OBS_MOVE;
}

int
TargetIntegralFOV::considerForObserving (double JD)
{
  // find pointing
  int ret;
  struct ln_equ_posn curr_position;
  double lst = ln_get_mean_sidereal_time (JD) + observer->lng / 15.0;

  load ();

  if (integralId < 0)
  {
    // no pointing..expect that after 3 minutes, it will be better,
    // as we can get new pointing from GCN
    setNextObservable (JD + 1/1440.0/20.0);
    return -1;
  }

  ret = getPosition (&curr_position, JD);
  if (ret)
  {
    setNextObservable (JD + 1/1440.0/20.0);
    return -1;
  }

  ret = isGood (lst, JD, &curr_position);
  
  if (!ret)
  {
    time_t nextObs;
    time (&nextObs);
    nextObs += 120;
    setNextObservable (&nextObs);
    return -1;
  }

  return selectedAsGood ();
}

int
TargetIntegralFOV::beforeMove ()
{
  // are we still the best swiftId on planet?
  load ();
  if (oldIntegralId != integralId)
    endObservation (-1);  // startSlew will be called after move suceeded and will write new observation..
  return 0;
}

float
TargetIntegralFOV::getBonus (double JD)
{
  EXEC SQL BEGIN DECLARE SECTION;
  int d_tar_id = target_id;
  double d_bonus;
  EXEC SQL END DECLARE SECTION;

  EXEC SQL
  SELECT
    tar_priority
  INTO
    :d_bonus
  FROM
    targets
  WHERE
    tar_id = :d_tar_id; 
  
  integralOnBonus = d_bonus;

  time_t now;
  ln_get_timet_from_julian (JD, &now);
  if (now > integralTimeStart - 120)
    return integralOnBonus;
  if (now > integralTimeStart - 300)
    return integralOnBonus / 2.0;
  return 1;
}

int
TargetIntegralFOV::isContinues ()
{
  double ret;
  load ();
  if (oldIntegralId != integralId)
    return 0;
  ret = Target::getBonus ();
  // FOV become uinteresting..
  if (ret == 1)
    return 0;
  return 1;
}

void
TargetIntegralFOV::printExtra (std::ostream &_os, double JD)
{
  Target::printExtra (_os, JD);
  _os 
  << InfoVal<Timestamp> ("FROM", Timestamp (integralTimeStart));
}


TargetGps::TargetGps (int in_tar_id, struct ln_lnlat_posn *in_obs): ConstTarget (in_tar_id, in_obs)
{
}

float
TargetGps::getBonus (double JD)
{
  // get our altitude..
  struct ln_hrz_posn hrz;
  struct ln_equ_posn curr;
  int numobs;
  int numobs2;
  time_t now;
  time_t start_t;
  time_t start_t2;
  double gal_ctr;
  getAltAz (&hrz, JD);
  getPosition (&curr, JD);
  // get number of observations in last 24 hours..
  ln_get_timet_from_julian (JD, &now);
  start_t = now - 86400;
  start_t2 = now - 86400 * 2;
  numobs = getNumObs (&start_t, &now);
  numobs2 = getNumObs (&start_t2, &start_t);
  gal_ctr = getGalCenterDist (JD);
  if ((90 - observer->lat + curr.dec) == 0)
    return ConstTarget::getBonus (JD) - 200;
  return ConstTarget::getBonus (JD) + 20 * (hrz.alt / (90 - observer->lat + curr.dec)) + gal_ctr / 9 - numobs * 10 - numobs2 * 5;
}

TargetSkySurvey::TargetSkySurvey (int in_tar_id, struct ln_lnlat_posn *in_obs): ConstTarget (in_tar_id, in_obs)
{
}

float
TargetSkySurvey::getBonus (double JD)
{
  time_t now;
  time_t start_t;
  int numobs;
  ln_get_timet_from_julian (JD, &now);
  now -= 300;
  start_t = now - 86400;
  numobs = getNumObs (&start_t, &now);
  return ConstTarget::getBonus (JD) - numobs * 10;
}

TargetTerestial::TargetTerestial (int in_tar_id, struct ln_lnlat_posn *in_obs):ConstTarget (in_tar_id, in_obs)
{
}

int
TargetTerestial::considerForObserving (double JD)
{
  // we can obsere it any time..
  return selectedAsGood ();
}

float
TargetTerestial::getBonus (double JD)
{
  time_t now;
  time_t start_t;
  struct tm *now_t;
  int numobs;
  int minofday;
  ln_get_timet_from_julian (JD, &now);
  now_t = gmtime (&now);

  minofday = now_t->tm_hour * 60 + now_t->tm_min;
  // HAM times (all UTC) (min of day)
  // 1:30 - 3:30          90 - 210
  // 4:00 - 6:00         240 - 360 
  // 6:30 - 8:30         390 - 510
  // not correct times..
  if (minofday < 100
    || (minofday > 200 && minofday < 250)
    || (minofday > 350 && minofday < 400)
    || minofday > 500)
    return 1;

  // we can observe..
  
  start_t = now - 3600;
  numobs = getNumObs (&start_t, &now);

  if (numobs == 0)
    return 600;
  if (numobs == 1)
    return 1;
  return 1;
}

moveType
TargetTerestial::afterSlewProcessed ()
{
  return OBS_MOVE_FIXED;
}

TargetPlan::TargetPlan (int in_tar_id, struct ln_lnlat_posn *in_obs) : Target (in_tar_id, in_obs)
{
  selectedPlan = NULL;
  nextPlan = NULL;
  hourLastSearch = 16.0;
  Rts2Config::instance ()->getFloat ("selector", "last_search", hourLastSearch);
  hourConsiderPlans = 1.0;
  Rts2Config::instance ()->getFloat ("selector", "consider_plan", hourConsiderPlans);
  nextTargetRefresh = 0;
}

TargetPlan::~TargetPlan (void)
{
  delete selectedPlan;
  delete nextPlan;
}

// refresh next time..
void
TargetPlan::refreshNext ()
{
  EXEC SQL BEGIN DECLARE SECTION;
  int db_next_plan_id;
  long db_next_plan_start;
  long db_next;
  EXEC SQL END DECLARE SECTION;

  time_t now;
  int ret;

  time (&now);
  if (now < nextTargetRefresh)
    return;

  // next refresh after 60 seconds
  nextTargetRefresh = now + 60;

  if (nextPlan)
  {
    // we need to select new plan..don't bother with db select
    if (nextPlan->getPlanStart () < now)
      return;
  }
  // consider targets 1/2 hours old..
  db_next = now - 1800;

  EXEC SQL
  SELECT
    plan_id,
    EXTRACT (EPOCH FROM plan_start)
  INTO
    :db_next_plan_id,
    :db_next_plan_start
  FROM
    plan
  WHERE
    plan_start = (SELECT min(plan_start) FROM plan WHERE plan_start >= abstime (:db_next) AND obs_id IS NULL);
  if (sqlca.sqlcode)
  {
    logMsgDb ("TargetPlan::refreshNext cannot load next target");
    EXEC SQL ROLLBACK;
    // we can be last plan entry - so change us, so selectedTarget will be changed
    return;
  }
  EXEC SQL ROLLBACK;
  // we loaded same plan as already prepared
  if (nextPlan && nextPlan->getPlanId() == db_next_plan_id && nextPlan->getPlanStart() == db_next_plan_start)
    return;
  delete nextPlan;
  nextPlan = new Rts2Plan (db_next_plan_id);
  ret = nextPlan->load ();
  if (ret)
  {
    delete nextPlan;
    nextPlan = NULL;
    return;
  }
  // we will need change
  return;
}

int
TargetPlan::load ()
{
  return load (ln_get_julian_from_sys ());
}

int
TargetPlan::load (double JD)
{
  EXEC SQL BEGIN DECLARE SECTION;
  int db_cur_plan_id;
  double db_cur_plan_start;
  int db_obs_id;
  int db_obs_id_ind;

  long last;
  EXEC SQL END DECLARE SECTION;

  int db_plan_id = -1;
  int db_next_plan_id = -1;

  time_t now;
  time_t considerPlans;

  int ret;

  ln_get_timet_from_julian (JD, &now);

  // always load plan target!
  ret = Target::loadTarget (getTargetID ());
  if (ret)
    return ret;

  // get plan entries from last 12 hours..
  last = (time_t) (now - (hourLastSearch * 3600));

  considerPlans = (time_t) (now - (hourConsiderPlans * 3600.0));

  // check for next plan entry after 1 minute..
  nextTargetRefresh = now + 60;

  EXEC SQL DECLARE cur_plan CURSOR FOR
  SELECT
    plan_id,
    obs_id,
    EXTRACT (EPOCH FROM plan_start)
  FROM
    plan
  WHERE
      EXTRACT (EPOCH FROM plan_start) >= :last
    AND tar_id <> 7
  ORDER BY
    plan_start ASC;

  EXEC SQL OPEN cur_plan;
  while (1)
  {
    EXEC SQL FETCH next FROM cur_plan INTO
      :db_cur_plan_id,
      :db_obs_id :db_obs_id_ind,
      :db_cur_plan_start;
    if (sqlca.sqlcode)
      break;
    if (db_cur_plan_start > now)
    {
      if (db_plan_id == -1)
      {
        // that's the last plan ID
        db_plan_id = db_next_plan_id;
      }
      db_next_plan_id = db_cur_plan_id;
      break;
    }
    if (db_obs_id_ind < 0 && db_cur_plan_start > considerPlans)
    {
      // we found current plan - one that wasn't observed and is close enought to current time
      if (db_plan_id == -1)
      {
        db_plan_id = db_cur_plan_id;
      }
      else
      {
	db_next_plan_id = db_cur_plan_id;
	break;
      }
    }
    // keep for futher reference
    db_next_plan_id = db_cur_plan_id;
  }
  if (sqlca.sqlcode)
  {
    if (sqlca.sqlcode != ECPG_NOT_FOUND
      || db_next_plan_id == -1)
    {
      logMsgDb ("TargetPlan::load cannot find any plan");
      EXEC SQL CLOSE cur_plan;
      return 0;
    }
    // we don't find next, but we have current
    if (db_plan_id == -1)
    {
      db_plan_id = db_next_plan_id;
    }  
    db_next_plan_id = -1;
  }
  EXEC SQL CLOSE cur_plan;

  if (db_plan_id != -1)
  {
    delete selectedPlan;
    selectedPlan = new Rts2Plan (db_plan_id);
    ret = selectedPlan->load ();
    if (ret
      || selectedPlan->getTarget ()->getTargetType () == TYPE_PLAN)
    {
      delete selectedPlan;
      selectedPlan = NULL;
      setTargetName ("Cannot load target from schedule");
      return -1;
    }
    setTargetName (selectedPlan->getTarget()->getTargetName ());
  }
  else
  {
    setTargetName ("No scheduled target");
  }
  if (db_next_plan_id != -1)
  {
    delete nextPlan;
    nextPlan = new Rts2Plan (db_next_plan_id);
    ret = nextPlan->load ();
    if (ret)
    {
      delete nextPlan;
      nextPlan = NULL;
      return -1;
    }
  }
  return 0;
}

int
TargetPlan::getDBScript (const char *camera_name, char *script)
{
  if (selectedPlan)
    return selectedPlan->getTarget()->getScript (camera_name, script);
  return Target::getDBScript (camera_name, script);
}

int
TargetPlan::getPosition (struct ln_equ_posn *pos, double JD)
{
  if (selectedPlan)
    return selectedPlan->getTarget()->getPosition (pos, JD);
  // pretend to observe in zenith..
  pos->ra = ln_get_mean_sidereal_time (JD) * 15.0 + observer->lng;
  pos->dec = observer->lat;
  return 0;
}

int
TargetPlan::getRST (struct ln_rst_time *rst, double JD, double horizon)
{
  if (selectedPlan)
    return selectedPlan->getTarget()->getRST (rst, JD, horizon);
  // otherwise we have circumpolar target
  return 1;
}

int
TargetPlan::getObsTargetID ()
{
  if (selectedPlan)
    return selectedPlan->getTarget()->getObsTargetID ();
  return Target::getObsTargetID ();
}

int
TargetPlan::considerForObserving (double JD)
{
  int ret;
  ret = load (JD);
  if (ret)
    return ret;
  return Target::considerForObserving (JD);
}

float
TargetPlan::getBonus (double JD)
{
  // we have something to observe
  if (selectedPlan)
    return Target::getBonus (JD);
  return 0;
}

int
TargetPlan::isContinues ()
{
  time_t now;
  refreshNext ();
  time (&now);
  if ((nextPlan && nextPlan->getPlanStart () < now)
    || !(nextPlan))
    return 2;
  if (selectedPlan)
    return selectedPlan->getTarget()->isContinues ();
  return 0;
}

int
TargetPlan::beforeMove ()
{
  if (selectedPlan)
    selectedPlan->getTarget ()->beforeMove ();
  return Target::beforeMove ();
}

moveType
TargetPlan::startSlew (struct ln_equ_posn *pos)
{
  moveType ret;
  if (selectedPlan)
  {
    ret = selectedPlan->startSlew (pos);
    setObsId (selectedPlan->getTarget()->getObsId ());
    return ret;
  }
  return Target::startSlew (pos);
}

void
TargetPlan::printExtra (std::ostream & _os, double JD)
{
  Target::printExtra (_os, JD);
  if (selectedPlan)
  {
    _os 
      << "SELECTED PLAN" << std::endl
      << selectedPlan << std::endl
      << selectedPlan->getTarget () << std::endl
      << "*************************************************" << std::endl;
  }
  else
  {
    _os << "NO PLAN SELECTED" << std::endl;
  }
  if (nextPlan && nextPlan->getTarget()->getTargetType() != TYPE_PLAN)
  {
    _os << "NEXT PLAN" << std::endl
      << nextPlan << std::endl
      << nextPlan->getTarget () << std::endl
      << "*************************************************" << std::endl;
  }
  else
  {
    _os << "NO NEXT PLAN SELECTED" << std::endl;
  }
}
