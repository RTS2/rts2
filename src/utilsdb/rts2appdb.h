#ifndef __RTS2_APPDB__
#define __RTS2_APPDB__

#include <list>

#include "../utils/rts2app.h"

// for SQL tables display on console..
//
class Rts2SqlColumn
{
  char *sql;
  char *name;
public:
    Rts2SqlColumn (const char *in_sql, const char *in_name);
    virtual ~ Rts2SqlColumn (void);
  char *genSql ()
  {
    return sql;
  }
};

class Rts2SqlQuery
{
private:
  std::list < Rts2SqlColumn * >columns;
  char *sql;
  char *from;
public:
    Rts2SqlQuery (const char *in_from);
    virtual ~ Rts2SqlQuery (void);

  void addColumn (Rts2SqlColumn * add_column);
  void addColumn (const char *in_sql, const char *in_name);
  char *genSql ();
  void display ();
};

class Rts2AppDb:public Rts2App
{
  char *connectString;
  char *configFile;
public:
    Rts2AppDb (int argc, char **argv);
    virtual ~ Rts2AppDb (void);

  virtual int processOption (int in_opt);
  int initDB ();
  virtual int init ();
};

#endif /* !__RTS2_APPDB__ */
