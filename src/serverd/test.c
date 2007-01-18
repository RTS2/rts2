#include <getopt.h>
#include <libnova/libnova.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include "riseset.h"
#include "../utils/rts2config.h"

void
usage ()
{
  printf ("Observing state display tool.\n"
	  "This program comes with ABSOLUTELY NO WARRANTY; for details\n"
	  "see http://www.gnu.org.  This is free software, and you are welcome\n"
	  "to redistribute it under certain conditions; see http://www.gnu.org\n"
	  "for them.\n"
	  "Program options:\n"
	  "    -a set latitude (overwrites config file). \n"
	  "    -n number of days to calculate summary. \n"
	  "    -t set time (int unix time)\n"
	  "    -l set longtitude (overwrites config file). Positive for east of Greenwich - acording to IAU)\n"
	  "    -h prints this help\n");
  exit (EXIT_SUCCESS);

}

void
print_jd (double JD, struct ln_lnlat_posn *obs)
{
  struct ln_date date;
  struct ln_equ_posn posn;
  struct ln_hrz_posn hrz;

  ln_get_solar_equ_coords (JD, &posn);
  ln_get_hrz_from_equ (&posn, obs, JD, &hrz);

  ln_get_date (JD, &date);
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
rise_set_cal (struct ln_lnlat_posn *obs, time_t * start_time, int ndays)
{
  double JD = ln_get_julian_from_timet (start_time);

  int i;
  struct ln_rst_time rst;

  printf ("Calculating for longtitude %+3.2f, latitude %2.2f\n", obs->lng,
	  obs->lat);

  for (i = 0; i < ndays * 2; i++, JD += 0.5)
    {
      printf ("\nJD: ");
      print_jd (JD, obs);

      ln_get_solar_rst (JD, obs, &rst);
      printf ("\nset/rise: ");
      print_rst (&rst, obs);

      ln_get_solar_rst_horizon (JD, obs, LN_SOLAR_CIVIL_HORIZON, &rst);
      printf ("\ncivil %02f: ", LN_SOLAR_CIVIL_HORIZON);
      print_rst (&rst, obs);

      ln_get_solar_rst_horizon (JD, obs, LN_SOLAR_NAUTIC_HORIZON, &rst);
      printf ("\nnautic %02f: ", LN_SOLAR_NAUTIC_HORIZON);
      print_rst (&rst, obs);

      ln_get_solar_rst_horizon (JD, obs, LN_SOLAR_ASTRONOMICAL_HORIZON, &rst);
      printf ("\nastronomical %02f: ", LN_SOLAR_ASTRONOMICAL_HORIZON);
      print_rst (&rst, obs);
    }

  return 0;
}

int
main (int argc, char **argv)
{
  time_t start_time;
  time_t end_time;
  time_t ev_time;
  time_t last_event = 0;
  struct ln_lnlat_posn *obs;
  int type, curr_type;
  int ndays = 5;
  int c;

  Rts2Config *config = Rts2Config::instance ();
  config->loadFile ();

  obs = config->getObserver ();
  ev_time = time (NULL);

  while (1)
    {
      c = getopt (argc, argv, "a:n:l:ht:");
      if (c == -1)
	break;
      switch (c)
	{
	case 'a':
	  obs->lat = atof (optarg);
	  break;
	case 'l':
	  obs->lng = atof (optarg);
	  break;
	case 'n':
	  ndays = atoi (optarg);
	  if (!ndays)
	    {
	      fprintf (stderr, "Invalid number of days: %i\n", ndays);
	      exit (1);

	    }
	  break;
	case 't':
	  ev_time = atoi (optarg);
	  if (!ev_time)
	    {
	      fprintf (stderr, "Invalid time: %s\n", optarg);
	      exit (1);
	    }
	  break;
	case '?':
	case 'h':
	  usage ();
	default:
	  fprintf (stderr, "Getopt returned unknow character %o\n", c);
	}
    }

  rise_set_cal (obs, &ev_time, ndays);

  start_time = ev_time;

  end_time = start_time + 86500 * ndays;
  for (; start_time < end_time; start_time += 60)
    {
      if (next_event (obs, &start_time, &curr_type, &type, &ev_time))
	{
	  printf ("error!!\n");
	  return EXIT_FAILURE;
	}

      if (last_event != ev_time)
	{

	  printf ("\nt: %li time: %s\n\n\n", ev_time, ctime (&start_time));

	  printf ("next_event: %i ev_time: %s", type, ctime (&ev_time));
	}
      last_event = ev_time;
    }
  printf ("done\n\nOK\n");
  return 0;
}
