#include "rts2sqlcolumn.h"
#include "target.h"

EXEC SQL INCLUDE sql3types;

Rts2SqlColumn::Rts2SqlColumn (const char *in_sql, const char *in_name, int in_order)
{
  if (in_sql)
  {
    sql = new char[strlen (in_sql) + 1];
    strcpy (sql, in_sql);
  }
  else
  {
    sql = NULL;
  }

  if (in_name)
  {
    name = new char[strlen (in_name) + 1];
    strcpy (name, in_name);
  }
  else
  {
    name = NULL;
  }
  order = in_order;
}


Rts2SqlColumn::~Rts2SqlColumn (void)
{
  if (sql)
    delete sql;
  if (name)
    delete name;
}


void
Rts2SqlColumn::processValue (void *val, int sqlType, int isNull)
{
  return;
}


void
Rts2SqlColumn::printStatistics (std::ostream &os)
{
}


bool operator < (Rts2SqlColumn lhs, Rts2SqlColumn rhs)
{
  return lhs.getOrderBy () < rhs.getOrderBy ();
}


/**
 * Defines some extra statistic for ObsState column.
 */
Rts2SqlColumnObsState::Rts2SqlColumnObsState (const char *in_sql, const char *in_name, int in_order):
Rts2SqlColumn (in_sql, in_name, in_order)
{
  interupted = 0;
  acq_failed = 0;
  ok = 0;
}


void
Rts2SqlColumnObsState::processValue (void *val, int sqlType, int isNull)
{
  int trueVal;
  if (sqlType != SQL3_INTEGER)
    return;
  trueVal = *((int *) val);
  if (trueVal & OBS_BIT_INTERUPED)
    interupted++;
  else if (trueVal & OBS_BIT_ACQUSITION_FAI)
    acq_failed++;
  else
    ok++;
}


void
Rts2SqlColumnObsState::printStatistics (std::ostream &os)
{
  os << std::endl;
  os << getName() << " statistics - good: " << ok << " interupted: " << interupted << " acqusition failed: " << acq_failed << std::endl;
  os << std::endl;
}
