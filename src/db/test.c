#include "db.h"
#include <stdio.h>
#include <time.h>

int
main (char argc, char **argv)
{
  time_t t;
  char *dark_name;
  int tar_id;
  db_connect ();
  time (&t);
  db_get_darkfield ("C1", 6000, 0, &dark_name);
  printf ("end..%s", dark_name);
  db_update_grb (7, 2, 10.0, 10.0, &t, &tar_id);
  printf ("target: %i\n", tar_id);
  free (dark_name);
  db_disconnect ();
}
