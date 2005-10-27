#include "rts2appdb.h"
#include "../utils/rts2config.h"

#include <ecpgtype.h>

#include <iostream>
#include <iomanip>
#include <string>

#include <syslog.h>

EXEC SQL INCLUDE sql3types;

std::ostream & operator << (std::ostream & _os, struct tm *print_t)
{
  static char buf[50];
  strftime (buf, 50, "%c", print_t);
  _os << buf;
  return _os;
}

std::ostream & operator << (std::ostream & _os, time_t *print_tt)
{
  static struct tm *print_t;
  print_t = gmtime (print_tt);
  _os << print_t;
  return _os;
}

Rts2SqlQuery::Rts2SqlQuery (const char *in_from)
{
  sql = NULL;
  if (in_from)
  {
    from = new char[strlen (in_from) + 1];
    strcpy (from, in_from);
  }
  else
  {
    from = NULL;
  }
  where = NULL;
}

Rts2SqlQuery::~Rts2SqlQuery (void)
{
  std::list <Rts2SqlColumn *>::iterator col_iter1, col_iter2;
  if (sql)
    delete sql;
  if (from)
    delete from;
  for (col_iter1 = columns.begin (); col_iter1 != columns.end (); )
  {
    col_iter2 = col_iter1;
    col_iter1++;
    delete *col_iter2;
  }
  columns.clear ();
  if (where)
    delete[] where;
}

void
Rts2SqlQuery::addColumn (Rts2SqlColumn *add_column)
{
  columns.push_back (add_column);
  if (sql)
    delete sql;
  sql = NULL;
}

void
Rts2SqlQuery::addColumn (const char *in_sql, const char *in_name, int in_order)
{
  Rts2SqlQuery::addColumn (new Rts2SqlColumn (in_sql, in_name, in_order));
}

void
Rts2SqlQuery::addWhere (const char *in_where)
{
  if (!where)
  {
    where = new char[strlen (in_where) + 1];
    strcpy (where, in_where);
  }
  else
  {
    char *new_where = new char[strlen (where) + strlen (in_where) + 6];
    strcpy (new_where, where);
    delete[] where;
    strcat (new_where, " and ");
    strcat (new_where, in_where);
    where = new_where;
  }
}

char *
Rts2SqlQuery::genSql ()
{
  std::string query;
  std::string order_by;
  std::list <Rts2SqlColumn *>::iterator col_iter1;
  std::list <Rts2SqlColumn *> col_copy;
  if (sql)
    return sql;
  query = std::string ();
  query += "select ";
  for (col_iter1 = columns.begin (); col_iter1 != columns.end (); col_iter1++)
  {
    char *col_sql = (*col_iter1)->genSql ();
    if (col_iter1 != columns.begin ())
      query += ", ";
    query += col_sql;
  }
  // add from
  if (from)
  {
    query += " from ";
    query += from;
  }
  if (where)
  {
    query += " where ";
    query += where;
  }
  order_by = std::string ();
  // order by part..
  col_copy = std::list <Rts2SqlColumn *> (columns);
  col_copy.sort ();
  for (col_iter1 = col_copy.begin (); col_iter1 != col_copy.end (); col_iter1++)
  {
    Rts2SqlColumn *col;
    col = (*col_iter1);
    if (col->getOrderBy () != 0)
    {
      if (order_by.length () == 0)
	order_by += " order by ";
      else
	order_by += ", ";

      order_by += col->genSql ();
      if (col->getOrderBy () > 0)
	order_by += " asc";
      else
	order_by += " desc";
    }
  }
  query += order_by;

  sql = new char[query.length() + 1];
  strcpy (sql, query.c_str ());
  return sql;
}

void
Rts2SqlQuery::displayMinusPlusLine ()
{
  std::list <Rts2SqlColumn *>::iterator col_iter;
  for (col_iter = columns.begin (); col_iter != columns.end (); col_iter++)
  {
    if (col_iter != columns.begin ())
      std::cout << "-+-";
    for (int i = 0; i < (*col_iter)->getFieldSize (); i++)
      std::cout << '-';
  }
  std::cout << std::endl;
}

