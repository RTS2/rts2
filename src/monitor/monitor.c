#define _GNU_SOURCE

#include "telescope_info.h"
#include "camera_info.h"
#include "dome_info.h"

#include "../utils/devconn.h"
#include "../utils/devcli.h"
#include "status.h"
#include "config.h"

#include <unistd.h>
#include <stdio.h>
#include <mcheck.h>
#include <getopt.h>
#include <stdlib.h>
#include <signal.h>
#include <stdarg.h>
#include <time.h>

#include <libnova.h>

int line_count = 0;

void
eprintline ()
{
  if (line_count % 2)
    printf ("\n");
  line_count = 0;
}

void
eprintf (char *name, char *format, ...)
{
  int x;
  va_list vl;
  va_start (vl, format);
  printf ("%15s : ", name);
  x = vprintf (format, vl);
  if (line_count % 2)
    printf ("\n");
  else
    for (; x < 15; x++)
      printf (" ");
  line_count++;
  va_end (vl);
}

void
status_serverd (const struct device *dev)
{
  struct client *cli;
  struct serverd_info *info = (struct serverd_info *) &dev->info;
  if (line_count % 2)
    printf ("\n");
  line_count = 0;
  for (cli = info->clients; cli; cli = cli->next)
    {
      eprintline ();
      eprintf ("Client", "%3i%c%s", cli->priority, cli->have_priority,
	       cli->status_txt);
    }
}

void
status_telescope (const struct device *dev)
{
  struct telescope_info *info = (struct telescope_info *) &dev->info;
  struct ln_equ_posn object;
  struct ln_lnlat_posn observer;
  struct ln_hrz_posn position;
  double st;
  st = info->siderealtime + info->longtitude / 15.0;
  eprintf ("longtitude", "%+03.3f", info->longtitude);
  eprintf ("latitude", "%+03.3f", info->latitude);
  eprintf ("ra", "%.3f/%c", info->ra, info->flip ? 'f' : 'n');
  eprintf ("dec", "%+.3f", info->dec);
  object.ra = info->ra;
  object.dec = info->dec;
  observer.lng = info->longtitude;
  observer.lat = info->latitude;
  get_hrz_from_equ_sidereal_time (&object, &observer, st, &position);
  eprintf ("az", "%i (%s)", (int) position.az, hrz_to_nswe (&position));
  eprintf ("alt", "%i", (int) position.alt);
  eprintf ("axis count 0", "%f", info->axis0_counts);
  eprintf ("axis count 1", "%f", info->axis1_counts);
  eprintf ("siderealtime", "%f", info->siderealtime);
  eprintf ("lst", "%f", st);
}

void
status_camera (const struct device *dev)
{
  struct camera_info *info = (struct camera_info *) &dev->info;
  eprintf ("type", "%s", info->type);
  eprintf ("SN", "%s", info->serial_number);
  eprintf ("set temp.", "%+03.3f oC", info->temperature_setpoint);
  eprintf ("air temp.", "%+03.3f oC", info->air_temperature);
  eprintf ("CCD temp.", "%+03.3f", info->ccd_temperature);
  eprintf ("cool pwr", "%03.1f %%", info->cooling_power / 10.0);
  eprintf ("fan", info->fan ? "on" : "off");
}

void
status_dome (struct device *dev)
{
  struct dome_info *info = (struct dome_info *) &dev->info;
  eprintf ("type", "%s", info->type);
  eprintline ();
  eprintf ("temperature", "%+.2f oC", info->temperature);
  eprintf ("humidity", "%.2f %%", info->humidity);
}

