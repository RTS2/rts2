#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "target.h"

#include "../utils/rts2config.h"

#include <syslog.h>

EXEC SQL include sqlca;

void
Target::logMsg (const char *message)
{
  printf ("%s\n", message);
}

void
Target::logMsg (const char *message, int num)
{
  printf ("%s %i\n", message, num);
}

void
Target::logMsg (const char *message, long num)
{
  printf ("%s %li\n", message, num);
}

void
Target::logMsg (const char *message, double num)
{
  printf ("%s %f\n", message, num);
}

void
Target::logMsg (const char *message, const char *val)
{
  printf ("%s %s\n", message, val);
}

void
Target::logMsgDb (const char *message)
{
  syslog (LOG_ERR, "SQL error: %i %s (at %s)", sqlca.sqlcode, sqlca.sqlerrm.sqlerrmc, message);
}

Target::Target (int in_tar_id, struct ln_lnlat_posn *in_obs)
{
  Rts2Config *config;
  config = Rts2Config::instance ();

  observer = in_obs;
  selected = 0;

  epochId = 1;
  config->getInteger ("observatory", "epoch_id", epochId);

  obs_id = -1;
  img_id = 0;
  target_id = in_tar_id;

  startCalledNum = 0;

  airmassScale = 750.0;

  observationStart = -1;
}

Target::~Target (void)
{
  endObservation (-1);
}

int
Target::startSlew (struct ln_equ_posn *position)
{
  EXEC SQL BEGIN DECLARE SECTION;
  int d_tar_id = target_id;
  int d_obs_id;
  double d_obs_ra;
  double d_obs_dec;
  float d_obs_alt;
  float d_obs_az;
  EXEC SQL END DECLARE SECTION;

  struct ln_hrz_posn hrz;

  getPosition (position);
  selected++;

  if (obs_id > 0) // we already observe that target
    return 1;

  d_obs_ra = position->ra;
  d_obs_dec = position->dec;
  ln_get_hrz_from_equ (position, observer, ln_get_julian_from_sys (), &hrz);
  d_obs_alt = hrz.alt;
  d_obs_az = hrz.az;

  EXEC SQL
  SELECT
    nextval ('obs_id')
  INTO
    :d_obs_id;
  EXEC SQL
  INSERT INTO
    observations
  (
    tar_id,
    obs_id,
    obs_slew,
    obs_ra,
    obs_dec,
    obs_alt,
    obs_az
  )
  VALUES
  (
    :d_tar_id,
    :d_obs_id,
    now (),
    :d_obs_ra,
    :d_obs_dec,
    :d_obs_alt,
    :d_obs_az
  );
  if (sqlca.sqlcode != 0)
  {
    logMsgDb ("cannot insert observation slew start to db");
    EXEC SQL ROLLBACK;
    return -1;
  }
  EXEC SQL COMMIT;
  obs_id = d_obs_id;
  return 0;
}

int
Target::startObservation ()
{
  EXEC SQL BEGIN DECLARE SECTION;
  int d_obs_id = obs_id;
  EXEC SQL END DECLARE SECTION;
  if (observationStarted ())
    return 0;
  time (&observationStart);
  if (obs_id > 0)
  {
    EXEC SQL
    UPDATE
      observations
    SET
      obs_start = now ()
    WHERE
      obs_id = :d_obs_id;
    if (sqlca.sqlcode != 0)
    {
      logMsgDb ("cannot start observation");
      EXEC SQL ROLLBACK;
      return -1;
    }
    EXEC SQL COMMIT;
  }
  return 0;
}

int
Target::endObservation (int in_next_id)
{
  EXEC SQL BEGIN DECLARE SECTION;
  int d_obs_id = obs_id;
  EXEC SQL END DECLARE SECTION;
  if (in_next_id == getTargetID ())
    return 1;
  if (obs_id > 0)
  {
    EXEC SQL
    UPDATE
      observations
    SET
      obs_end = now ()
    WHERE
      obs_id = :d_obs_id;
    obs_id = -1;
    if (sqlca.sqlcode != 0)
    {
      logMsgDb ("cannot end observation");
      EXEC SQL ROLLBACK;
      return -1;
    }
    EXEC SQL COMMIT;
  }
  return 0;
}

int
Target::observationStarted ()
{
  return (observationStart > 0) ? 1 : 0;
}

