#include "rts2db/sqlcolumn.h"
#include "rts2db/target.h"

EXEC SQL INCLUDE sql3types;

using namespace rts2db;

SqlColumn::SqlColumn (const char *in_sql, const char *in_name, int in_order)
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

SqlColumn::~SqlColumn (void)
{
	delete sql;
	delete name;
}

void SqlColumn::processValue (__attribute__ ((unused)) void *val, __attribute__ ((unused)) int sqlType, __attribute__ ((unused)) int isNull)
{
	return;
}

void SqlColumn::printStatistics (__attribute__ ((unused)) std::ostream &os)
{
}


bool operator < (SqlColumn lhs, SqlColumn rhs)
{
	return lhs.getOrderBy () < rhs.getOrderBy ();
}

/**
 * Defines some extra statistic for ObsState column.
 */
SqlColumnObsState::SqlColumnObsState (const char *in_sql, const char *in_name, int in_order):SqlColumn (in_sql, in_name, in_order)
{
	interupted = 0;
	acq_failed = 0;
	ok = 0;
}

void SqlColumnObsState::processValue (void *val, int sqlType, __attribute__ ((unused)) int isNull)
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

void SqlColumnObsState::printStatistics (std::ostream &os)
{
	os << std::endl;
	os << getName() << " statistics - good: " << ok << " interupted: " << interupted << " acqusition failed: " << acq_failed << std::endl;
	os << std::endl;
}
