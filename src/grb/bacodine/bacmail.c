#include <stdio.h>

#include "../../db/db.h"

int
main (int argc, char **argv)
{
  int grb_id;
  int grb_seqn;
  double grb_ra;
  double grb_dec;
  time_t grb_date;
  char *grb_name;
  char line_buf[200];
  int tar_id;
  int ret;
  db_connect ();
  while (fgets (line_buf, 200, stdin))
    {
      ret =
	sscanf (line_buf, "%i %i %lf %lf %li %as", &grb_id, &grb_seqn,
		&grb_ra, &grb_dec, &grb_date, &grb_name);
      printf ("ret %i", ret);
      fflush (stdout);
      if (ret == 6)
	{
	  db_update_grb (grb_id, &grb_seqn, &grb_ra, &grb_dec, &grb_date,
			 &tar_id);
	  printf ("tar_id: %i\n", tar_id);
	}
    }
  db_disconnect ();
  return 0;
}