void
Rts2SqlQuery::display ()
{
  EXEC SQL BEGIN DECLARE SECTION;
  char *stmp;
  int row;
  int cols;
  int cur_col;
  int col_car, scale, precision;
  int type;

  bool d_bool;
  int d_int;
  float d_float;
  double d_double;
  char d_string[1024];

  int len;
  int is_null;

  EXEC SQL END DECLARE SECTION;

  std::list <Rts2SqlColumn *>::iterator col_iter;

  EXEC SQL ALLOCATE DESCRIPTOR disp_desc;

  std::cout << genSql () << std::endl;
  // try to execute query
  stmp = genSql ();

  EXEC SQL PREPARE disp_stmp FROM :stmp;

  EXEC SQL DECLARE disp_cur CURSOR FOR disp_stmp;
  EXEC SQL OPEN disp_cur;

  row = 0;
  
  while (1)
  {
    EXEC SQL FETCH next FROM disp_cur INTO DESCRIPTOR disp_desc;

    if (sqlca.sqlcode)
    {
      if (sqlca.sqlcode != ECPG_NOT_FOUND)
        std::cerr << "Sql error " << sqlca.sqlerrm.sqlerrmc << std::endl;
      break;
    }

    EXEC SQL GET DESCRIPTOR disp_desc :cols = COUNT;

    if (row == 0)
    {
      cur_col = 1;
      for (col_iter = columns.begin (); col_iter != columns.end (); col_iter++)
      {
        EXEC SQL GET DESCRIPTOR disp_desc VALUE :cur_col
	  :type = TYPE,
	  :len = RETURNED_OCTET_LENGTH;
	switch (type)
	{
	  case SQL3_BOOLEAN:
	    len = 5;
	    break;
          case SQL3_NUMERIC:
	  case SQL3_DECIMAL:
	    len = 8;
	    break;
	  case SQL3_INTEGER:
	    len = 10;
	    break;
	  case SQL3_SMALLINT:
	    len = 6;
	    break;
	  case SQL3_FLOAT:
	  case SQL3_REAL:
	    len = 8;
	    break;
	  case SQL3_DOUBLE_PRECISION:
	    len = 10;
	    break;
	  case SQL3_DATE_TIME_TIMESTAMP:
	  case SQL3_INTERVAL:
	    len = 28;
	    break;
	  case SQL3_CHARACTER:
	  case SQL3_CHARACTER_VARYING:
	  default:
	    // leave empty..
	    break;
	}
	if (cur_col > 1)
	{
	  std::cout << " | ";
	}
	std::cout << std::setw (len) << (*col_iter)->getHeader (len);
	cur_col++;
      }
      std::cout << std::endl;
      // display -+ line
      displayMinusPlusLine ();
    }

    row++;

    for (cur_col = 1, col_iter = columns.begin (); cur_col <= cols && col_iter != columns.end (); cur_col++, col_iter++)
    {
      if (cur_col > 1)
	std::cout << " | ";
      EXEC SQL GET DESCRIPTOR disp_desc VALUE :cur_col
      	:type = TYPE,
	:scale = SCALE,
	:precision = PRECISION,
	:is_null = INDICATOR,
        :col_car = CARDINALITY;
      // unsigned short..
      len = (*col_iter)->getFieldSize ();
      switch (type)
      {
	case SQL3_BOOLEAN:
	  EXEC SQL GET DESCRIPTOR disp_desc VALUE :cur_col :d_bool = DATA;
	  std::cout << std::setw(len) << (d_bool ? "true" : "false");
	  (*col_iter)->processValue (&d_bool, type, is_null);
	  break;
	case SQL3_NUMERIC:
	case SQL3_DECIMAL:
	  if (scale == 0)
	  {
	    EXEC SQL GET DESCRIPTOR disp_desc VALUE :cur_col :d_int = DATA;
	    std::cout << std::setw(len) << d_int;
	    (*col_iter)->processValue (&d_int, type, is_null);
	  }
	  else
	  {
	    EXEC SQL GET DESCRIPTOR disp_desc VALUE :cur_col :d_double = DATA;
	    std::cout << std::setw(len) << d_double;
	    (*col_iter)->processValue (&d_double, type, is_null);
	  }
	  break;
	case SQL3_INTEGER:
	case SQL3_SMALLINT:
	  EXEC SQL GET DESCRIPTOR disp_desc VALUE :cur_col :d_int = DATA;
	  std::cout << std::setw(len) << d_int;
	  (*col_iter)->processValue (&d_int, type, is_null);
	  break;
	case SQL3_FLOAT:
	case SQL3_REAL:
	  EXEC SQL GET DESCRIPTOR disp_desc VALUE :cur_col :d_float = DATA;
	  std::cout << std::setw(len) << d_float;
	  (*col_iter)->processValue (&d_float, type, is_null);
	  break;
	case SQL3_DOUBLE_PRECISION:
	  EXEC SQL GET DESCRIPTOR disp_desc VALUE :cur_col :d_double = DATA;
	  std::cout << std::setw(len) << d_double;
	  (*col_iter)->processValue (&d_double, type, is_null);
	  break;
	case SQL3_DATE_TIME_TIMESTAMP:
	case SQL3_INTERVAL:
	case SQL3_CHARACTER:
	case SQL3_CHARACTER_VARYING:
	default:
	  EXEC SQL GET DESCRIPTOR disp_desc VALUE :cur_col :d_string = DATA;
	  std::cout << std::setw(len) << std::setprecision(len) << d_string;
	  (*col_iter)->processValue (&d_string, type, is_null);
	  break;
      }
    }
    std::cout << std::endl;
  }

  displayMinusPlusLine ();

  std::cout << std::endl << row << " rows were displayed" << std::endl << std::endl;

  EXEC SQL CLOSE disp_cur;
  EXEC SQL DEALLOCATE DESCRIPTOR disp_desc;
}

