#include "db.h"
#include <stdio.h>
#include <time.h>

int
main (char argc, char **argv)
{
  time_t t;
  char *dark_name;
  int tar_id;
  int sq;
  double ra;
  double dec;
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
  db_disconnect ();
}
