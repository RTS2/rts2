#include "rts2obsset.h"

void
Rts2ObsSet::load (std::string in_where)
{
  EXEC SQL BEGIN DECLARE SECTION;
  const char *db_where = in_where.c_str();
  
  VARCHAR db_tar_name[TARGET_NAME_LEN];
  int db_tar_id;
  int db_obs_id;
  double db_obs_ra;
  double db_obs_dec;
  double db_obs_alt;
  double db_obs_az;
  double db_obs_slew;
  double db_obs_start;
  int db_obs_state;
  double db_obs_end;

  int db_tar_ind;
  int db_obs_ra_ind;
  int db_obs_dec_ind;
  int db_obs_alt_ind;
  int db_obs_az_ind;
  int db_obs_slew_ind;
  int db_obs_start_ind;
  int db_obs_state_ind;
  int db_obs_end_ind;
  EXEC SQL END DECLARE SECTION;

  initObsSet ();

  EXEC SQL DECLARE obs_cur_timestamps CURSOR FOR
  SELECT
    tar_name,
    observations.tar_id,
    obs_id,
    obs_ra,
    obs_dec,
    obs_alt,
    obs_az,
    EXTRACT (EPOCH FROM obs_slew),
    EXTRACT (EPOCH FROM obs_start),
    obs_state,
    EXTRACT (EPOCH FROM obs_end)
  FROM
    observations, targets
  WHERE
      observations.tar_id = targets.tar_id
    AND :db_where
  ORDER BY
    obs_id DESC;

  EXEC SQL OPEN obs_cur_timestamps;
  while (1)
  {
    EXEC SQL FETCH next FROM obs_cur_timestamps INTO
      :db_tar_name :db_tar_ind,
      :db_tar_id,
      :db_obs_id,
      :db_obs_ra :db_obs_ra_ind,
      :db_obs_dec :db_obs_dec_ind,
      :db_obs_alt :db_obs_alt_ind,
      :db_obs_az :db_obs_az_ind,
      :db_obs_slew :db_obs_slew_ind,
      :db_obs_start :db_obs_start_ind,
      :db_obs_state :db_obs_state_ind,
      :db_obs_end :db_obs_end_ind;
    if (sqlca.sqlcode)
      break;
    // add new observations to vector
    observations.push_back (Rts2Obs (db_tar_id, db_obs_id, db_obs_ra, db_obs_dec, db_obs_alt,
      db_obs_az, db_obs_slew, db_obs_start, db_obs_state, db_obs_end));
  }
  if (sqlca.sqlcode != ECPG_NOT_FOUND)
  {
    syslog (LOG_ERR, "Rts2ObsSet::Rts2ObsSet cannot load observation set");
  }
  EXEC SQL CLOSE obs_cur_timestamps;
  EXEC SQL ROLLBACK;
}

Rts2ObsSet::Rts2ObsSet (int in_tar_id, const time_t * start_t, const time_t * end_t)
{
  std::string w = std::string ("observations.tar_id = ");
  w += in_tar_id;
  w += " AND observations.obs_slew >= abstime (";
  w += *start_t;
  w += ") AND (observations.obs_end is NULL OR observations.obs_end < abstime (";
  w += *end_t;
  w += "))";
  load (w);
}

Rts2ObsSet::Rts2ObsSet (const time_t * start_t, const time_t * end_t)
{
  std::string w = std::string ("observations.obs_slew >= abstime (");
  w += *start_t;
  w += ") AND (observations.obs_end is NULL OR observations.obs_end < abstime (";
  w += *end_t;
  w += "))";
  load (w);
}

Rts2ObsSet::Rts2ObsSet (int in_tar_id)
{
  std::string w = std::string ("observations.tar_id = ");
  w += in_tar_id;
  load (w);
}


Rts2ObsSet::~Rts2ObsSet (void)
{
  observations.clear ();
}

std::ostream & operator << (std::ostream &_os, Rts2ObsSet &obs_set)
{
  std::vector <Rts2Obs>::iterator obs_iter;
  _os << "Observations list follow:" << std::endl;
  for (obs_iter = obs_set.observations.begin (); obs_iter != obs_set.observations.end (); obs_iter++)
  {
    (*obs_iter).setPrintImages (obs_set.getPrintImages ());
    (*obs_iter).setPrintCounts (obs_set.getPrintCounts ());
    _os << (*obs_iter);
    _os << std::endl;
  }
  return _os;
}
