#include "../db/db.h"
#include "selector.h"
#include "../utils/config.h"

#include <libnova.h>
#include <stdio.h>

int
main (int argc, char **argv)
{
  struct target *plan, *last;
  char *dp;
  db_connect ();
  read_config (CONFIG_FILE);
  ECPGdebug (1, "log.out");
  db_get_darkfield ("C0", 6000, -50, &dp);
  printf ("dp: %s\n", dp);
  make_plan (&plan, 120, -15, 50);
  for (last = plan; last; last = last->next)
    {
      double jd = get_julian_from_timet (&last->ctime);
      struct ln_equ_posn object;
      struct ln_lnlat_posn observer;
      struct ln_hrz_posn hrz;
      object.ra = last->ra;
      object.dec = last->dec;
      observer.lng = 6.733;
      observer.lat = 37.1;
      get_hrz_from_equ (&object, &observer, jd, &hrz);
      if (last->id > 0)
	printf ("%i\t%f\t%f\t%s\t%f\t%f\n", last->id, last->ra, last->dec,
		ctime (&last->ctime), hrz.alt, hrz.az);
      else
	printf ("%i\t\t\t\t%s** correction %i\n", last->id,
		ctime (&last->ctime), last->type);
    }
  free_plan (plan);

  printf ("last_image C0: %i\n", db_last_good_image ("C0"));
  printf ("last_image C1: %i\n", db_last_good_image ("C1"));

  printf ("last observation 4080: %i\n", db_last_observation (4080));
  printf ("images_night_count 1001: %i\n", db_last_night_images_count (1001));

  return db_disconnect ();
}
