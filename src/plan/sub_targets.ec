#include <libnova/libnova.h>
#include "target.h"

EXEC SQL include sqlca;

// ConstTarget
ConstTarget::ConstTarget (int in_tar_id, struct ln_lnlat_posn *in_obs):
Target (in_tar_id, in_obs)
{
  EXEC SQL BEGIN DECLARE SECTION;
  double ra;
  double dec;
  int db_tar_id = in_tar_id;
  EXEC SQL END DECLARE SECTION;

  EXEC SQL
  SELECT 
    tar_ra, tar_dec 
  INTO
    :ra, :dec
  FROM
    targets
  WHERE
    tar_id = :db_tar_id;
  if (sqlca.sqlcode)
  {
    throw &sqlca;
  }
  position.ra = ra;
  position.dec = dec;
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
  return ln_get_object_rst (JD, observer, &pos, rst);
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
  lastUpdate = db_grb_last_update;
}

int
TargetGRB::getScript (const char *deviceName, char *buf)
{
  time_t now;
  time (&now);
  // switch based on time after burst..
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

TargetSwiftFOV::TargetSwiftFOV (int in_tar_id, struct ln_lnlat_posn *in_obs):Target (in_tar_id, in_obs)
{
  int ret;
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
    if (testHrz.alt > -30)
    {
      if (testHrz.alt < 30)
        testHrz.alt = 30;
      // get equ coordinates we will observe..
      ln_get_equ_from_hrz (&testHrz, observer, JD, &testEqu);
      swiftFovCenter.ra = testEqu.ra;
      swiftFovCenter.dec = testEqu.dec;
      swiftId = d_swift_id;
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
TargetSwiftFOV::startObservation ()
{
  EXEC SQL BEGIN DECLARE SECTION;
  int d_obs_id;
  int d_swift_id = swiftId;
  EXEC SQL END DECLARE SECTION;
  int ret;
  
  ret = Target::startObservation ();
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
    logMsgDb ("TargetSwiftFOV::startObservation SQL error");
    EXEC SQL ROLLBACK;
    return -1;
  }
  EXEC SQL COMMIT;
  return 0;
}

int
TargetSwiftFOV::beforeMove ()
{
  int oldSwiftId = swiftId;
  // are we still the best swiftId on planet?
  findPointing ();
  if (oldSwiftId != swiftId)
    endObservation ();  // startObservation will be called after move suceeded and will write new observation..
}
