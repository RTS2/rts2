/*!
 * Pohotometer list in VYPAR (Harmanec and al.) B format
 */

#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <time.h>
#include <stdlib.h>
#include <getopt.h>
#include <libnova/libnova.h>

#include "libpq-fe.h"
#include "../utils/config.h"

PGconn *conn;

static void
exit_nicely ()
{
  PQfinish (conn);
  exit (1);
}

struct ln_lnlat_posn observer;

int night_day = 0;
int night_month = 0;
int night_year = 0;
int target = 0;
int target_info = 0;

#define PQ_EXEC(command) \
      res = PQexec(conn, command); \
      if (PQresultStatus(res) != PGRES_COMMAND_OK) \
	{ \
                fprintf(stderr, "%s command failed: %s", command, PQerrorMessage(conn)); \
                PQclear(res); \
		return; \
	}

void
print_head (struct tm *date)
{
  printf
    ("FRAM photometry\nPierre-Auger observatory, Argentina, South America\n");
  printf ("%3i%3i%5i 426 0\n", date->tm_mday + 1, date->tm_mon + 1,
	  date->tm_year + 1900);
  printf ("  10.000 20.300 120\n");
}

void
print_target_info (int tar_id, time_t * t)
{
  char command[400] =
    "SELECT tar_ra, tar_dec, phot_mag1, phot_mag2, phot_index1, phot_index2 FROM phot, targets WHERE targets.tar_id = phot.tar_id  AND targets.tar_id = ";
  struct ln_equ_posn object;
  struct ln_hrz_posn hrz;
  int i;

  sprintf (command, "%s %i;", command, tar_id);
  PGresult *res;

  double JD;

  JD = ln_get_julian_from_timet (t);

  res = PQexecParams (conn, command, 0, NULL, NULL, NULL, NULL, 0);
  if (PQresultStatus (res) != PGRES_TUPLES_OK)
    {
      fprintf (stderr, "SELECT failed: %s", PQerrorMessage (conn));
      PQclear (res);
      return;
    }
  /* next, print out the rows */
  if (PQntuples (res) != 1)
    {
      printf ("8%4i invalid\n", tar_id);
      return;
    }
  printf ("7%4i", tar_id);
  object.ra = atof (PQgetvalue (res, 0, 0));
  object.dec = atof (PQgetvalue (res, 0, 1));
  ln_get_hrz_from_equ (&object, &observer, JD, &hrz);
  printf (" %.4f %.4f", hrz.alt, hrz.az);
  for (i = 2; i < PQnfields (res); i++)
    {
      printf (" %s", PQgetvalue (res, 0, i));
    }
  printf ("\n");
  PQclear (res);
}

void
get_list (int tar_id)
{
  int nFields;
  int i;
  int obs_id, last_obs_id = -1;

  time_t count_date;
  struct tm date;
  uint64_t count_value;
  char count_filter[3];
  char *command;
  char sel[2000] =
    "SELECT "
    "  tar_id,"
    "  MIN(count_date) as cd,"
    "  int8(AVG(count_value / count_exposure)),"
    "  count_filter," "  obs_id " "FROM" "  targets_counts ";
  PGresult *res;
// d d y b v u
  int filters[] = { 0, 0, 4, 2, 1, 3 };
  time_t next_midnight = 0;

  command = "BEGIN";

  PQ_EXEC (command);
  PQclear (res);

  if (target)
    {
      sprintf (sel, "%s WHERE tar_id = %i ", sel, target);
    }

  strcat (sel,
	  "GROUP BY"
	  "  tar_id, count_filter, obs_id " "ORDER BY" "  cd ASC;");

  res = PQexecParams (conn, sel, 0, NULL, NULL, NULL, NULL, 1);
  if (PQresultStatus (res) != PGRES_TUPLES_OK)
    {
      fprintf (stderr, "SELECT failed: %s", PQerrorMessage (conn));
      PQclear (res);
      return;
    }
  nFields = PQnfields (res);
  /* next, print out the rows */
  for (i = 0; i < PQntuples (res); i++)
    {
      tar_id = ntohl (*((uint32_t *) PQgetvalue (res, i, 0)));
      obs_id = ntohl (*((uint32_t *) PQgetvalue (res, i, 4)));
      count_date = ntohl (*((uint32_t *) (PQgetvalue (res, i, 1))));
      gmtime_r (&count_date, &date);
      if (target_info && obs_id != last_obs_id)
	{
	  print_target_info (tar_id, &count_date);
	  last_obs_id = obs_id;
	}
      count_value = ntohl (*((uint32_t *) PQgetvalue (res, i, 2)));
      count_value <<= 32;
      count_value |= ntohl (*(((uint32_t *) PQgetvalue (res, i, 2)) + 1));
      strncpy (count_filter, PQgetvalue (res, i, 3), 3);
      if (next_midnight < count_date)
	{
	  print_head (&date);
	  next_midnight = count_date + abs ((date.tm_hour - 4) % 24) * 3600;
	}
      printf ("%1i%4i%3i%3i%4i.0000%10lli\n", filters[count_filter[0] - 'A'],
	      tar_id, date.tm_hour, date.tm_min, date.tm_sec, count_value);
    }

  PQclear (res);
  command = "END";
  PQ_EXEC (command);
  PQclear (res);
  return;
}

int
main (int argc, char **argv)
{
  const char *conninfo;
  int c;

  conninfo = "dbname = stars";

  conn = PQconnectdb (conninfo);

  if (PQstatus (conn) != CONNECTION_OK)
    {
      fprintf (stderr, "Connection to database '%s' failed.\n", PQdb (conn));
      fprintf (stderr, "%s", PQerrorMessage (conn));
      exit_nicely ();
    }
  while (1)
    {
      static struct option long_option[] = {
	{"start_day", 1, 0, 'd'},
	{"start_month", 1, 0, 'm'},
	{"start_year", 1, 0, 'y'},
	{"target", 1, 0, 't'},
	{"target_info", 0, 0, 'i'},
	{0, 0, 0, 0}
      };
      c = getopt_long (argc, argv, "d:m:y:t:i", long_option, NULL);
      if (c == -1)
	break;
      switch (c)
	{
	case 'd':
	  night_day = atoi (optarg);
	  break;
	case 'm':
	  night_month = atoi (optarg);
	  break;
	case 'y':
	  night_year = atoi (optarg);
	  break;
	case 't':
	  target = atoi (optarg);
	  break;
	case 'i':
	  target_info = -1;
	  break;
	case '?':
	  break;
	default:
	  printf ("?? getopt ret: %o\n", c);
	}
    }

  printf ("Readign config file" CONFIG_FILE "...");
  if (read_config (CONFIG_FILE) == -1)
    printf ("failed, defaults will be used when apopriate.\n");
  else
    printf ("ok\n");
  observer.lng = get_double_default ("longtitude", 0);
  observer.lat = get_double_default ("latitude", 0);
  printf ("lng: %f lat: %f\n", observer.lng, observer.lat);

  get_list (120);

  /* close the connection to the database and cleanup */
  PQfinish (conn);

  return 0;

}
