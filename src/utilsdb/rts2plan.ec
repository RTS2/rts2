#include "../utils/rts2config.h"
#include "../utils/libnova_cpp.h"
#include "rts2plan.h"

#include <ostream>
#include <sstream>
#include <iomanip>

Rts2Plan::Rts2Plan ()
{
  plan_id = -1;
  tar_id = -1;
  prop_id = -1;
  target = NULL;
  obs_id = -1;
  observation = NULL;
  plan_status = 0;
}

Rts2Plan::Rts2Plan (int in_plan_id)
{
  plan_id = in_plan_id;
  tar_id = -1;
  prop_id = -1;
  target = NULL;
  obs_id = -1;
  observation = NULL;
  plan_status = 0;
}

Rts2Plan::~Rts2Plan (void)
{
  delete target;
}

int
Rts2Plan::load ()
{
  EXEC SQL BEGIN DECLARE SECTION;
  int db_plan_id = plan_id;
  int db_prop_id;
  int db_prop_id_ind;
  int db_tar_id;
  int db_obs_id;
  int db_obs_id_ind;
  double db_plan_start;
  int db_plan_status;
  EXEC SQL END DECLARE SECTION;

  EXEC SQL SELECT
    prop_id,
    tar_id,
    obs_id,
    EXTRACT (EPOCH FROM plan_start),
    plan_status
  INTO
    :db_prop_id :db_prop_id_ind,
    :db_tar_id,
    :db_obs_id :db_obs_id_ind,
    :db_plan_start,
    :db_plan_status
  FROM
    plan
  WHERE
    plan_id = :db_plan_id;
  if (sqlca.sqlcode)
  {
    EXEC SQL ROLLBACK;
    return -1;
  }
  if (db_prop_id_ind)
    prop_id = -1;
  else
    prop_id = db_prop_id;
  tar_id = db_tar_id;
  if (db_obs_id_ind)
    db_obs_id = -1;
  else
    obs_id = db_obs_id;
  plan_start = (long) db_plan_start;
  plan_status = db_plan_status;
  EXEC SQL COMMIT;
  return 0;
}

int
Rts2Plan::save ()
{
  EXEC SQL BEGIN DECLARE SECTION;
  int db_plan_id = plan_id;
  int db_tar_id = tar_id;
  int db_prop_id = prop_id;
  int db_prop_id_ind;
  int db_obs_id = obs_id;
  int db_obs_id_ind;
  long db_plan_start = plan_start;
  int db_plan_status = plan_status;
  EXEC SQL END DECLARE SECTION;

  if (db_plan_id == -1)
  {
    EXEC SQL
    SELECT
      nextval ('plan_id')
    INTO
      :db_plan_id;
    if (sqlca.sqlcode)
    {
      std::cerr << "Error getting nextval" << sqlca.sqlcode << sqlca.sqlerrm.sqlerrmc << std::endl;
      return -1;
    }
  }

  if (db_prop_id == -1)
  {
    db_prop_id = 0;
    db_prop_id_ind = -1;
  }
  else
  {
    db_prop_id_ind = 0;
  }

  if (db_obs_id == -1)
  {
    db_obs_id = 0;
    db_obs_id_ind = -1;
  }
  else
  {
    db_obs_id_ind = 0;
  }

  EXEC SQL INSERT INTO plan (
    plan_id,
    tar_id,
    prop_id,
    obs_id,
    plan_start,
    plan_status
  )
  VALUES (
    :db_plan_id,
    :db_tar_id,
    :db_prop_id :db_prop_id_ind,
    :db_obs_id :db_obs_id_ind,
    abstime (:db_plan_start),
    :db_plan_status
  );
  if (sqlca.sqlcode)
  {
    std::cerr << "Error inserting plan " << sqlca.sqlcode << sqlca.sqlerrm.sqlerrmc
      << " prop_id:" << db_prop_id << " ind:" << db_prop_id_ind << std::endl;
    // try update
    EXEC SQL UPDATE
      plan
    SET
      tar_id = :db_tar_id,
      prop_id = :db_prop_id :db_prop_id_ind,
      obs_id = :db_obs_id :db_obs_id_ind,
      plan_start = abstime (:db_plan_start),
      plan_status = :db_plan_status
    WHERE
      plan_id = :db_plan_id;
    if (sqlca.sqlcode)
    {
      std::cerr << "Error updating plan " << sqlca.sqlcode << sqlca.sqlerrm.sqlerrmc << std::endl;
      EXEC SQL ROLLBACK;
      return -1;
    }
  }
  EXEC SQL COMMIT;
  return 0;
}

