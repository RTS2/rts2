#include <curses.h>
#include <unistd.h>
#include <stdio.h>
#include <mcheck.h>
#include <getopt.h>
#include <stdlib.h>
#include <signal.h>

#include "telescope_info.h"
#include "camera_info.h"

#include "../utils/devcli.h"
#include "../utils/devconn.h"
#include "../utils/hms.h"
#include "status.h"

int exit_c = 0;

void
status_telescope (WINDOW * wnd, const struct telescope_info *info)
{
  char buf[10];
  dtohms (info->ra / 15, buf);
  mvwprintw (wnd, 1, 1, " Ra: %8s", buf);
  mvwprintw (wnd, 2, 1, "Dec: %+03.3f", info->dec);
  mvwprintw (wnd, 3, 1, "Lon: %+03.3f", info->longtitude);
  mvwprintw (wnd, 4, 1, "Lat: %+03.3f", info->latitude);
  dtohms (info->siderealtime, buf);
  mvwprintw (wnd, 5, 1, "Stm: %6s", buf);
}

void
status_camera (WINDOW * wnd, const struct camera_info *info)
{
  mvwprintw (wnd, 1, 1, "Typ: %s", info->name);
  mvwprintw (wnd, 2, 1, "Ser: %s", info->serial_number);
  mvwprintw (wnd, 3, 1, "Siz: [%ix%i]", info->chip_info[0].width,
	     info->chip_info[0].height);
  mvwprintw (wnd, 4, 1, "Set: %+03.3f oC", info->temperature_setpoint);
  mvwprintw (wnd, 5, 1, "Air: %+03.3f oC", info->air_temperature);
  mvwprintw (wnd, 6, 1, "CCD: %+03.3f oC", info->ccd_temperature);
  mvwprintw (wnd, 7, 1, "CPo: %03.1f %%", info->cooling_power / 10.0);
  mvwprintw (wnd, 8, 1, "Fan: %s", info->fan ? "on" : "off");
}

void
status (WINDOW * wnd, struct device *dev)
{
  int ret;
  devcli_command (dev, &ret, "ready");
  devcli_command (dev, &ret, "info");
  mvwprintw (wnd, 0, 1, "==== Name: %s ======", dev->name);
  if (ret)
    {
      wclear (wnd);
      mvwprintw (wnd, 3, 1, "NOT READY");
    }
  else
    {
      switch (dev->type)
	{
	case DEVICE_TYPE_MOUNT:
	  status_telescope (wnd, (struct telescope_info *) (&dev->info));
	  break;
	case DEVICE_TYPE_CCD:
	  devcli_command (dev, NULL, "chipinfo 0");
	  status_camera (wnd, (struct camera_info *) &dev->info);
	  break;
	default:
	  mvwprintw (wnd, 3, 1, "UNKNOW TYPE: %i", dev->type);
	}
    }
  wrefresh (wnd);
}

void
ex_func (void)
{
  printf ("exit_c: %i", exit_c);
  if (!exit_c)
    endwin ();
  exit_c++;
}

int
main (int argc, char **argv)
{
  WINDOW *wnd[4];
  uint16_t port = SERVERD_PORT;
  char *server;
  char c;
  int ret_code;

  mtrace ();

  /* get attrs */
  while (1)
    {
      static struct option long_option[] = {
	{"port", 1, 0, 'p'},
	{"help", 0, 0, 'h'},
	{0, 0, 0, 0}
      };
      c = getopt_long (argc, argv, "p:h", long_option, NULL);

      if (c == -1)
	break;

      switch (c)
	{
	case 'p':
	  port = atoi (optarg);
	  if (port < 1 || port == UINT_MAX)
	    {
	      printf ("inval camd_id port option: %s\n", optarg);
	      exit (EXIT_FAILURE);
	    }
	  break;
	case 'h':
	  printf ("Options:\n\tport|p <port_num>\t\tport of the server");
	  exit (EXIT_SUCCESS);
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

  printf ("connecting to %s:%i\n", server, port);

  /* connect to the server */
  if (devcli_server_login (server, port, "petr", "petr") < 0)
    {
      perror ("devcli_server_login");
      exit (EXIT_FAILURE);
    }

  if (!initscr ())
    {
      printf ("ncurses not supported!");
      exit (EXIT_FAILURE);
    }

  start_color ();

/*  signal (SIGQUIT, sig_exit);
  signal (SIGINT, sig_exit);
  signal (SIGTERM, sig_exit); */

  atexit (ex_func);

  // prepare windows
  wnd[0] = newwin (LINES / 2, COLS / 2, 0, 0);
  wnd[1] = newwin (LINES / 2, COLS / 2, LINES / 2, 0);
  wnd[2] = newwin (LINES / 2, COLS / 2, 0, COLS / 2);
  wnd[3] = newwin (LINES / 2, COLS / 2, LINES / 2, COLS / 2);

  if (!wnd[3])
    {
      printf ("ncurser not supported\n");
      exit (EXIT_FAILURE);
    }

  printf ("ncurses end\n");

  while (1)
    {
      int i;
      struct device *dev;

      for (i = 0, dev = devcli_devices (); dev && i < 3; dev = dev->next, i++)
	{
	  status (wnd[i], dev);
	}

      for (c = 10; c; c--)
	{
	  //wclear (wnd[3]);
	  mvwprintw (wnd[3], 1, 1, "next update in %02i seconds", c);
	  wrefresh (wnd[3]);
	  sleep (1);
	}
    }
}
