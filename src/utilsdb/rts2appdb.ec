#include "rts2appdb.h"
#include "../utils/rts2config.h"

#include <syslog.h>

Rts2AppDb::Rts2AppDb (int argc, char **argv) : Rts2App (argc, argv)
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