void
print_device (struct device *dev)
{
  int ret, ret_code, i;
  line_count = 0;
  if ((ret = devcli_command (dev, &ret_code, "ready")))
    eprintf ("ready", "%i", ret);
  eprintf ("ready ret_c", "%i", ret_code);
  eprintline ();
  if ((ret = devcli_command (dev, &ret_code, "base_info")))
    eprintf ("base_info", "%i", ret);
  eprintf ("base_info ret_c", "%i", ret_code);
  eprintline ();
  if ((ret = devcli_command (dev, &ret_code, "info")))
    eprintf ("info", "%i", ret);
  eprintf ("info ret_c", "%i", ret_code);
  eprintline ();
  if (*dev->name)
    {
      eprintf ("device name", "%s", dev->name);
      eprintline ();
      eprintf ("hostname", "%s", dev->hostname);
      eprintf ("port", "%i", dev->port);
      eprintline ();
    }
  switch (dev->type)
    {
    case DEVICE_TYPE_MOUNT:
      eprintf ("type", "%i (MOUNT)", dev->type);
      eprintline ();
      if (ret_code == 0)
	status_telescope (dev);
      break;
    case DEVICE_TYPE_CCD:
      eprintf ("type", "%i (CCD)", dev->type);
      eprintline ();
      if (ret_code == 0)
	status_camera (dev);
      break;
    case DEVICE_TYPE_DOME:
      eprintf ("type", "%i (DOME)", dev->type);
      eprintline ();
      if (ret_code == 0)
	status_dome (dev);
      break;
    case DEVICE_TYPE_SERVERD:
      eprintf ("type", "%i (Central server)", dev->type);
      eprintline ();
      status_serverd (dev);
      break;
    default:
      printf ("unknow device type: %i\n", dev->type);
    }
  if (ret_code != 0)
    {
      eprintline ();
      printf ("\n      NOT CONNECTED / STATUS UNKNOW!!!\n");
    }
  line_count = 1;
  printf ("\n");
  eprintf ("states #", "%i", dev->status_num);
  line_count = 0;
  for (i = 0; i < dev->status_num; i++)
    {
      struct devconn_status *st = &dev->statutes[i];
      if (*st->name)
	{
	  eprintf ("status", "%s", st->name);
	  eprintf ("value", "%s (%i)", devcli_status_string (dev, st),
		   st->status);
	}
    }
  printf ("\n Device %s status end.\n", dev->name);
}

int
main (int argc, char **argv)
{
  uint16_t port = SERVERD_PORT;
  char *server;
  int c;
  struct device *dev;
  time_t t;
  char *subject = NULL;

#ifdef DEBUG
  mtrace ();
#endif
  /* get attrs */
  while (1)
    {
      static struct option long_option[] = {
	{"mail", 1, 0, 'm'},
	{"help", 0, 0, 'h'},
	{"port", 1, 0, 'p'},
	{0, 0, 0, 0}
      };
      c = getopt_long (argc, argv, "m:hp:", long_option, NULL);

      if (c == -1)
	break;

      switch (c)
	{
	case 'h':
	  printf ("Output status report (with optional formating)\n"
		  " Invocation: \n"
		  "\t%s [options] <centrald_host>\n"
		  " Options:\n"
		  "\t-h|--help			print that help message\n"
		  "\t-m|--mail <subject_prefix>	print at the begining status line\n"
		  "\t				so we can feed it to mail command\n"
		  "\t-p|--port <port_num>		port of the server\n"
		  " Part of rts2 package.\n", argv[0]);
	  exit (EXIT_SUCCESS);
	case 'm':
	  subject = optarg;
	  break;
	case 'p':
	  port = atoi (optarg);
	  break;
	case '?':
	  break;
	default:
	  printf ("?? getopt returned unknow character %o ??\n", c);
	}
    }
  if (optind != argc - 1)
    {
      printf ("You must pass server address\n");
      exit (EXIT_FAILURE);
    }
  server = argv[optind++];

  time (&t);

  c = devcli_server_login (server, port, "petr", "petr");

  if (subject)
    {
      int failed_found = 0;	// become 1 when we founded fail device and print
      // warning prefix
      int ret;
      printf ("Subject: %s %s", subject, ctime (&t));
      if (c == -1)
	{
	  printf (" FATAL ERROR - cannot connect to centrald!\n.\n");
	  exit (EXIT_FAILURE);
	}
      for (dev = devcli_devices (); dev; dev = dev->next)
	{
	  if (devcli_command (dev, &ret, "ready") == -1)
	    {
	      if (!failed_found)
		{
		  failed_found = 1;
		  printf (" WARNING - FAILED DEVICE(s):");
		}
	      printf (" %s(%i)", dev->name, ret);
	    }
	}
      if (!failed_found)
	printf (" ALL OK");
      printf ("\n Full status report follows\n");
    }

  printf (" Bootes status report generated at: %s", ctime (&t));
  printf (" RTS2 " VERSION);
  printf (" Connecting to %s:%i\n", server, port);
  if (c == -1)
    {
      printf ("Connection to centrald failed!\n");
      exit (EXIT_FAILURE);
    }

  printf
    ("=================================================================\n");

  print_device (devcli_server ());

  for (dev = devcli_devices (); dev; dev = dev->next)
    {
      printf
	("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
      print_device (dev);
    }

  time (&t);
  printf
    ("==================================================================\n");
  printf (" Status report ends at: %s", ctime (&t));
  return 0;
}
