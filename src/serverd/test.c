#include <libnova.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include "riseset.h"

#include <mcheck.h>


int
main ()
{
  time_t start_time = time (NULL);
  time_t end_time;
  time_t ev_time;
  time_t last_event = 0;
  struct ln_lnlat_posn obs;
  int type, curr_type;

  mtrace ();

  obs.lng = -15;
  obs.lat = 50;

  ev_time = 1038460770;

  end_time = start_time + 86500;
  for (; start_time < end_time; start_time += 60)
    {
      if (next_event (&obs, &start_time, &curr_type, &type, &ev_time))
	{
	  printf ("error!!\n");
	  return EXIT_FAILURE;
	}

      if (last_event != ev_time)
	{

	  printf ("t: %li time: %s\n\n\n", ev_time, ctime (&start_time));

	  printf ("next_event: %i ev_time: %s", type, ctime (&ev_time));
	}
      last_event = ev_time;
    }
  printf ("done\n\nOK\n");
  return 0;
}
