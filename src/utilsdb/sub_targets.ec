#include <libnova/libnova.h>
#include "target.h"
#include "../utils/rts2config.h"

EXEC SQL include sqlca;

// ConstTarget
ConstTarget::ConstTarget (int in_tar_id, struct ln_lnlat_posn *in_obs):
Target (in_tar_id, in_obs)
{
  EXEC SQL BEGIN DECLARE SECTION;
  double d_ra;
  double d_dec;
  float d_tar_priority;
  float d_tar_bonus;
  int db_tar_id = in_tar_id;
  EXEC SQL END DECLARE SECTION;

  EXEC SQL
  SELECT 
    tar_ra,
    tar_dec,
    tar_priority,
    tar_bonus
  INTO
    :d_ra,
    :d_dec,
    :d_tar_priority,
    :d_tar_bonus
  FROM
    targets
  WHERE
    tar_id = :db_tar_id;
  if (sqlca.sqlcode)
  {
    throw &sqlca;
  }
  position.ra = d_ra;
  position.dec = d_dec;
  tar_priority = d_tar_priority;
  tar_bonus = d_tar_bonus;
}

int
ConstTarget::getPosition (struct ln_equ_posn *pos, double JD)
{
  *pos = position;
  return 0;
}

int
ConstTarget::getRST (struct ln_rst_time *rst, double JD)
{
  struct ln_equ_posn pos;
  int ret;
  
  ret = getPosition (&pos, JD);
  if (ret)
    return ret;
  return ln_get_object_next_rst (JD, observer, &pos, rst);
}

int
ConstTarget::selectedAsGood ()
{
  EXEC SQL BEGIN DECLARE SECTION;
  bool d_tar_enabled;
  int d_tar_id = target_id;
  float d_tar_priority;
  float d_tar_bonus;
  double d_tar_ra;
  double d_tar_dec;
  EXEC SQL END DECLARE SECTION;
  // check if we are still enabled..
  EXEC SQL
  SELECT
    tar_enabled,
    tar_priority,
    tar_bonus,
    tar_ra,
    tar_dec
  INTO
    :d_tar_enabled,
    :d_tar_priority,
    :d_tar_bonus,
    :d_tar_ra,
    :d_tar_dec
  FROM
    targets
  WHERE
    tar_id = :d_tar_id;
  if (sqlca.sqlcode)
  {
    logMsgDb ("Target::selectedAsGood");
    return -1;
  }
  tar_priority = d_tar_priority;
  tar_bonus = d_tar_bonus;
  position.ra = d_tar_ra;
  position.dec = d_tar_dec;
  if (d_tar_enabled && tar_priority + tar_bonus > 0)
    return 0;
  return -1;
}

// EllTarget - good for commets and so on
EllTarget::EllTarget (int in_tar_id, struct ln_lnlat_posn *in_obs):Target (in_tar_id, in_obs)
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
  double min_m;			// minimal magnitude
  int db_tar_id = in_tar_id;
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
    throw &sqlca;
  orbit.a = ell_a;
  orbit.e = ell_e;
  orbit.i = ell_i;
  orbit.w = ell_w;
  orbit.omega = ell_omega;
  orbit.n = ell_n;
  orbit.JD = ell_JD;
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
EllTarget::getRST (struct ln_rst_time *rst, double JD)
{
  return ln_get_ell_body_rst (JD, observer, &orbit, rst);
}

int
DarkTarget::defaultDark (const char *deviceName, char *buf)
{
  char dark_exposures [1000];
  char *tmp_c;
  char *tmp_c2;
  char *tmp_s;
  int ret;
  int was_blank;
  int exp_count;
  int exp_count2;

  Rts2Config *config;
  config = Rts2Config::instance ();
  ret = config->getString (deviceName, "darks", dark_exposures, 1000);
  if (ret)
    {
      strcpy (buf, "D 15");
      return 0;
    }
  // get the getCalledNum th observation
  // count how many exposures are in dark list..
  tmp_s = dark_exposures;
  while (*tmp_s && isblank (*tmp_s))
    tmp_s++;
  tmp_c = tmp_s;
  was_blank = 1;
  exp_count = 1;
  while (*tmp_c)
    {
      switch (was_blank)
        {
          case 1:
            if (!isblank (*tmp_c))
	      was_blank = 0;
	    break;
          case 0:
            if (isblank (*tmp_c))
  	      {
	        was_blank = 1;
	        exp_count++;
	      }
	    break;
        }
      tmp_c++;
    }
  if (exp_count == 0)
  {
    strcpy (buf, "D 17");
    return 0;
  }
  exp_count = getCalledNum () % exp_count;
  was_blank = 0;
  exp_count2 = 0;
  tmp_c = tmp_s;
  while (*tmp_c)
    {
      switch (was_blank)
        {
          case 1:
            if (!isblank (*tmp_c))
              {
                was_blank = 0;
              }
            break;
          case 0:
            if (isblank (*tmp_c))
	      {
	        was_blank = 1;
	        exp_count2++;
	      }
	    break;
        }
      if (exp_count == exp_count2)
        break;
      tmp_c++;
    }
  tmp_c2 = buf;
  *tmp_c2 = 'D';
  tmp_c2++;
  *tmp_c2 = ' ';
  *tmp_c2++;
  while (*tmp_c)
    {
      if (!isblank (*tmp_c))
        {
          *tmp_c2 = *tmp_c;
        }
      else
        break;
      tmp_c++;
      tmp_c2++;
    }
  *tmp_c2 = '\0'; 
  return 0;
}

