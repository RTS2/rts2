#include <libnova/libnova.h>
#include "target.h"
#include "../utils/rts2config.h"

EXEC SQL include sqlca;

// ConstTarget
ConstTarget::ConstTarget (int in_tar_id, struct ln_lnlat_posn *in_obs):
Target (in_tar_id, in_obs)
{
}

int
ConstTarget::load ()
{
  EXEC SQL BEGIN DECLARE SECTION;
  double d_ra;
  double d_dec;
  float d_tar_priority;
  float d_tar_bonus;
  int db_tar_id = getTargetID ();
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
    logMsgDb ("ConstTarget::load");
    return -1;
  }
  position.ra = d_ra;
  position.dec = d_dec;
  tar_priority = d_tar_priority;
  tar_bonus = d_tar_bonus;
  return Target::load ();
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
  double min_m;			// minimal magnitude
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
EllTarget::getRST (struct ln_rst_time *rst, double JD)
{
  return ln_get_ell_body_rst (JD, observer, &orbit, rst);
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
      dark_exposures.push_back (15);
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
      dark_exposures.push_back (exp);
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

  int ret;

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
        target->logMsgDb ("PossibleDarks::getDb cannot get entry for darks, get default 10 sec darks");
        defaultDark ();
	break;
      }
      else
      {
        break;
      }
    }
    dark_exposures.push_back (d_img_exposure);
  }
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

ModelTarget::ModelTarget (int in_tar_id, struct ln_lnlat_posn *in_obs):ConstTarget (in_tar_id, in_obs)
{
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
  step++;
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
  // calc ra + dec
  return 0;
}

int
ModelTarget::beforeMove ()
{
  endObservation (-1); // we will not observe same model target twice
  return getNextPosition ();
}

int
ModelTarget::startSlew (struct ln_equ_posn *position)
{
  EXEC SQL BEGIN DECLARE SECTION;
  int d_obs_id;
  int d_step = step;
  EXEC SQL END DECLARE SECTION;
  int ret;

  ret = ConstTarget::startSlew (position);
  if (ret)
    return ret;

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
  return 0;
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
  ln_get_equ_from_hrz (&hrz_poz, observer, JD, pos);
  pos->ra += ra_noise;
  pos->dec += dec_noise;
  return 0;
}

// pick up some opportunity target; don't pick it too often
OportunityTarget::OportunityTarget (int in_tar_id, struct ln_lnlat_posn *in_obs):ConstTarget (in_tar_id, in_obs)
{
}

float
OportunityTarget::getBonus ()
{
  double JD;
  double retBonus = 0;
  struct ln_hrz_posn hrz;
  struct ln_rst_time rst;
  double lunarDist;
  double ha;
  double lastObs;
  time_t now;
  time_t start_t;
  JD = ln_get_julian_from_sys ();
  getAltAz (&hrz, JD);
  getRST (&rst, JD);
  lunarDist = getLunarDistance (JD);
  ha = getHourAngle ();
  lastObs = getLastObsTime ();
  time (&now);
  start_t = now - 86400;
  if (lastObs > 3600 && lastObs < (3 * 3600))
    retBonus += log (lastObs - 3599) * 20;
  if (lunarDist < 60.0)
    retBonus -= lunarDist * 10;
  // bonus for south
  if (ha < 11)
    retBonus += log (12 - ha);
  if (ha > 13)
    retBonus += log (ha - 12);
  retBonus += hrz.alt * 2;
  retBonus -= sin (getNumObs (&start_t, &now) * M_PI_4) * 5;
  return ConstTarget::getBonus () + retBonus;
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
  shouldUpdate = 1;
}

int
TargetGRB::load ()
{
  EXEC SQL BEGIN DECLARE SECTION;
  long  db_grb_date;
  long  db_grb_last_update;
  int db_grb_type;
  int db_tar_id = getTargetID ();
  EXEC SQL END DECLARE SECTION;

  EXEC SQL SELECT
    EXTRACT (EPOCH FROM grb_date),
    EXTRACT (EPOCH FROM grb_last_update),
    grb_type
  INTO
    :db_grb_date,
    :db_grb_last_update,
    :db_grb_type
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
    struct ln_equ_posn other_position;
    in_target->getPosition (&other_position);
    return (getDistance (&other_position) < 4 * in_sep_limit);
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
TargetGRB::getBonus ()
{
  // time from GRB
  time_t now;
  time (&now);
  // special SWIFT handling..
  // last packet we have is SWIFT_ALERT..
  switch (gcnPacketType)
    {
      case 60:
        // we don't get ACK | position within 1 hour..drop our priority to some minumu
        if (now - grbDate > 3600)
          return 1;
        break;
      case 62:
        if (now - lastUpdate > 1800)
          return -1;
	break;
    }
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

int
TargetGRB::isContinues ()
{
  EXEC SQL BEGIN DECLARE SECTION;
  long  db_grb_last_update;
  int db_tar_id = getTargetID ();
  EXEC SQL END DECLARE SECTION;
  // once we detect need to update, we need to update - stop the observation and start new
  if (shouldUpdate)
    return 0;

  EXEC SQL SELECT
    EXTRACT (EPOCH FROM grb_last_update)
  INTO
    :db_grb_last_update
  FROM
    grb
  WHERE
    tar_id = :db_tar_id;
  if (sqlca.sqlcode)
  {
    logMsgDb ("TargetGRB::isContinues");
    return -1;
  }
  return (lastUpdate == db_grb_last_update) ? 1 : 0;
}

TargetSwiftFOV::TargetSwiftFOV (int in_tar_id, struct ln_lnlat_posn *in_obs):Target (in_tar_id, in_obs)
{
  int ret;
  swiftOnBonus = 0;
  target_id = TARGET_SWIFT_FOV;
}

int
TargetSwiftFOV::load ()
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

  load ();

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
  load ();
  if (oldSwiftId != swiftId)
    endObservation (-1);  // startSlew will be called after move suceeded and will write new observation..
  return 0;
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