int
Target::secToObjectSet (double JD)
{
  struct ln_rst_time rst;
  int ret;
  ret = getRST (&rst, JD);
  if (ret)
    return -1;  // don't rise, circumpolar etc..
  ret = int ((rst.set - JD) * 86400.0);
  if (ret < 0)
    // hope we get current set, we are interested in next set..
    return ret + ((int (ret/86400) + 1) * -86400);
  return ret;
}

int
Target::secToObjectRise (double JD)
{
  struct ln_rst_time rst;
  int ret;
  ret = getRST (&rst, JD);
  if (ret)
    return -1;  // don't rise, circumpolar etc..
  ret = int ((rst.rise - JD) * 86400.0);
  if (ret < 0)
    // hope we get current set, we are interested in next set..
    return ret + ((int (ret/86400) + 1) * -86400);
  return ret;
}

int
Target::secToObjectMeridianPass (double JD)
{
  struct ln_rst_time rst;
  int ret;
  ret = getRST (&rst, JD);
  if (ret)
    return -1;  // don't rise, circumpolar etc..
  ret = int ((rst.transit - JD) * 86400.0);
  if (ret < 0)
    // hope we get current set, we are interested in next set..
    return ret + ((int (ret/86400) + 1) * -86400);
  return ret;
}

int
Target::beforeMove ()
{
  startCalledNum++;
  return 0;
}

/**
 * Move to designated target, get astrometry, proof that target was acquired with given margin.
 *
 * @return 0 if I can observe futher, -1 if observation was canceled
 */
int
Target::acquire ()
{

}

int
Target::observe ()
{

}

/**
 * Return script for camera exposure.
 *
 * @param target        target id
 * @param camera_name   name of the camera
 * @param script        script
 * 
 * return -1 on error, 0 on success
 */
int
Target::getDBScript (const char *camera_name, char *script)
{
  EXEC SQL BEGIN DECLARE SECTION;
  int tar_id = getTargetID ();
  VARCHAR d_camera_name[8];
  VARCHAR sc_script[2000];
  int sc_indicator;
  EXEC SQL END DECLARE SECTION;
  
  d_camera_name.len = strlen (camera_name);
  strncpy (d_camera_name.arr, camera_name, d_camera_name.len);
  
  EXEC SQL 
  SELECT
    script
  INTO
    :sc_script :sc_indicator
  FROM
    scripts
  WHERE
    tar_id = :tar_id
    AND camera_name = :d_camera_name;
  if (sqlca.sqlcode)
    goto err;
  if (sc_indicator < 0)
    goto err;
  strncpy (script, sc_script.arr, sc_script.len);
  script[sc_script.len] = '\0';
  return 0;
err:
  printf ("err db_get_script: %li %s\n", sqlca.sqlcode,
	  sqlca.sqlerrm.sqlerrmc);
  script[0] = '\0';
  return -1;
}


/**
 * Return script for camera exposure.
 *
 * @param device_name	camera device for script
 * @param buf		buffer for script
 *
 * @return 0 on success, < 0 on error
 */
int
Target::getScript (const char *device_name, char *buf)
{
  char obs_type_str[2];
  int ret;
  Rts2Config *config;
  config = Rts2Config::instance ();

  obs_type_str[0] = obs_type;
  obs_type_str[1] = 0;

  ret = getDBScript (device_name, buf);
  if (!ret)
    return 0;

  ret = config->getString (device_name, "script", buf, MAX_COMMAND_LENGTH);
  if (!ret)
    return 0;

  strncpy (buf, "E 11", MAX_COMMAND_LENGTH);
  return 0;
}

int
Target::getAltAz (struct ln_hrz_posn *hrz, double JD)
{
  int ret;
  struct ln_equ_posn object;

  ret = getPosition (&object, JD);
  if (ret)
    return ret;
  ln_get_hrz_from_equ (&object, observer, JD, hrz);
  return 0;
}

int
Target::getGalLng (struct ln_gal_posn *gal, double JD)
{
  struct ln_equ_posn curr;
  getPosition (&curr, JD);
  ln_get_gal_from_equ (&curr, gal);
  return 0;
}

