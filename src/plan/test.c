#include "db.h"
#include "selector.h"

#include <libnova.h>
#include <stdio.h>

int
main (int argc, char **argv)
{
  struct target *plan, *last;
  db_connect ();
  make_plan (&plan);
  for (last = plan; last; last = last->next)
    {
      double jd = get_julian_from_timet (&last->ctime);
      struct ln_equ_posn object;
      struct ln_lnlat_posn observer;
      struct ln_hrz_posn hrz;
      object.ra = last->ra;
      object.dec = last->dec;
      observer.lat = 50;
      observer.lng = -15;
      get_hrz_from_equ (&object, &observer, jd, &hrz);
      printf ("%i\t%f\t%f\t%s\t%f\t%f\n", last->id, last->ra, last->dec,
	      ctime (&last->ctime), hrz.alt, hrz.az);
    }
  free_plan (plan);
  db_disconnect ();
}
