#include "rts2targetset.h"
#include "../utils/rts2config.h"

#include <sstream>

void
Rts2TargetSet::load (std::string in_where)
{
  EXEC SQL BEGIN DECLARE SECTION;
  char *stmp_c;

  int db_tar_id;

  EXEC SQL END DECLARE SECTION;

  asprintf (&stmp_c, 
  "SELECT "
   "tar_id"
  " FROM "
    "targets"
  " WHERE "
    "%s"
  " ORDER BY tar_id ASC;",
    in_where.c_str());

  EXEC SQL PREPARE tar_stmp FROM :stmp_c;

  EXEC SQL DECLARE tar_cur CURSOR FOR tar_stmp;

  EXEC SQL OPEN tar_cur;

  while (1)
  {
    EXEC SQL FETCH next FROM tar_cur INTO
      :db_tar_id;
    if (sqlca.sqlcode)
      break;
    Target *tar = createTarget (db_tar_id, Rts2Config::instance()->getObserver ());
    push_back (tar);
  }
  if (sqlca.sqlcode != ECPG_NOT_FOUND)
  {
    syslog (LOG_ERR, "Rts2TargetSet::load cannot load targets");
  }
  EXEC SQL CLOSE tar_cur;
  free (stmp_c);
  EXEC SQL ROLLBACK;
}

Rts2TargetSet::Rts2TargetSet (struct ln_equ_posn *pos, double radius)
{
  std::ostringstream os;
  os << "ln_angular_separation (targets.tar_ra, targets.tar_dec, "
    << pos->ra << ", "
    << pos->dec << ") < "
    << radius;
  load (os.str());
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