int
Rts2Plan::del ()
{
  EXEC SQL BEGIN DECLARE SECTION;
  int db_plan_id = plan_id;
  EXEC SQL END DECLARE SECTION;

  if (db_plan_id == -1)
    return 0;

  EXEC SQL DELETE FROM
    plan
  WHERE
    plan_id = :db_plan_id;
  if (sqlca.sqlcode)
  {
    EXEC SQL ROLLBACK;
    return -1;
  }
  EXEC SQL COMMIT;
  return 0;
}

int
Rts2Plan::startSlew (struct ln_equ_posn *position)
{
  EXEC SQL BEGIN DECLARE SECTION;
  int db_plan_id = plan_id;
  int db_obs_id;
  EXEC SQL END DECLARE SECTION;
  int ret;
  ret = getTarget ()->startSlew (position);
  if (obs_id > 0)
    return ret;

  obs_id = getTarget()->getObsId ();
  db_obs_id = obs_id;

  EXEC SQL
  UPDATE
    plan
  SET
    obs_id = :db_obs_id,
    plan_status = plan_status | 1
  WHERE
    plan_id = :db_plan_id;
  if (sqlca.sqlcode)
  {
    syslog (LOG_ERR, "Rts2Plan::startSlew %s (%li)", sqlca.sqlerrm.sqlerrmc, sqlca.sqlcode);
    EXEC SQL ROLLBACK;
  }
  else
  {
    EXEC SQL COMMIT;
  }
  return ret;
}

Target *
Rts2Plan::getTarget ()
{
  if (target)
    return target;
  target = createTarget (tar_id);
  return target;
}

Rts2Obs *
Rts2Plan::getObservation ()
{
  int ret;
  if (observation)
    return observation;
  if (obs_id <= 0)
    return NULL;
  observation = new Rts2Obs (obs_id);
  ret = observation->load ();
  if (ret)
  {
    delete observation;
    observation = NULL;
  }
  return observation;
}

std::ostream & operator << (std::ostream & _os, Rts2Plan * plan)
{
  struct ln_hrz_posn hrz;
  time_t plan_start;
  const char *tar_name;
  struct ln_lnlat_posn *obs;
  int good;
  double JD;
  int ret;
  plan_start = plan->getPlanStart ();
  JD = ln_get_julian_from_timet (&plan_start);
  ret = plan->load ();
  if (ret)
  {
    tar_name = "(not loaded!)";
    good = 0;
  }
  else
  {
    tar_name = plan->getTarget()->getTargetName();
    good = plan->getTarget()->isGood (JD);
  }
  obs = Rts2Config::instance()->getObserver ();
  plan->getTarget ()->getAltAz (&hrz, JD);
  _os << "  " << std::setw (8) << plan->plan_id << "|"
    << std::setw (8) << plan->prop_id << "|"
    << std::left << std::setw (20) << tar_name << "|"
    << std::right << std::setw (8) << plan->tar_id << "|"
    << std::setw (8) << plan->obs_id << "|"
    << std::setw (9) << LibnovaDate (&(plan->plan_start)) << "|"
    << std::setw (8) << plan->plan_status << "|"
    << LibnovaDeg90 (hrz.alt) << "|"
    << LibnovaDeg (hrz.az) << "|"
    << std::setw(1) << (good ? 'G' : 'B')
    << std::endl;

  return _os;
}

std::istream & operator >> (std::istream & _is, Rts2Plan & plan)
{
  char buf[201];
  char *comt;
  _is.getline (buf, 200);
  // ignore comment..
  comt = strchr (buf, '#');
  if (comt)
    *comt = '\0';
 
 std::istringstream buf_s (buf);
 LibnovaDate start_date;
 buf_s >> plan.tar_id >> start_date;
 start_date.getTimeT (& plan.plan_start);

 return _is;
}
