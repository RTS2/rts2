#include "rts2targetset.h"
#include "../utils/rts2config.h"

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

  for (std::list<int>::iterator iter = target_ids.begin(); iter != target_ids.end(); iter++)
  {
    Target *tar = createTarget (*iter, Rts2Config::instance()->getObserver ());
    if (tar)
      push_back (tar);
  }

  target_ids.clear ();
}

Rts2TargetSet::Rts2TargetSet (struct ln_equ_posn *pos, double radius)
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
  load (os.str(), where_os.str());
}

Rts2TargetSet::~Rts2TargetSet (void)
{
  for (Rts2TargetSet::iterator iter = begin (); iter != end (); iter++)
  {
    delete *iter;
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
