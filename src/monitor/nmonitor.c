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
#include "../status.h"

int exit_c = 0;

void
status_telescope (WINDOW * wnd, const struct telescope_info *info)
{
  char buf[10];
  if (info)
    {
      dtohms (info->ra / 15, buf);
      mvwprintw (wnd, 1, 1, " Ra: %8s", buf);
      mvwprintw (wnd, 2, 1, "Dec: %+03.3f", info->dec);
      mvwprintw (wnd, 3, 1, "Lon: %+03.3f", info->longtitude);
      mvwprintw (wnd, 4, 1, "Lat: %+03.3f", info->latitude);
      dtohms (info->siderealtime, buf);
      mvwprintw (wnd, 5, 1, "Stm: %6s", buf);
    }
  else
    {
      wclear (wnd);
      mvwprintw (wnd, 3, 1, "NOT READY");
    }
  wrefresh (wnd);
}

void
status_camera (WINDOW * wnd, const struct camera_info *info)
{
  if (info)
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
  else
    {
      wclear (wnd);
      mvwprintw (wnd, 3, 1, "NOT READY");
    }
  wrefresh (wnd);
}

void
ex_func (void)
{
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
  int camd_id, teld_id;
  int ret_code;
  union devhnd_info *info;

#ifdef DEBUG
  mtrace ();
#endif
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
	      printf ("invalcamd_id port option: %s\n", optarg);
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

  if (devcli_connectdev (&camd_id, "camd", NULL) < 0)
    {
      perror ("devcli_connectdev");
      exit (EXIT_FAILURE);
    }

  printf ("camd_id: %i\n", camd_id);

  if (devcli_connectdev (&teld_id, "teld", NULL) < 0)
    {
      perror ("devcli_connectdev");
      exit (EXIT_FAILURE);
    }

  printf ("teld_id: %i\n---------------\n", teld_id);

  if (!initscr ())
    {
      printf ("ncurses not supported!");
      exit (EXIT_FAILURE);
    }

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

  while (1)
    {
      if (devcli_command (teld_id, &ret_code, "ready"))
	{
	  status_telescope (wnd[0], NULL);
	}
      else
	{
	  devcli_command (teld_id, &ret_code, "info");
	  devcli_getinfo (teld_id, &info);
	  status_telescope (wnd[0], (struct telescope_info *) info);
	}

      if (devcli_command (camd_id, &ret_code, "ready"))
	{
	  status_camera (wnd[1], NULL);
	}
      else
	{
	  devcli_command (camd_id, &ret_code, "info");
	  devcli_command (camd_id, &ret_code, "chipinfo 0");
	  devcli_getinfo (camd_id, &info);
	  status_camera (wnd[1], &(info->camera));
	}

      for (c = 10; c; c--)
	{
	  mvwprintw (wnd[2], 1, 1, "next update in %02i seconds", c);
	  wrefresh (wnd[2]);
	  sleep (1);
	}
    }
}