double
Target::getGalCenterDist (double JD)
{
  static struct ln_equ_posn cntr = { 265.610844, -28.916790 };
  struct ln_equ_posn curr;
  getPosition (&curr, JD);
  return ln_get_angular_separation (&curr, &cntr); 
}

double
Target::getAirmass (double JD)
{
  struct ln_hrz_posn hrz;
  double x;
  getAltAz (&hrz, JD);
  x = airmassScale * sin (ln_deg_to_rad (hrz.alt));
  return sqrt (x * x + 2 * airmassScale + 1) - x;
}

double
Target::getZenitDistance (double JD)
{
  struct ln_hrz_posn hrz;
  getAltAz (&hrz, JD);
  return 90.0 - hrz.alt;
}

double
Target::getHourAngle (double JD)
{
  double lst;
  double ha;
  struct ln_equ_posn pos;
  int ret;
  lst = ln_get_mean_sidereal_time (JD) * 15.0 + observer->lng;
  ret = getPosition (&pos);
  if (ret)
    return nan ("f");
  ha = lst - pos.ra;
  ha = ln_range_degrees (ha) / 15.0;
  if (ha > 12.0)
    return 24 - ha;
  return ha;
}

double
Target::getDistance (struct ln_equ_posn *in_pos, double JD)
{
  struct ln_equ_posn object;
  getPosition (&object, JD);
  return ln_get_angular_separation (&object, in_pos);
}

double
Target::getSolarDistance (double JD)
{
  struct ln_equ_posn sun;
  ln_get_solar_equ_coords (JD, &sun);
  return getDistance (&sun, JD);
}

double
Target::getLunarDistance (double JD)
{
  struct ln_equ_posn moon;
  ln_get_lunar_equ_coords (JD, &moon);
  return getDistance (&moon, JD);
}

int
Target::postprocess ()
{

}

int
Target::selectedAsGood ()
{
  EXEC SQL BEGIN DECLARE SECTION;
  bool d_tar_enabled;
  int d_tar_id = target_id;
  EXEC SQL END DECLARE SECTION;
  // check if we are still enabled..
  EXEC SQL
  SELECT
    tar_enabled
  INTO
    :d_tar_enabled
  FROM
    targets
  WHERE
    tar_id = :d_tar_id;
  if (sqlca.sqlcode)
  {
    logMsgDb ("Target::selectedAsGood");
    return -1;
  }
  if (d_tar_enabled)
    return 0;
  return -1;
}

/****
 * 
 *   Return -1 if target is not suitable for observing,
 *   othrwise return 0.
 */
int
Target::considerForObserving (ObjectCheck *checker, double JD)
{
  // horizont constrain..
  struct ln_equ_posn curr_position;
  double lst = ln_get_mean_sidereal_time (JD) + observer->lng / 15.0;
  int ret;
  if (getPosition (&curr_position, JD))
  {
    changePriority (-100, JD + 1);
    return -1;
  }
  ret = checker->is_good (lst, curr_position.ra, curr_position.dec);
  if (!ret)
  {
    struct ln_rst_time rst;
    ret = getRST (&rst, JD);
    if (ret == -1)
    {
      // object doesn't rise, let's hope tomorrow it will rise
      syslog (LOG_DEBUG, "Target::considerForObserving tar %i don't rise", getTargetID ());
      changePriority (-100, JD + 1);
      return -1;
    }
    // object is above horizont, but checker reject it..let's see what
    // will hapens in 10 minutes 
    if (rst.rise < JD)
    {
      // object doesn't rise, let's hope tomorrow it will rise
      syslog (LOG_DEBUG, "Target::considerForObserving %i will rise tommorow: %f", getTargetID (), rst.rise);
      changePriority (-100, JD + 12*(1.0/1440.0));
      return -1;
    }
    syslog (LOG_DEBUG, "Target::considerForObserving %i will rise at: %f", getTargetID (), rst.rise);
    changePriority (-100, rst.rise);
    return -1;
  }
  // target was selected for observation
  return selectedAsGood (); 
}

int
Target::dropBonus ()
{
  EXEC SQL BEGIN DECLARE SECTION;
  int db_tar_id;
  EXEC SQL END DECLARE SECTION;

  EXEC SQL UPDATE 
    targets 
  SET
    tar_bonus = NULL,
    tar_bonus_time = NULL
  WHERE
    tar_id = :db_tar_id;
  if (sqlca.sqlcode)
  {
    logMsgDb ("Target::dropBonus");
    EXEC SQL ROLLBACK;
    return -1;
  }
  EXEC SQL COMMIT;
  return 0;
}

