#include "db.h"
#include <stdio.h>
#include <time.h>
#include <malloc.h>

int
main (int argc, char **argv)
{
  time_t t;
  char *dark_name;
  int tar_id;
  int sq;
  double ra;
  double dec;
  int ret;
  char script[MAX_COMMAND_LENGTH];

  db_connect ();
  time (&t);
  db_get_darkfield ("C1", 6000, 0, &dark_name);
  printf ("end..%s\n", dark_name);
  db_get_darkfield ("C1", 6000, 0, &dark_name);
  printf ("end..%s\n", dark_name);

  sq = 4;
  ra = 11.0;
  dec = 11.0;
  db_update_grb (4, &sq, &ra, &dec, &t, &tar_id);
  printf ("sq: %i ra: %f dec: %f target: %i\n", sq, ra, dec, tar_id);
  free (dark_name);

  ret = db_get_script (180, "C0", script);
  printf ("script C0: %s ret %i\n", script, ret);

  ret = db_get_script (180, "C1", script);
  printf ("script C1: %s ret %i\n", script, ret);

  db_disconnect ();
  return 0;
}
