#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "target.h"
#include "rts2obs.h"
#include "rts2obsset.h"

#include "../utils/rts2app.h"
#include "../utils/rts2config.h"
#include "../utils/libnova_cpp.h"
#include "../utils/timestamp.h"

#include <syslog.h>

#include <sstream>

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
  syslog (LOG_ERR, "SQL error: %li %s (at %s)", sqlca.sqlcode, sqlca.sqlerrm.sqlerrmc, message);
}

void
Target::sendTargetMail (int eventMask, const char *subject_text)
{
  std::string mails;
  
  // send mails
  mails = getUsersEmail (eventMask);
  if (mails.length ())
  {
    std::ostringstream os;
    std::ostringstream subject;
    subject << "TARGET #" << getObsTargetID ()
     << " (" << getTargetID () << ") "
     << subject_text << " #" << getObsId ();
    // lazy observation init
    if (observation == NULL)
    {
      observation = new Rts2Obs (getObsId ());
      observation->load ();
      observation->setPrintImages (DISPLAY_ALL | DISPLAY_SUMMARY);
      observation->setPrintCounts (DISPLAY_ALL | DISPLAY_SUMMARY);
    }
    os << *observation;
    sendMailTo (subject.str().c_str(), os.str().c_str(), mails.c_str());
  }
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
  observation = NULL;

  img_id = 0;
  obs_state = 0;
  target_id = in_tar_id;
  obs_target_id = -1;
  target_type = TYPE_UNKNOW;
  target_name = NULL;
  targetUsers = NULL;

  startCalledNum = 0;

  airmassScale = 750.0;

  observationStart = -1;

  acquired = 0;
}

Target::Target ()
{
  Rts2Config *config;
  config = Rts2Config::instance ();

  observer = config->getObserver ();
  selected = 0;

  epochId = 1;
  config->getInteger ("observatory", "epoch_id", epochId);

  obs_id = -1;
  observation = NULL;

  img_id = 0;
  obs_state = 0;
  target_id = -1;
  obs_target_id = -1;
  target_type = TYPE_UNKNOW;
  target_name = NULL;
  targetUsers = NULL;

  startCalledNum = 0;

  airmassScale = 750.0;

  observationStart = -1;

  acquired = 0;
}

Target::~Target (void)
{
  endObservation (-1);
  delete[] target_name;
  delete targetUsers;
  delete observation;
}

int
Target::load ()
{
  // load target users for events..
  targetUsers = new Rts2TarUser (getTargetID (), getTargetType ());
  return 0;
}

int
Target::save ()
{
  return 0;
}

int
Target::save (int tar_id)
{
  return 0;
}

int
Target::compareWithTarget (Target *in_target, double in_sep_limit)
{
  struct ln_equ_posn other_position;
  in_target->getPosition (&other_position);

  return (getDistance (&other_position) < in_sep_limit);
}

int
Target::startSlew (struct ln_equ_posn *position)
{
  EXEC SQL BEGIN DECLARE SECTION;
  int d_tar_id = getObsTargetID ();
  int d_obs_id;
  double d_obs_ra;
  double d_obs_dec;
  float d_obs_alt;
  float d_obs_az;
  EXEC SQL END DECLARE SECTION;

  struct ln_hrz_posn hrz;

  getPosition (position);
  selected++;

  obs_state |= OBS_BIT_MOVED;

  if (obs_id > 0) // we already observe that target
    return OBS_ALREADY_STARTED;

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
  return OBS_MOVE;
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
    obs_state |= OBS_BIT_STARTED;

    sendTargetMail (SEND_START_OBS, "START OBSERVATION");
  }
  return 0;
}

void
Target::acqusitionStart ()
{
  obs_state |= OBS_BIT_ACQUSITION;
}

void
Target::acqusitionEnd ()
{
  obs_state &= ~ OBS_BIT_ACQUSITION;
  acquired = 1;
}

void
Target::interupted ()
{
  obs_state |= OBS_BIT_INTERUPED;
}

void
Target::acqusitionFailed ()
{
  obs_state |= OBS_BIT_ACQUSITION_FAI;
}

