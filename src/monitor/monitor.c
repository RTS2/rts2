#define _GNU_SOURCE

#include "telescope_info.h"
#include "camera_info.h"
#include "dome_info.h"

#include "../utils/devconn.h"
#include "../utils/devcli.h"
#include "status.h"
#include "config.h"

#include <errno.h>
#include <getopt.h>
#include <mcheck.h>
#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <libnova.h>

int line_count = 0;

FILE *out;

void
eprintline ()
{
  if (line_count % 2)
    fprintf (out, "\n");
  line_count = 0;
}

void
efprintf (char *name, char *format, ...)
{
  int x;
  va_list vl;
  va_start (vl, format);
  fprintf (out, "%15s : ", name);
  x = vfprintf (out, format, vl);
  if (line_count % 2)
    fprintf (out, "\n");
  else
    for (; x < 15; x++)
      fprintf (out, " ");
  line_count++;
  va_end (vl);
}

void
status_serverd (const struct device *dev)
{
  struct client *cli;
  struct serverd_info *info = (struct serverd_info *) &dev->info;
  if (line_count % 2)
    fprintf (out, "\n");
  line_count = 0;
  for (cli = info->clients; cli; cli = cli->next)
    {
      eprintline ();
      efprintf ("Client", "%3i%c%s", cli->priority, cli->have_priority,
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
  efprintf ("longtitude", "%+03.3f", info->longtitude);
  efprintf ("latitude", "%+03.3f", info->latitude);
  efprintf ("ra", "%.3f/%c", info->ra, info->flip ? 'f' : 'n');
  efprintf ("dec", "%+.3f", info->dec);
  object.ra = info->ra;
  object.dec = info->dec;
  observer.lng = info->longtitude;
  observer.lat = info->latitude;
  get_hrz_from_equ_sidereal_time (&object, &observer, st, &position);
  efprintf ("az", "%i (%s)", (int) position.az, hrz_to_nswe (&position));
  efprintf ("alt", "%i", (int) position.alt);
  efprintf ("axis count 0", "%f", info->axis0_counts);
  efprintf ("axis count 1", "%f", info->axis1_counts);
  efprintf ("siderealtime", "%f", info->siderealtime);
  efprintf ("lst", "%f", st);
}

void
status_camera (const struct device *dev)
{
  struct camera_info *info = (struct camera_info *) &dev->info;
  efprintf ("type", "%s", info->type);
  efprintf ("SN", "%s", info->serial_number);
  efprintf ("set temp.", "%+03.3f oC", info->temperature_setpoint);
  efprintf ("air temp.", "%+03.3f oC", info->air_temperature);
  efprintf ("CCD temp.", "%+03.3f", info->ccd_temperature);
  efprintf ("cool pwr", "%03.1f %%", info->cooling_power / 10.0);
  efprintf ("fan", info->fan ? "on" : "off");
}

void
status_dome (struct device *dev)
{
  struct dome_info *info = (struct dome_info *) &dev->info;
  efprintf ("type", "%s", info->type);
  eprintline ();
  efprintf ("temperature", "%+.2f oC", info->temperature);
  efprintf ("humidity", "%.2f %%", info->humidity);
}

void
print_device (struct device *dev)
{
  int ret, ret_code, i;
  line_count = 0;
  if ((ret = devcli_command (dev, &ret_code, "ready")))
    efprintf ("ready", "%i", ret);
  efprintf ("ready ret_c", "%i", ret_code);
  eprintline ();
  if ((ret = devcli_command (dev, &ret_code, "base_info")))
    efprintf ("base_info", "%i", ret);
  efprintf ("base_info ret_c", "%i", ret_code);
  eprintline ();
  if ((ret = devcli_command (dev, &ret_code, "info")))
    efprintf ("info", "%i", ret);
  efprintf ("info ret_c", "%i", ret_code);
  eprintline ();
  if (*dev->name)
    {
      efprintf ("device name", "%s", dev->name);
      eprintline ();
      efprintf ("hostname", "%s", dev->hostname);
      efprintf ("port", "%i", dev->port);
      eprintline ();
    }
  switch (dev->type)
    {
    case DEVICE_TYPE_MOUNT:
      efprintf ("type", "%i (MOUNT)", dev->type);
      eprintline ();
      if (ret_code == 0)
	status_telescope (dev);
      break;
    case DEVICE_TYPE_CCD:
      efprintf ("type", "%i (CCD)", dev->type);
      eprintline ();
      if (ret_code == 0)
	status_camera (dev);
      break;
    case DEVICE_TYPE_DOME:
      efprintf ("type", "%i (DOME)", dev->type);
      eprintline ();
      if (ret_code == 0)
	status_dome (dev);
      break;
    case DEVICE_TYPE_SERVERD:
      efprintf ("type", "%i (Central server)", dev->type);
      eprintline ();
      status_serverd (dev);
      break;
    default:
      fprintf (out, "unknow device type: %i\n", dev->type);
    }
  if (ret_code != 0)
    {
      eprintline ();
      fprintf (out, "\n      NOT CONNECTED / STATUS UNKNOW!!!\n");
    }
  line_count = 1;
  fprintf (out, "\n");
  efprintf ("states #", "%i", dev->status_num);
  line_count = 0;
  for (i = 0; i < dev->status_num; i++)
    {
      struct devconn_status *st = &dev->statutes[i];
      if (*st->name)
	{
	  efprintf ("status", "%s", st->name);
	  efprintf ("value", "%s (%i)", devcli_status_string (dev, st),
		    st->status);
	}
    }
  fprintf (out, "\n Device %s status end.\n", dev->name);
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
  char *mail_to = NULL;

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
	{"subject", 1, 0, 's'},
	{0, 0, 0, 0}
      };
      c = getopt_long (argc, argv, "m:hp:s:", long_option, NULL);

      if (c == -1)
	break;

      switch (c)
	{
	case 'h':
	  fprintf (out, "Output status report (with optional formating)\n"
		   " Invocation: \n"
		   "\t%s [options] <centrald_host>\n"
		   " Options:\n"
		   "\t-h|--help			print that help message\n"
		   "\t-m|--mail <addresses>	pass output to mail with adresses\n"
		   "\t-p|--port <port_num>	port of the server\n"
		   "\t-s|--subject <subject>	mail subject\n"
		   " Part of rts2 package.\n", argv[0]);
	  exit (EXIT_SUCCESS);
	case 'm':
	  mail_to = optarg;
	  break;
	case 'p':
	  port = atoi (optarg);
	  break;
	case 's':
	  subject = optarg;
	  break;
	case '?':
	  break;
	default:
	  fprintf (out, "?? getopt returned unknow character %o ??\n", c);
	}
    }
  if (optind != argc - 1)
    {
      fprintf (out, "You must pass server address\n");
      exit (EXIT_FAILURE);
    }
  server = argv[optind++];

  time (&t);

  c = devcli_server_login (server, port, "petr", "petr");

  if (mail_to)
    {
      int failed_found = 0;	// become 1 when we founded fail device and print
      // warning prefix
      int ret;
      char mail_command[500];
      strcpy (mail_command, "mail -s \"");
      strcat (mail_command, subject);
      if (c == -1)
	{
	  strcat (mail_command,
		  " FATAL ERROR - cannot connect to centrald!\n.\n");
	}
      else
	{
	  for (dev = devcli_devices (); dev; dev = dev->next)
	    {
	      if (devcli_command (dev, &ret, "ready") == -1)
		{
		  char ret_code[20];
		  if (!failed_found)
		    {
		      failed_found = 1;
		      strcat (mail_command, " WARNING - FAILED DEVICE(s):");
		    }
		  strcat (mail_command, " ");
		  strcat (mail_command, dev->name);
		  sprintf (ret_code, "(%d)", ret);
		  strcat (mail_command, ret_code);
		}
	    }
	  if (!failed_found)
	    strcat (mail_command, " ALL OK");
	}
      strcat (mail_command, " at ");
      strcat (mail_command, ctime (&t));
      mail_command[strlen (mail_command) - 1] = 0;
      strcat (mail_command, "\" \"");
      strcat (mail_command, mail_to);
      strcat (mail_command, "\"");
      out = popen (mail_command, "w");
      if (!out)
	{
	  fprintf (stderr, "Error opening pipe to mail %i %s!\n", errno,
		   strerror (errno));
	  exit (EXIT_FAILURE);
	}
      fprintf (out, "Full status report follows\n");
    }

  fprintf (out, " Bootes status report generated at: %s", ctime (&t));
  fprintf (out, " RTS2 " VERSION);
  fprintf (out, " Connecting to %s:%i\n", server, port);
  if (c == -1)
    {
      fprintf (out, "Connection to centrald failed!\n");
      exit (EXIT_FAILURE);
    }

  fprintf
    (out,
     "=================================================================\n");

  print_device (devcli_server ());

  for (dev = devcli_devices (); dev; dev = dev->next)
    {
      fprintf
	(out,
	 "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
      print_device (dev);
    }

  time (&t);
  fprintf
    (out,
     "==================================================================\n");
  fprintf (out, " Status report ends at: %s", ctime (&t));
  return 0;
}
