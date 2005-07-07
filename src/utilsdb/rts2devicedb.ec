#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

/**
 * This add database connectivity to device class
 */

#include "rts2devicedb.h"

EXEC SQL include sqlca;

Rts2DeviceDb::Rts2DeviceDb (int in_argc, char **in_argv, int in_device_type,
   int default_port, char *default_name):Rts2Device (in_argc, in_argv, in_device_type, default_port, default_name)
{
  connectString = "stars"; // defualt DB

  addOption ('b', "database", 1, "connect string to PSQL database (default to stars)");
}

Rts2DeviceDb::~Rts2DeviceDb (void)
{
  EXEC SQL DISCONNECT;
  if (connectString)
    delete connectString;
}

int
Rts2DeviceDb::processOption (int in_opt)
{
  switch (in_opt)
  {
    case 'b':
      connectString = new char[strlen (optarg) + 1];
      strcpy (connectString, optarg);
      break;
    default:
      return Rts2Device::processOption (in_opt);
  }
  return 0;
}

int
Rts2DeviceDb::init ()
{
  int ret;
  EXEC SQL BEGIN DECLARE SECTION;
  char *conn_str;
  EXEC SQL END DECLARE SECTION;
  // try to connect to DB

  ret = Rts2Device::init ();
  if (ret)
    return ret;
  
  conn_str = connectString;

  EXEC SQL CONNECT TO :conn_str;
  if (sqlca.sqlcode != 0)
  {
    syslog (LOG_ERR, "Rts2DeviceDb::init Cannot connect to DB: %s", sqlca.sqlerrm.sqlerrmc); 
    return -1;
  }

  return 0;
}