int
Target::endObservation (int in_next_id)
{
  EXEC SQL BEGIN DECLARE SECTION;
  int d_obs_id = obs_id;
  int d_obs_state = obs_state;
  EXEC SQL END DECLARE SECTION;

  if (isContinues () && in_next_id == getTargetID ())
    return 1;
  if (obs_id > 0)
  {
    EXEC SQL
    UPDATE
      observations
    SET
      obs_end = now (),
      obs_state = :d_obs_state
    WHERE
      obs_id = :d_obs_id;
    if (sqlca.sqlcode != 0)
    {
      logMsgDb ("cannot end observation");
      EXEC SQL ROLLBACK;
      obs_id = -1;
      return -1;
    }
    EXEC SQL COMMIT;

    sendTargetMail (SEND_END_OBS, "END OF OBSERVATION");

    // check if that was the last observation..
    Rts2Obs observation = Rts2Obs (getObsId ());
    observation.checkUnprocessedImages ();

    obs_id = -1;
  }
  observationStart = -1;
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
  int ret;
  Rts2Config *config;
  config = Rts2Config::instance ();

  ret = getDBScript (device_name, buf);
  if (!ret)
    return 0;

  ret = config->getString (device_name, "script", buf, MAX_COMMAND_LENGTH);
  if (!ret)
    return 0;

  strncpy (buf, "", MAX_COMMAND_LENGTH);
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
  getAltAz (&hrz, JD);
  return ln_get_airmass (hrz.alt, airmassScale);
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
  ha = ln_range_degrees (ha);
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
Target::selectedAsGood ()
{
  EXEC SQL BEGIN DECLARE SECTION;
  bool d_tar_enabled;
  int d_tar_id = getObsTargetID ();
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

/**
 * return 0 if we cannot observe that target, 1 if it's above horizont.
 */
int
Target::isGood (double lst, double ra, double dec)
{
  return Rts2Config::instance ()->getObjectChecker ()->is_good (lst, ra, dec);
}

/****
 * 
 *   Return -1 if target is not suitable for observing,
 *   othrwise return 0.
 */
int
Target::considerForObserving (double JD)
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
  ret = isGood (lst, curr_position.ra, curr_position.dec);
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
    // handle circumpolar objects..
    if (ret == 1)
    {
      syslog (LOG_DEBUG, "Target::considerForObserving is circumpolar, but is not good, scheduling after 10 minutes");
      changePriority (-100, JD + 10.0 / (24.0 * 60.0));
      return -1;
    }
    // object is above horizont, but checker reject it..let's see what
    // will hapens in 12 minutes 
    if (rst.transit < rst.set && rst.set < rst.rise)
    {
      // object rose, but is not above horizont, let's hope in 12 minutes it will get above horizont
      syslog (LOG_DEBUG, "Target::considerForObserving %i will rise tommorow: %f JD %f", getTargetID (), rst.rise, JD);
      changePriority (-100, JD + 12*(1.0/1440.0));
      return -1;
    }
    // object is setting, let's target it for next rise..
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
  int db_tar_id = getObsTargetID ();
  int db_priority_change = pri_change;
  int db_next_t = (int) *time_ch;
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
    printf ("db_next_t: %i\n", db_next_t);
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
    min (EXTRACT (EPOCH FROM (now () - obs_slew)))
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

std::string
Target::getUsersEmail (int in_event_mask)
{
  if (targetUsers)
    return targetUsers->getUsers (in_event_mask);
  return std::string ("");
}

double
Target::getFirstObs ()
{
  EXEC SQL BEGIN DECLARE SECTION;
  int db_tar_id = getTargetID ();
  double ret;
  EXEC SQL END DECLARE SECTION;
  EXEC SQL
  SELECT
    MIN (EXTRACT (EPOCH FROM obs_start))
  INTO
    :ret
  FROM
    observations
  WHERE
    tar_id = :db_tar_id;
  if (sqlca.sqlcode)
  {
    EXEC SQL ROLLBACK;
    return nan("f");
  }
  EXEC SQL ROLLBACK;
  return ret;
}

double
Target::getLastObs ()
{
  EXEC SQL BEGIN DECLARE SECTION;
  int db_tar_id = getTargetID ();
  double ret;
  EXEC SQL END DECLARE SECTION;
  EXEC SQL
  SELECT
    MAX (EXTRACT (EPOCH FROM obs_start))
  INTO
    :ret
  FROM
    observations
  WHERE
    tar_id = :db_tar_id;
  if (sqlca.sqlcode)
  {
    EXEC SQL ROLLBACK;
    return nan("f");
  }
  EXEC SQL ROLLBACK;
  return ret;
}

void
Target::printExtra (std::ostream &_os)
{
}

Target *createTarget (int in_tar_id, struct ln_lnlat_posn *in_obs)
{
  EXEC SQL BEGIN DECLARE SECTION;
  int db_tar_id = in_tar_id;
  char db_type_id;
  EXEC SQL END DECLARE SECTION;

  Target *retTarget;
  int ret;

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
  {
    syslog (LOG_ERR, "createTarget cannot get entry from targets table for target with ID %i", db_tar_id);
    return NULL;
  }

  // get more informations about target..
  switch (db_type_id)
  {
    // calibration targets..
    case TYPE_DARK:
      retTarget = new DarkTarget (in_tar_id, in_obs);
      break;
    case TYPE_FLAT:
      retTarget = new FlatTarget (in_tar_id, in_obs);
      break;
    case TYPE_FOCUSING:
      retTarget = new FocusingTarget (in_tar_id, in_obs);
      break;
    case TYPE_CALIBRATION:
      retTarget = new CalibrationTarget (in_tar_id, in_obs);
      break;
    case TYPE_MODEL:
      retTarget = new ModelTarget (in_tar_id, in_obs);
      break;
    case TYPE_OPORTUNITY:
      retTarget = new OportunityTarget (in_tar_id, in_obs);
      break;
    case TYPE_ELLIPTICAL:
      retTarget = new EllTarget (in_tar_id, in_obs);
      break;
    case TYPE_GRB:
      retTarget = new TargetGRB (in_tar_id, in_obs);
      break;
    case TYPE_SWIFT_FOV:
      retTarget = new TargetSwiftFOV (in_tar_id, in_obs);
      break;
    case TYPE_GPS:
      retTarget = new TargetGps (in_tar_id, in_obs);
      break;
    case TYPE_SKY_SURVEY:
      retTarget = new TargetSkySurvey (in_tar_id, in_obs);
      break;
    case TYPE_TERESTIAL:
      retTarget = new TargetTerestial (in_tar_id, in_obs);
      break;
    default:
      retTarget = new ConstTarget (in_tar_id, in_obs);
      break;
  }

  retTarget->setTargetType (db_type_id);
  ret = retTarget->load ();
  if (ret)
  {
    syslog (LOG_ERR, "Cannot create target: %i sqlcode: %li %s",
    db_tar_id, sqlca.sqlcode, sqlca.sqlerrm.sqlerrmc);
    EXEC SQL ROLLBACK;
    delete retTarget;
    return NULL;
  }
  EXEC SQL COMMIT;
  return retTarget;
}

void
sendEndMails (const time_t *t_from, const time_t *t_to, int printImages, int printCounts, struct ln_lnlat_posn *in_obs)
{
  EXEC SQL BEGIN DECLARE SECTION;
  long db_from = (long) *t_from;
  long db_end = (long) *t_to;
  int db_tar_id;
  EXEC SQL END DECLARE SECTION;

  EXEC SQL DECLARE tar_obs_cur CURSOR FOR
  SELECT
    targets.tar_id
  FROM
    targets,
    observations
  WHERE
      targets.tar_id = observations.tar_id
    AND observations.obs_slew >= abstime (:db_from)
    AND observations.obs_end <= abstime (:db_end);
    
  EXEC SQL OPEN tar_obs_cur;

  while (1)
  {
    Target *tar;
    EXEC SQL FETCH next FROM tar_obs_cur INTO
      :db_tar_id;
    if (sqlca.sqlcode)
      break;
    tar = createTarget (db_tar_id, in_obs);
    if (tar)
    {
      std::string mails = tar->getUsersEmail (SEND_END_NIGHT);
      std::string subject_text = std::string ("END OF NIGHT, TARGET #");
      Rts2ObsSet obsset = Rts2ObsSet (db_tar_id, t_from, t_to);
      subject_text += db_tar_id;
      std::ostringstream os;
      obsset.printImages (printImages);
      obsset.printCounts (printCounts);
      os << tar << obsset;
      sendMailTo (subject_text.c_str(), os.str().c_str(), mails.c_str());
      delete tar;
    }
  }
  EXEC SQL CLOSE tar_obs_cur;
  EXEC SQL COMMIT;
}

std::ostream &
operator << (std::ostream &_os, Target *target)
{
  int ret;
  struct ln_equ_posn pos;
  struct ln_hrz_posn hrz;
  struct ln_gal_posn gal;
  struct ln_rst_time rst;
  double JD;
  double gst;
  double lst;
  time_t now, last;

  const char *name = target->getTargetName ();

  time (&now);
  JD = ln_get_julian_from_timet (&now);

  target->getPosition (&pos, JD);

  _os << target->getTargetID () 
    << " (" << target->getObsTargetID () << ") " 
    << (name ? name : "null name")
    << " (" << target->getTargetType () << ")"
    << " RA " << LibnovaRa (pos.ra)
    << " DEC " << LibnovaDeg90 (pos.dec)
    << " (J2000) "
    << std::endl;
  target->getAltAz (&hrz, JD);
  _os << "ALT " << LibnovaDeg90 (hrz.alt)
    << " ZD " << LibnovaDeg90 (target->getZenitDistance ()) 
    << " AZ " << LibnovaDeg (hrz.az)
    << " HA " << LibnovaRa (target->getHourAngle ())
    << " AM " << target->getAirmass ()
    << std::endl;
  ret = target->getRST (&rst, JD);
  switch (ret)
  {
    case -1:
      _os << " - circumpolar - " << std::endl;
      break;
    case 1:
      _os << " - don't rise - " << std::endl;
      break;
    default:
      if (rst.set < rst.rise && rst.rise < rst.transit)
      {
	_os << "S " << TimeJDDiff (rst.set, now) << std::endl
	  << "R " << TimeJDDiff (rst.rise, now) << std::endl
	  << "T " << TimeJDDiff (rst.transit, now) << std::endl;
      }
      else if (rst.transit < rst.set && rst.set < rst.rise)
      {
	_os << "T " << TimeJDDiff (rst.transit, now) << std::endl
	  << "S " << TimeJDDiff (rst.set, now) << std::endl
	  << "R " << TimeJDDiff (rst.rise, now) << std::endl
	  << std::endl;
      }
      else
      {
	_os << "R " << TimeJDDiff (rst.rise, now) << std::endl
	  << "T " << TimeJDDiff (rst.transit, now) << std::endl
	  << "S " << TimeJDDiff (rst.set, now) << std::endl
	  << std::endl;
      }
  }
  target->getGalLng (&gal, JD);
  _os << "GL " << LibnovaDeg (gal.l) 
    << " GB " << LibnovaDeg90 (gal.b)
    << " GCD " << LibnovaDeg (target->getGalCenterDist (JD))
    << std::endl;
  _os << "SD " << LibnovaDeg (target->getSolarDistance (JD))
    << " LD " << LibnovaDeg (target->getLunarDistance (JD))
    << std::endl;
  last = now - 86400;
  _os << "OBS_24_H " << target->getNumObs (&last, &now)
    << std::endl;
  _os << "BONUS " << target->getBonus (JD) << std::endl;

  // is above horizont?
  gst = ln_get_mean_sidereal_time (JD);
  lst = gst + Rts2Config::instance ()->getObserver()->lng / 15.0;
  _os << (target->isGood (lst, pos.ra, pos.dec)
   ? "Target is above local horizont." 
   : "Target is below local horizont, it's not possible to observe it.")
   << std::endl;
  target->printExtra (_os);
  return _os;
}
