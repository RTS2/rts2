/* 
 * State - display day states for rts2.
 * Copyright (C) 2003 Petr Kubanek <petr@kubanek,net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include "riseset.h"
#include "../utils/config.h"
#include <getopt.h>

int verbose = 0;		// verbosity level

/*!
 * Prints current state on STDERR
 * States are (as described in ../../include/state.h):
 * 
 * 0 - day
 * 1 - evening
 * 2 - dusk
 * 3 - night
 * 4 - dawn
 * 5 - morning
 *
 * Configuration file CONFIG_FILE is used
 */

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
	  "    -c just print current state (one number) and exists\n"
	  "    -t set time (int unix time)"
	  "    -l set longtitude (overwrites config file). Negative for east of Greenwich)\n"
	  "    -h prints that help\n" "    -v be verbose\n");
  exit (EXIT_SUCCESS);

}

int
main (int argc, char **argv)
{
  struct ln_lnlat_posn obs;
  time_t t, ev_time;
  int ret, curr_type, next_type;
  int c;

  if ((ret = read_config (CONFIG_FILE)) == -1)
    {
      fprintf (stderr,
	       "Cannot read configuration file " CONFIG_FILE ", exiting.");
    }

  obs.lat = get_double_default ("latitude", 0);
  obs.lng = get_double_default ("longtitude", 0);

  t = time (NULL);

  while (1)
    {
      c = getopt (argc, argv, "a:cl:hvt:");
      if (c == -1)
	break;
      switch (c)
	{
	case 'a':
	  obs.lat = atof (optarg);
	  break;
	case 'c':
	  verbose = -1;
	  break;
	case 'l':
	  obs.lng = atof (optarg);
	  break;
	case 't':
	  t = atoi (optarg);
	  if (!t)
	    {
	      fprintf (stderr, "Invalid time: %s\n", optarg);
	      exit (1);
	    }
	  break;
	case '?':
	case 'h':
	  usage ();
	case 'v':
	  verbose++;
	  break;
	default:
	  fprintf (stderr, "Getopt returned unknow character %o\n", c);
	}
    }
  if (verbose > 0)
    printf ("Longtitude: %+f Latitude: %f Time: %s", obs.lng, obs.lat,
	    ctime (&t));
  if (next_event (&obs, &t, &curr_type, &next_type, &ev_time))
    {
      fprintf (stderr, "Error getting next type\n");
      exit (EXIT_FAILURE);
    }
  switch (verbose)
    {
    case -1:
      printf ("%i\n", curr_type);
      break;
    default:
      printf ("current: %i next: %i next_time: %s", curr_type, next_type,
	      ctime (&ev_time));
      break;
    }
  return 0;
}
