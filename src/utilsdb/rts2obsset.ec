#include "rts2obsset.h"

#include <sstream>

#include "../utils/libnova_cpp.h"

void
Rts2ObsSet::load (std::string in_where)
{
  EXEC SQL BEGIN DECLARE SECTION;
  char *stmp_c;
  
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
  char db_tar_type;
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

  asprintf (&stmp_c, 
  "SELECT "
   "tar_name,"
   "observations.tar_id,"
   "type_id,"
   "obs_id,"
   "obs_ra,"
   "obs_dec,"
   "obs_alt,"
   "obs_az,"
   "EXTRACT (EPOCH FROM obs_slew),"
   "EXTRACT (EPOCH FROM obs_start),"
   "obs_state,"
   "EXTRACT (EPOCH FROM obs_end)"
  " FROM "
    "observations, targets"
  " WHERE "
      "observations.tar_id = targets.tar_id "
    "AND %s"
    " ORDER BY obs_id DESC;",
    in_where.c_str());

  EXEC SQL PREPARE obs_stmp FROM :stmp_c;
  
  EXEC SQL DECLARE obs_cur_timestamps CURSOR FOR obs_stmp;

  EXEC SQL OPEN obs_cur_timestamps;
  while (1)
  {
    EXEC SQL FETCH next FROM obs_cur_timestamps INTO
      :db_tar_name :db_tar_ind,
      :db_tar_id,
      :db_tar_type,
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
    if (db_obs_ra_ind < 0)
      db_obs_ra = nan("f");
    if (db_obs_dec_ind < 0)
      db_obs_dec = nan("f");
    if (db_obs_alt_ind < 0)
      db_obs_alt = nan("f");
    if (db_obs_az_ind < 0)
      db_obs_az = nan("f");
    if (db_obs_slew_ind < 0)
      db_obs_slew = nan("f");
    if (db_obs_start_ind < 0)
      db_obs_start = nan("f");
    if (db_obs_state_ind < 0)
      db_obs_state = 0;
    if (db_obs_end_ind < 0)
      db_obs_end = nan("f");

    // add new observations to vector
    Rts2Obs obs = Rts2Obs (db_tar_id, db_tar_type, db_obs_id, db_obs_ra, db_obs_dec, db_obs_alt,
      db_obs_az, db_obs_slew, db_obs_start, db_obs_state, db_obs_end);
    observations.push_back (obs);
    if (db_obs_state & OBS_BIT_STARTED)
    {
      if (db_obs_state & OBS_BIT_ACQUSITION_FAI)
	failedNum++;
      else
	successNum++;
    }
  }
  if (sqlca.sqlcode != ECPG_NOT_FOUND)
  {
    syslog (LOG_ERR, "Rts2ObsSet::Rts2ObsSet cannot load observation set");
  }
  EXEC SQL CLOSE obs_cur_timestamps;
  free (stmp_c);
  EXEC SQL ROLLBACK;
}

Rts2ObsSet::Rts2ObsSet (int in_tar_id, const time_t * start_t, const time_t * end_t)
{
  std::ostringstream os;
  os << "observations.tar_id = "
     << in_tar_id
     << " AND observations.obs_slew >= abstime ("
     << *start_t
     << ") AND ((observations.obs_slew <= abstime ("
     << *end_t
     << ") AND (observations.obs_end is NULL OR observations.obs_end < abstime ("
     << *end_t
     << "))";
  load (os.str());
}

Rts2ObsSet::Rts2ObsSet (const time_t * start_t, const time_t * end_t)
{
  std::ostringstream os;
  os << "observations.obs_slew >= abstime ("
     << *start_t
     << ") AND ((observations.obs_slew <= abstime ("
     << *end_t
     << ") AND observations.obs_end is NULL) OR observations.obs_end < abstime ("
     << *end_t
     << "))";
  load (os.str());
}

Rts2ObsSet::Rts2ObsSet (int in_tar_id)
{
  std::ostringstream os;
  os << "observations.tar_id = "
     << in_tar_id;
  load (os.str());
}

Rts2ObsSet::~Rts2ObsSet (void)
{
  observations.clear ();
}

int
Rts2ObsSet::computeStatistics ()
{
  int anum = 0;

  std::vector <Rts2Obs>::iterator obs_iter;
  allNum = 0;
  goodNum = 0;
  firstNum = 0;

  errFirstRa = 0;
  errFirstDec = 0;
  errFirstRad = 0;
  
  errAvgRa = 0;
  errAvgDec = 0;
  errAvgRad = 0;

  for (obs_iter = observations.begin (); obs_iter != observations.end (); obs_iter++)
  {
    double efRa, efDec, efRad;
    double eaRa, eaDec, eaRad;
    int ret;
    allNum += (*obs_iter).getNumberOfImages ();
    goodNum += (*obs_iter).getNumberOfGoodImages ();
    ret = (*obs_iter).getFirstErrors (efRa, efDec, efRad);
    if (!ret)
    {
      errFirstRa += efRa;
      errFirstDec += efDec;
      errFirstRad += efRad;
      firstNum++;
    }
    ret = (*obs_iter).getAverageErrors (eaRa, eaDec, eaRad);
    if (ret > 0)
    {
      errAvgRa += eaRa;
      errAvgDec += eaDec;
      errAvgRad += eaRad;
      anum ++;
    }
  }

  errFirstRa /= firstNum;
  errFirstDec /= firstNum;
  errFirstRad /= firstNum;

  errAvgRa /= anum;
  errAvgDec /= anum;
  errAvgRad /= anum;
  
  return 0;
}

void
Rts2ObsSet::printStatistics (std::ostream & _os)
{
  computeStatistics ();
  int success = getSuccess ();
  int failed = getFailed ();
  int prec = _os.precision (2);
  _os << "Number of observations: " << observations.size () << std::endl
    << "Succesfully ended:" << success << std::endl
    << "Failed:" << failed << std::endl
    << "Number of images:" << allNum 
    << " with astrometry:" << goodNum
    << " without astrometry:" << (allNum - goodNum);
  if (allNum > 0)
  {
    _os << " (" << (((double)(goodNum * 100)) / allNum) << "%)";
  }
  _os << std::endl;
  if (goodNum > 0)
  {
    _os
    << "First images errors: ra " << LibnovaDegArcMin (errFirstRa)
    << " dec: " << LibnovaDegArcMin (errFirstDec)
    << " radius: " << LibnovaDegArcMin (errFirstRad) << std::endl
    << "Average error ra: " << LibnovaDegArcMin (errAvgRa)
    << " dec: " << LibnovaDegArcMin (errAvgDec)
    << " radius: " << LibnovaDegArcMin (errAvgRad);
  }
  _os.precision (prec);
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
