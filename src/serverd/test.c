#include <libnova.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include "riseset.h"
#include "../utils/config.h"

#include <mcheck.h>

void
print_jd (double JD, struct ln_lnlat_posn *obs)
{
  struct ln_date date;
  struct ln_equ_posn posn;
  struct ln_hrz_posn hrz;

  get_equ_solar_coords (JD, &posn);
  get_hrz_from_equ (&posn, obs, JD, &hrz);

  get_date (JD, &date);
  printf ("%i/%i/%i %i:%i:%02.3f %02.2f", date.years, date.months, date.days,
	  date.hours, date.minutes, date.seconds, hrz.alt);
}

void
print_rst (struct ln_rst_time *rst, struct ln_lnlat_posn *obs)
{
  print_jd (rst->rise, obs);
  printf ("|");
  print_jd (rst->transit, obs);
  printf ("|");
  print_jd (rst->set, obs);
}

int
rise_set_cal ()
{
  struct ln_lnlat_posn obs;
  double JD = get_julian_from_sys ();

  int i;
  struct ln_rst_time rst;

  obs.lat = get_double_default ("latitude", 0);
  obs.lng = get_double_default ("longtitude", 0);

  printf ("Calculating for longtitude %+3.2f, latitude %2.2f\n", obs.lng,
	  obs.lat);

  for (i = 0; i < 5; i++, JD += 0.5)
    {
      printf ("\nJD: ");
      print_jd (JD, &obs);

      get_solar_rst (JD, &obs, &rst);
      printf ("\nset/rise: ");
      print_rst (&rst, &obs);

      get_solar_rst_horizont (JD, &obs, CIVIL_HORIZONT, &rst);
      printf ("\ncivil %02f: ", CIVIL_HORIZONT);
      print_rst (&rst, &obs);

      get_solar_rst_horizont (JD, &obs, NAUTIC_HORIZONT, &rst);
      printf ("\nnautic %02f: ", NAUTIC_HORIZONT);
      print_rst (&rst, &obs);

      get_solar_rst_horizont (JD, &obs, ASTRONOMICAL_HORIZONT, &rst);
      printf ("\nastronomical %02f: ", ASTRONOMICAL_HORIZONT);
      print_rst (&rst, &obs);
    }

  return 0;
}

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

  read_config (CONFIG_FILE);

  obs.lat = get_double_default ("latitude", 0);
  obs.lng = get_double_default ("longtitude", 0);

  rise_set_cal ();

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