int
DarkTarget::getScript (const char *deviceName, char *buf)
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
        dark_count DESC,
        img_exposure DESC;
  EXEC SQL OPEN dark_target;
  if (sqlca.sqlcode)
  {
    logMsgDb ("DarkTarget::getScript cannot open cursor dark_target, get default 10 sec darks");
    EXEC SQL CLOSE dark_target;
    return defaultDark (deviceName, buf);
  }
  EXEC SQL FETCH next FROM dark_target INTO
    :d_img_exposure,
    :d_dark_count;
  if (sqlca.sqlcode)
  {
    logMsgDb ("DarkTarget::getScript cannot get entry for darks, get default 10 sec darks");
    EXEC SQL CLOSE dark_target;
    return defaultDark (deviceName, buf);
  }
  // we found (finnaly) dark..let's make the script
  sprintf (buf, "D %f", d_img_exposure);
  EXEC SQL CLOSE dark_target;
  return 0;
}

DarkTarget::DarkTarget (int in_tar_id, struct ln_lnlat_posn *in_obs): Target (in_tar_id, in_obs)
{
  currPos.ra = 0;
  currPos.dec = 0;
}

int
DarkTarget::getPosition (struct ln_equ_posn *pos, double JD)
{
  *pos = currPos;
  return 0;
}

int
DarkTarget::startSlew (struct ln_equ_posn *position)
{
  currPos.ra = position->ra;
  currPos.dec = position->dec;
  Target::startSlew (position);
  return OBS_DONT_MOVE;
}

int
FlatTarget::getScript (const char *deviceName, char *buf)
{
  strcpy (buf, "E 1");
}

FlatTarget::FlatTarget (int in_tar_id, struct ln_lnlat_posn *in_obs): ConstTarget (in_tar_id, in_obs)
{
}

int
FlatTarget::getPosition (struct ln_equ_posn *pos, double JD)
{
  struct ln_equ_posn sun;
  struct ln_hrz_posn hrz;
  ln_get_solar_equ_coords (JD, &sun);
  ln_get_hrz_from_equ (&sun, observer, JD, &hrz);
  hrz.alt = 40;
  hrz.az = ln_range_degrees (hrz.az + 180);
  ln_get_equ_from_hrz (&hrz, observer, JD, pos);
  return 0;
}

int
FocusingTarget::getScript (const char *device_name, char *buf)
{
  buf[0] = COMMAND_FOCUSING;
  buf[1] = 0;
  return 0;
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
LunarTarget::getRST (struct ln_rst_time *rst, double JD)
{
  return ln_get_lunar_rst (JD, observer, rst);
}

int
LunarTarget::getScript (const char *deviceName, char *buf)
{
  strcpy (buf, "E 1");
  return 0;
}

TargetGRB::TargetGRB (int in_tar_id, struct ln_lnlat_posn *in_obs) : 
ConstTarget (in_tar_id, in_obs)
{
  EXEC SQL BEGIN DECLARE SECTION;
  long  db_grb_date;
  long  db_grb_last_update;
  int db_tar_id = in_tar_id;
  EXEC SQL END DECLARE SECTION;

  EXEC SQL SELECT
    EXTRACT (EPOCH FROM grb_date),
    EXTRACT (EPOCH FROM grb_last_update)
  INTO
    :db_grb_date,
    :db_grb_last_update
  FROM
    grb
  WHERE
    tar_id = :db_tar_id;
  if (sqlca.sqlcode)
    throw &sqlca;
  grbDate = db_grb_date; 
  // we don't expect grbDate to change much during observation,
  // so we will not update that in beforeMove (or somewhere else)
  lastUpdate = db_grb_last_update;
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

  post_sec = now - grbDate;

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
      return -1;
    }
  strncpy (script, sc_script.arr, sc_script.len);
  script[sc_script.len] = '\0';
  EXEC SQL CLOSE find_grb_script;
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

  if (now - grbDate < 1000)
  {
    strcpy (buf, "E 10 E 20 E 30 E 40");
  }
  else if (now - grbDate < 10000)
  {
    strcpy (buf, "E 100 E 200");
  }
  else
  {
    strcpy (buf, "E 300");
  }
  return 0;
}

