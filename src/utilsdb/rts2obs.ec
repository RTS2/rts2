#include "rts2obs.h"
#include "target.h"
#include "rts2taruser.h"
#include "../utils/libnova_cpp.h"
#include "../utils/timestamp.h"

#include <iostream>
#include <iomanip>
#include <syslog.h>
#include <sstream>

EXEC SQL INCLUDE sqlca;

Rts2Obs::Rts2Obs (int in_obs_id)
{
  obs_id = in_obs_id;
  tar_id = -1;
  tar_type = 0; 
  displayImages = 0;
  displayCounts = 0;
}

Rts2Obs::Rts2Obs (int in_tar_id, char in_tar_type, int in_obs_id, double in_obs_ra, double in_obs_dec, double in_obs_alt, 
      double in_obs_az, double in_obs_slew, double in_obs_start, int in_obs_state, double in_obs_end)
{
  tar_id = in_tar_id;
  tar_type = in_tar_type;
  obs_id = in_obs_id;
  obs_ra = in_obs_ra;
  obs_dec = in_obs_dec;
  obs_alt = in_obs_alt;
  obs_az = in_obs_az;
  obs_slew = in_obs_slew;
  obs_start = in_obs_start;
  obs_state = in_obs_state;
  obs_end = in_obs_end;
}

Rts2Obs::~Rts2Obs (void)
{
  std::vector <Rts2ImageDb *>::iterator img_iter;
  for (img_iter = imgset.begin (); img_iter != imgset.end (); img_iter++)
    delete (*img_iter);
  imgset.clear ();
}

int
Rts2Obs::load ()
{
  EXEC SQL BEGIN DECLARE SECTION;
  VARCHAR db_tar_name[TARGET_NAME_LEN];
  int db_tar_id;
  char db_tar_type;
  int db_obs_id = obs_id;
  double db_obs_ra;
  int ind_obs_ra;
  double db_obs_dec;
  int ind_obs_dec;
  double db_obs_alt;
  int ind_obs_alt;
  double db_obs_az;
  int ind_obs_az;
  double db_obs_slew;
  int ind_obs_slew;
  double db_obs_start;
  int ind_obs_start;
  int db_obs_state;
  double db_obs_end;
  int ind_obs_end;
  EXEC SQL END DECLARE SECTION;

  // already loaded
  if (tar_id > 0)
    return 0;

  obs_ra = nan ("f");
  obs_dec = nan ("f");
  obs_alt = nan ("f");
  obs_az = nan ("f");
  obs_slew = 0;
  obs_start = 0;
  obs_state = 0;
  obs_end = 0;

  EXEC SQL
  SELECT
    tar_name,
    observations.tar_id,
    type_id,
    obs_ra,
    obs_dec,
    obs_alt,
    obs_az,
    EXTRACT (EPOCH FROM obs_slew),
    EXTRACT (EPOCH FROM obs_start),
    obs_state,
    EXTRACT (EPOCH FROM obs_end)
  INTO
    :db_tar_name,
    :db_tar_id,
    :db_tar_type,
    :db_obs_ra     :ind_obs_ra,
    :db_obs_dec    :ind_obs_dec,
    :db_obs_alt    :ind_obs_alt,
    :db_obs_az     :ind_obs_az,
    :db_obs_slew   :ind_obs_slew,
    :db_obs_start  :ind_obs_start,
    :db_obs_state,
    :db_obs_end    :ind_obs_end
  FROM
    observations,
    targets
  WHERE
      obs_id = :db_obs_id
    AND observations.tar_id = targets.tar_id;
  if (sqlca.sqlcode)
  {
    syslog (LOG_DEBUG, "Rts2Obs::load DB error: %s (%li)", sqlca.sqlerrm.sqlerrmc, sqlca.sqlcode);
    EXEC SQL ROLLBACK;
    return -1;
  }
  EXEC SQL COMMIT;
  tar_id = db_tar_id;
  tar_type = db_tar_type;
  if (ind_obs_ra >= 0)
    obs_ra = db_obs_ra;
  if (ind_obs_dec >= 0)
    obs_dec = db_obs_dec;
  if (ind_obs_alt >= 0)
    obs_alt = db_obs_alt;
  if (ind_obs_az >= 0)
    obs_az = db_obs_az;
  if (ind_obs_slew >= 0)
    obs_slew = db_obs_slew;
  if (ind_obs_start >= 0)
    obs_start = db_obs_start;
  obs_state = db_obs_state;
  if (ind_obs_end >= 0)
    obs_end = db_obs_end;
  return 0;
}

int
Rts2Obs::loadImages ()
{
  int ret;
  ret = load ();
  if (ret)
    return ret;

  imgset = Rts2ImgSet (this);

  return imgset.load ();
}

void
Rts2Obs::printImages (std::ostream &_os)
{
  std::vector <Rts2ImageDb *>::iterator img_iter;
  if (imgset.empty ())
  {
    _os << "      " << "--- no images ---" << std::endl;
    return;
  }
  for (img_iter = imgset.begin (); img_iter != imgset.end (); img_iter++)
  {
    _os << "      " << *(*img_iter);
  }
}

void
Rts2Obs::printImagesSummary (std::ostream &_os)
{
  _os << "      " << imgset << std::endl;
}

