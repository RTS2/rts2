#include "rts2targetset.h"
#include "../utils/rts2config.h"
#include "../utils/libnova_cpp.h"

#include <sstream>

void
Rts2TargetSet::load (std::string in_where, std::string order_by)
{
  EXEC SQL BEGIN DECLARE SECTION;
  char *stmp_c;

  int db_tar_id;
  EXEC SQL END DECLARE SECTION;

  std::list <int> target_ids;

  asprintf (&stmp_c, 
  "SELECT "
   "tar_id"
  " FROM "
    "targets"
  " WHERE "
    "%s"
  " ORDER BY %s;",
    in_where.c_str(), order_by.c_str());

  EXEC SQL PREPARE tar_stmp FROM :stmp_c;

  EXEC SQL DECLARE tar_cur CURSOR FOR tar_stmp;

  EXEC SQL OPEN tar_cur;

  while (1)
  {
    EXEC SQL FETCH next FROM tar_cur INTO
      :db_tar_id;
    if (sqlca.sqlcode)
      break;
    target_ids.push_back (db_tar_id);
  }
  if (sqlca.sqlcode != ECPG_NOT_FOUND)
  {
    syslog (LOG_ERR, "Rts2TargetSet::load cannot load targets");
  }
  EXEC SQL CLOSE tar_cur;
  free (stmp_c);
  EXEC SQL ROLLBACK;

  load (target_ids);
}

void
Rts2TargetSet::load (std::list<int> &target_ids)
{
  for (std::list<int>::iterator iter = target_ids.begin(); iter != target_ids.end(); iter++)
  {
    Target *tar = createTarget (*iter, obs);
    if (tar)
      push_back (tar);
  }
}

Rts2TargetSet::Rts2TargetSet (struct ln_equ_posn *pos, double radius, struct ln_lnlat_posn *in_obs)
{
  std::ostringstream os;
  std::ostringstream where_os;
  os << "ln_angular_separation (targets.tar_ra, targets.tar_dec, "
    << pos->ra << ", "
    << pos->dec << ") ";
  where_os << os.str();
  os << "<"
    << radius;
  where_os << " ASC";
  obs = in_obs;
  if (!obs)
    obs = Rts2Config::instance ()->getObserver ();
  load (os.str(), where_os.str());
}

Rts2TargetSet::Rts2TargetSet (std::list<int> &tar_ids, struct ln_lnlat_posn *in_obs)
{
  obs = in_obs;
  if (!obs)
    obs = Rts2Config::instance ()->getObserver ();
  load (tar_ids);
}

Rts2TargetSet::~Rts2TargetSet (void)
{
  for (Rts2TargetSet::iterator iter = begin (); iter != end (); iter++)
  {
    delete *iter;
  }
  clear ();
}

void
Rts2TargetSetGrb::load ()
{
  EXEC SQL BEGIN DECLARE SECTION;
  int db_tar_id;
  EXEC SQL END DECLARE SECTION;

  std::list <int> target_ids;

  EXEC SQL DECLARE grb_tar_cur CURSOR FOR
  SELECT
    tar_id
  FROM
    grb
  ORDER BY
    grb_date DESC;

  EXEC SQL OPEN grb_tar_cur;

  while (1)
  {
    EXEC SQL FETCH next FROM grb_tar_cur INTO
      :db_tar_id;
    if (sqlca.sqlcode)
      break;
    target_ids.push_back (db_tar_id);
  }
  if (sqlca.sqlcode != ECPG_NOT_FOUND)
  {
    syslog (LOG_ERR, "Rts2TargetSet::load cannot load targets");
  }
  EXEC SQL CLOSE grb_tar_cur;
  EXEC SQL ROLLBACK;

  for (std::list<int>::iterator iter = target_ids.begin(); iter != target_ids.end(); iter++)
  {
    TargetGRB *tar = (TargetGRB *) createTarget (*iter, obs);
    if (tar)
      push_back (tar);
  }
}

void
Rts2TargetSet::setTargetEnabled (bool enabled)
{
  for (iterator iter = begin (); iter != end (); iter++)
  {
    (*iter)->setTargetEnabled (enabled);
  }
}

void
Rts2TargetSet::setTargetBonus (float new_bonus)
{
  for (iterator iter = begin (); iter != end (); iter++)
  {
    (*iter)->setTargetBonus (new_bonus);
  }
}

void
Rts2TargetSet::setTargetBonusTime (time_t *new_time)
{
  for (iterator iter = begin (); iter != end (); iter++)
  {
    (*iter)->setTargetBonusTime (new_time);
  }
}

int
Rts2TargetSet::save ()
{
  int ret = 0;
  int ret_s;
  
  for (iterator iter = begin (); iter != end (); iter++)
  {
    ret_s = (*iter)->save ();
    if (ret_s)
      ret--;
  }
  // return number of targets for which save failed
  return ret;
}

Rts2TargetSetGrb::Rts2TargetSetGrb (struct ln_lnlat_posn * in_obs)
{
  obs = in_obs;
  if (!obs)
    obs = Rts2Config::instance ()->getObserver ();
  load ();
}

Rts2TargetSetGrb::~Rts2TargetSetGrb (void)
{
  for (Rts2TargetSetGrb::iterator iter = begin (); iter != end (); iter++)
  {
    delete *iter;
  }
  clear ();
}

void
Rts2TargetSetGrb::printGrbList (std::ostream & _os)
{
  for (Rts2TargetSetGrb::iterator iter = begin (); iter != end (); iter++)
  {
    (*iter)->printGrbList (_os);
  }
  clear ();
}

std::ostream & operator << (std::ostream &_os, Rts2TargetSet &tar_set)
{
  for (Rts2TargetSet::iterator tar_iter = tar_set.begin(); tar_iter != tar_set.end (); tar_iter++)
  {
    _os << (*tar_iter);
  }
  return _os;
}