float
TargetGRB::getBonus ()
{
  // time from GRB
  time_t now;
  time (&now);
  if (now - grbDate < 86400)
  {
    // < 24 hour post burst
    return ConstTarget::getBonus ()
      + 100.0 * cos ((double)((double)(now - grbDate) / (2.0*86400.0))) 
      + 10.0 * cos ((double)((double)(now - lastUpdate) / (2.0*86400.0)));
  }
  else if (now - grbDate < 5 * 86400)
  {
    // < 5 days post burst - add some time after last observation
    return ConstTarget::getBonus ()
      + 10.0 * sin (getLastObsTime () / 3600.0);
  }
  // default
  return ConstTarget::getBonus ();
}

TargetSwiftFOV::TargetSwiftFOV (int in_tar_id, struct ln_lnlat_posn *in_obs):Target (in_tar_id, in_obs)
{
  int ret;
  swiftOnBonus = 0;
  target_id = TARGET_SWIFT_FOV;
  ret = findPointing ();
  if (ret)
  {
    throw (&sqlca);
  }
}

int
TargetSwiftFOV::findPointing ()
{
  struct ln_hrz_posn testHrz;
  struct ln_equ_posn testEqu;
  double JD = ln_get_julian_from_sys ();

  EXEC SQL BEGIN DECLARE SECTION;
  int d_swift_id = -1;
  double d_swift_ra;
  double d_swift_dec;
  double d_swift_roll;
  int d_swift_roll_null;
  int d_swift_time;
  float d_swift_obstime;
  EXEC SQL END DECLARE SECTION;

  swiftId = -1;

  EXEC SQL DECLARE find_swift_poiniting CURSOR FOR
    SELECT
      swift_id,
      swift_ra,
      swift_dec,
      swift_roll,
      EXTRACT (EPOCH FROM swift_time),
      swift_obstime
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
      :d_swift_obstime;
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
      swiftId = d_swift_id;
      swiftTimeStart = d_swift_time;
      swiftTimeEnd = d_swift_time + (int) d_swift_obstime;
      break;
    }
  }
  EXEC SQL CLOSE find_swift_poiniting;
  if (swiftId < 0)
    return -1;
  return 0;
}

int
TargetSwiftFOV::getPosition (struct ln_equ_posn *pos, double JD)
{
  *pos = swiftFovCenter;
  return 0;
}

int
TargetSwiftFOV::startSlew (struct ln_equ_posn *position)
{
  EXEC SQL BEGIN DECLARE SECTION;
  int d_obs_id;
  int d_swift_id = swiftId;
  EXEC SQL END DECLARE SECTION;
  int ret;
  
  ret = Target::startSlew (position);
  if (ret)
    return ret;

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
    return -1;
  }
  EXEC SQL COMMIT;
  return 0;
}

int
TargetSwiftFOV::considerForObserving (ObjectCheck * checker, double JD)
{
  // find pointing
  int ret;
  struct ln_equ_posn curr_position;
  double gst = ln_get_mean_sidereal_time (JD);

  findPointing ();

  if (swiftId < 0)
  {
    // no pointing..expect that after 30 minutes, it will be better,
    // as we can get new pointing from GCN
    changePriority (-100, JD + 1/1440.0/2.0);
    return -1;
  }

  ret = getPosition (&curr_position, JD);
  if (ret)
  {
    changePriority (-100, JD + 1/1440.0/2.0);
    return -1;
  }

  ret = checker->is_good (gst, curr_position.ra, curr_position.dec);
  
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
    changePriority (-50, &nextObs);
    return -1;
  }

  return selectedAsGood ();
}

int
TargetSwiftFOV::beforeMove ()
{
  int oldSwiftId = swiftId;
  // are we still the best swiftId on planet?
  findPointing ();
  if (oldSwiftId != swiftId)
    endObservation (-1);  // startSlew will be called after move suceeded and will write new observation..
}

float
TargetSwiftFOV::getBonus ()
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
  time (&now);
  if (now > swiftTimeStart - 120 && now < swiftTimeEnd + 120)
    return swiftOnBonus;
  if (now > swiftTimeStart - 300 && now < swiftTimeEnd + 300)
    return swiftOnBonus / 2.0;
  return 1;
}

TargetGps::TargetGps (int in_tar_id, struct ln_lnlat_posn *in_obs): ConstTarget (in_tar_id, in_obs)
{
}

float
TargetGps::getBonus ()
{
  // get our altitude..
  struct ln_hrz_posn hrz;
  struct ln_equ_posn curr;
  int numobs;
  int numobs2;
  time_t now;
  time_t start_t;
  time_t start_t2;
  double JD;
  double gal_ctr;
  JD = ln_get_julian_from_sys ();
  getAltAz (&hrz, JD);
  getPosition (&curr, JD);
  // get number of observations in last 24 hours..
  time (&now);
  start_t = now - 86400;
  start_t2 = now - 86400 * 2;
  numobs = getNumObs (&start_t, &now);
  numobs2 = getNumObs (&start_t2, &start_t);
  gal_ctr = getGalCenterDist (JD);
  if ((90 - observer->lat + curr.dec) == 0)
    return ConstTarget::getBonus () - 200;
  return ConstTarget::getBonus () + 20 * (hrz.alt / (90 - observer->lat + curr.dec)) + gal_ctr / 9 - numobs * 10 - numobs2 * 5;
}