int
Rts2Obs::loadCounts ()
{
  EXEC SQL BEGIN DECLARE SECTION;
  int db_obs_id = obs_id;
  long db_count_date;
  int db_count_usec;
  int db_count_value;
  float db_count_exposure;
  char db_count_filter;
  EXEC SQL END DECLARE SECTION;

  EXEC SQL DECLARE cur_counts CURSOR FOR
  SELECT
    EXTRACT (EPOCH FROM count_date),
    count_usec,
    count_value,
    count_exposure,
    count_filter
  FROM
    counts
  WHERE
    obs_id = :db_obs_id;

  EXEC SQL OPEN cur_counts;
  while (1)
  {
    EXEC SQL FETCH next FROM cur_counts INTO
      :db_count_date,
      :db_count_usec,
      :db_count_value,
      :db_count_exposure,
      :db_count_filter;
    if (sqlca.sqlcode)
      break;
    counts.push_back (Rts2Count (obs_id, db_count_date, db_count_usec, db_count_value, db_count_exposure,
      db_count_filter));
  }
  if (sqlca.sqlcode != ECPG_NOT_FOUND)
  {
    syslog (LOG_DEBUG, "Rts2Obs::loadCounts DB error: %s (%li)", sqlca.sqlerrm.sqlerrmc, sqlca.sqlcode);
    EXEC SQL CLOSE cur_counts;
    EXEC SQL ROLLBACK;
    return -1;
  }
  EXEC SQL CLOSE cur_counts;
  EXEC SQL COMMIT;
  return 0;
}

void
Rts2Obs::printCounts (std::ostream &_os)
{
  std::vector <Rts2Count>::iterator count_iter;
  if (counts.empty ())
  {
    _os << "      " << "--- no counts ---" << std::endl;
    return;
  }
  for (count_iter = counts.begin (); count_iter != counts.end (); count_iter++)
  {
    _os << "      " << (*count_iter);
  }
}

void
Rts2Obs::printCountsSummary (std::ostream &_os)
{
  _os << "       Number of counts:" << counts.size () << std::endl;
}

int
Rts2Obs::getUnprocessedCount ()
{
  EXEC SQL BEGIN DECLARE SECTION;
  int db_obs_id = getObsId ();
  int db_count = 0;
  EXEC SQL END DECLARE SECTION;

  EXEC SQL
  SELECT
    count (*)
  INTO
    :db_count
  FROM
    images
  WHERE
      obs_id = :db_obs_id
    AND ((process_bitfield & 1) = 0);
  EXEC SQL ROLLBACK;

  return db_count;
}

void
Rts2Obs::checkUnprocessedImages ()
{
  load ();
  if (obs_end == 0)
    return;
  // obs_end is not null - observation ends sucessfully
  // get unprocessed counts..
  if (getUnprocessedCount () == 0)
  {
    Rts2TarUser tar_user = Rts2TarUser (getTargetId (), getTargetType ());
    std::string mails = tar_user.getUsers (SEND_ASTRO_OK);
    std::ostringstream subject;
    subject << "TARGET #" << getTargetId ()
      << ", OBSERVATION " << getObsId ()
      << " ALL IMAGES PROCESSED";
    std::ostringstream os;
    os << *this;
    sendMailTo (subject.str().c_str(), os.str().c_str(), mails.c_str());
  }
}

std::ostream & operator << (std::ostream &_os, Rts2Obs &obs)
{
  std::ios_base::fmtflags old_settings = _os.flags ();
  int old_precision = _os.precision ();

  _os.setf (std::ios_base::fixed, std::ios_base::floatfield);
  _os.precision (2);
  _os << std::setw (8) << obs.obs_id << " | "
    << obs.tar_id << " | "
    << LibnovaRa (obs.obs_ra) << " | "
    << LibnovaDeg90 (obs.obs_dec) << " | "
    << LibnovaDeg90 (obs.obs_alt) << " | "
    << LibnovaDeg (obs.obs_az) << " | "
    << Timestamp (obs.obs_slew) << " | "
    << std::setfill (' ') 
    << std::setw (10) << (obs.obs_start - obs.obs_slew) << " | "
    << std::setw (10) << (obs.obs_end - obs.obs_start) << " | "
    << Rts2ObsState (obs.obs_state);
  if (obs.displayImages)
  {
    obs.loadImages ();
    _os << std::endl;
    if (obs.displayImages & DISPLAY_ALL)
      obs.printImages (_os);
    if (obs.displayImages & DISPLAY_SUMMARY)
      obs.printImagesSummary (_os);
  }
  if (obs.displayCounts)
  {
    obs.loadCounts ();
    _os << std::endl;
    if (obs.displayCounts & DISPLAY_ALL)
      obs.printCounts (_os);
    if (obs.displayCounts & DISPLAY_SUMMARY)
      obs.printCountsSummary (_os);
  }

  _os.flags (old_settings);
  _os.precision (old_precision);

  return _os;
}

std::ostream & operator << (std::ostream &_os, Rts2ObsState obs_state)
{
  if (obs_state.state & OBS_BIT_STARTED)
    _os << 'S';
  else if (obs_state.state & OBS_BIT_MOVED)
    _os << 'M';
  else
    _os << ' ';
    
  if (obs_state.state & OBS_BIT_ACQUSITION_FAI)
    _os << 'F';
  else if (obs_state.state & OBS_BIT_ACQUSITION)
    _os << 'A';
  else
    _os << ' ';

  if (obs_state.state & OBS_BIT_INTERUPED)
    _os << 'I';
  else
    _os << ' ';

  return _os;
}