Rts2AppDb::Rts2AppDb (int in_argc, char **in_argv) : Rts2App (in_argc, in_argv)
{
  connectString = NULL;
  configFile = NULL;

  addOption ('b', "database", 1, "connect string to PSQL database (default to stars)");
  addOption ('c', "config", 1, "configuration file");
}

Rts2AppDb::~Rts2AppDb ()
{
  EXEC SQL DISCONNECT;
  if (connectString)
    delete connectString;
}

int
Rts2AppDb::processOption (int in_opt)
{
  switch (in_opt)
  {
    case 'b':
      connectString = new char[strlen (optarg) + 1];
      strcpy (connectString, optarg);
      break;
    case 'c':
      configFile = optarg;
      break;
    default:
      return Rts2App::processOption (in_opt);
  }
  return 0;
}

int
Rts2AppDb::initDB ()
{
  int ret;
  EXEC SQL BEGIN DECLARE SECTION;
  char conn_str[200];
  EXEC SQL END DECLARE SECTION;
  // try to connect to DB

  Rts2Config *config;

  // load config..

  config = Rts2Config::instance ();
  ret = config->loadFile (configFile);
  if (ret)
    return ret;
  
  if (connectString)
  {
    strncpy (conn_str, connectString, 200);
  }
  else
  {
    config->getString ("database", "name", conn_str, 200);
  }
  conn_str[199] = '\0';

  EXEC SQL CONNECT TO :conn_str;
  if (sqlca.sqlcode != 0)
  {
    syslog (LOG_ERR, "Rts2DeviceDb::init Cannot connect to DB %s: %s", conn_str, sqlca.sqlerrm.sqlerrmc); 
    return -1;
  }

  return 0;
}

int
Rts2AppDb::init ()
{
  int ret;

  ret = Rts2App::init ();
  if (ret)
    return ret;

  // load config.
  return initDB ();
}