int
Target::changePriority (int pri_change, time_t *time_ch)
{
  EXEC SQL BEGIN DECLARE SECTION;
  int db_tar_id = target_id;
  int db_priority_change = pri_change;
  long int db_next_t = (long int) *time_ch;
  EXEC SQL END DECLARE SECTION;
  EXEC SQL UPDATE 
    targets
  SET
    tar_bonus = tar_bonus + :db_priority_change,
    tar_bonus_time = abstime(:db_next_t)
  WHERE
    tar_id = :db_tar_id;
  if (sqlca.sqlcode)
  {
    logMsgDb ("Target::changePriority");
    EXEC SQL ROLLBACK;
    return -1;
  }
  EXEC SQL COMMIT;
  return 0;
}

int
Target::changePriority (int pri_change, double validJD)
{
  time_t next;
  ln_get_timet_from_julian (validJD, &next);
  return changePriority (pri_change, &next);
}

int
Target::getNumObs (time_t *start_time, time_t *end_time)
{
  EXEC SQL BEGIN DECLARE SECTION;
  int d_start_time = (int) *start_time;
  int d_end_time = (int) *end_time;
  int d_count;
  int d_tar_id = getTargetID ();
  EXEC SQL END DECLARE SECTION;

  EXEC SQL
  SELECT
    count (*)
  INTO
    :d_count
  FROM
    observations
  WHERE
      tar_id = :d_tar_id
    AND obs_slew >= abstime (:d_start_time)
    AND (obs_end is null 
      OR obs_end <= abstime (:d_end_time)
    );
  // runnign observations counts as well - hence obs_end is null

  return d_count;
}

double
Target::getLastObsTime ()
{
  EXEC SQL BEGIN DECLARE SECTION;
  int d_tar_id = getTargetID ();
  double d_time_diff;
  EXEC SQL END DECLARE SECTION;

  EXEC SQL
  SELECT
    min (EXTRACT (EPOCH FROM (now () - obs_start)))
  INTO
    :d_time_diff
  FROM
    observations
  WHERE
    tar_id = :d_tar_id
  GROUP BY
    tar_id;

  if (sqlca.sqlcode)
    {
      if (sqlca.sqlcode == ECPG_NOT_FOUND)
        {
           // 1 year was the last observation..
           return 356 * 86400.0;
        }
      else
        logMsgDb ("Target::getLastObsTime");
    }
  return d_time_diff;
}

Target *createTarget (int in_tar_id, struct ln_lnlat_posn *in_obs)
{
  EXEC SQL BEGIN DECLARE SECTION;
  int db_tar_id = in_tar_id;
  char db_type_id;
  EXEC SQL END DECLARE SECTION;

  try
  {
    EXEC SQL
    SELECT
      type_id
    INTO
      :db_type_id
    FROM
      targets
    WHERE
      tar_id = :db_tar_id;
  
    if (sqlca.sqlcode)
      throw &sqlca;

    // get more informations about target..
    switch (db_type_id)
    {
      // calibration targets..
      case TYPE_DARK:
        return new DarkTarget (in_tar_id, in_obs);
      case TYPE_FLAT:
        return new FlatTarget (in_tar_id, in_obs);
      case TYPE_FOCUSING:
        return new FocusingTarget (in_tar_id, in_obs);
      case TYPE_ELLIPTICAL:
	return new EllTarget (in_tar_id, in_obs);
      case TYPE_GRB:
        return new TargetGRB (in_tar_id, in_obs);
      case TYPE_SWIFT_FOV:
        return new TargetSwiftFOV (in_tar_id, in_obs);
      case TYPE_GPS:
        return new TargetGps (in_tar_id, in_obs);
      default:
        return new ConstTarget (in_tar_id, in_obs);
    }
  }
  catch (struct sqlca_t *sqlerr)
  {
    syslog (LOG_ERR, "Cannot create target: %i sqlcode: %i %s",
    db_tar_id, sqlerr->sqlcode, sqlerr->sqlerrm.sqlerrmc);
  }
  EXEC SQL ROLLBACK;
  return NULL;
}
