#include <curses.h>
#include <unistd.h>
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

WINDOW *status_win;

void
print_status (WINDOW * wnd, int start_row, int col, struct device *dev)
{
  int i;
  for (i = 0; i < dev->status_num; i++)
    {
      struct devconn_status *st;
      st = &dev->statutes[i];
      mvwprintw (wnd, start_row + i, col, "status: %s - %i", st->name,
		 st->status);
    }
}

void
status_serverd (WINDOW * wnd, struct device *dev)
{
  mvwprintw (wnd, 1, 1, "status:");
  switch (dev->statutes[0].status)
    {
    case SERVERD_DAY:
      mvwprintw (wnd, 1, 8, "Day");
      break;
    case SERVERD_DUSK:
      mvwprintw (wnd, 1, 8, "Dusk");
      break;
    case SERVERD_NIGHT:
      mvwprintw (wnd, 1, 8, "Night");
      break;
    case SERVERD_DAWN:
      mvwprintw (wnd, 1, 8, "Dawn");
      break;
    case SERVERD_MAINTANCE:
      mvwprintw (wnd, 1, 8, "Maintance");
      break;
    case SERVERD_OFF:
      mvwprintw (wnd, 1, 8, "Off");
      break;
    default:
      mvwprintw (wnd, 1, 8, "Unkow %i", dev->statutes[0].status);
    }
}

void
status_telescope (WINDOW * wnd, struct device *dev)
{
  char buf[10];
  struct telescope_info *info = (struct telescope_info *) &dev->info;
  dtohms (info->ra / 15, buf);
  mvwprintw (wnd, 1, 1, " Ra: %8s", buf);
  mvwprintw (wnd, 2, 1, "Dec: %+03.3f", info->dec);
  mvwprintw (wnd, 3, 1, "Lon: %+03.3f", info->longtitude);
  mvwprintw (wnd, 4, 1, "Lat: %+03.3f", info->latitude);
  dtohms (info->siderealtime, buf);
  print_status (wnd, 5, 1, dev);
}

void
status_camera (WINDOW * wnd, struct device *dev)
{
  struct camera_info *info = (struct camera_info *) &dev->info;
  mvwprintw (wnd, 1, 1, "Typ: %s", info->name);
  mvwprintw (wnd, 2, 1, "Ser: %s", info->serial_number);
  mvwprintw (wnd, 3, 1, "Siz: [%ix%i]", info->chip_info[0].width,
	     info->chip_info[0].height);
  mvwprintw (wnd, 4, 1, "Set: %+03.3f oC", info->temperature_setpoint);
  mvwprintw (wnd, 5, 1, "Air: %+03.3f oC", info->air_temperature);
  mvwprintw (wnd, 6, 1, "CCD: %+03.3f oC", info->ccd_temperature);
  mvwprintw (wnd, 7, 1, "CPo: %03.1f %%", info->cooling_power / 10.0);
  mvwprintw (wnd, 8, 1, "Fan: %s", info->fan ? "on" : "off");
  print_status (wnd, 9, 1, dev);
}

void
status (WINDOW * wnd, struct device *dev)
{
  int ret;
  mvwprintw (status_win, 4, 1, "ready..%s    ", dev->name);
  wrefresh (status_win);
  devcli_command (dev, &ret, "ready");
  mvwprintw (status_win, 4, 1, "info..%s   ", dev->name);
  wrefresh (status_win);
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
	case DEVICE_TYPE_SERVERD:
	  status_serverd (wnd, dev);
	  break;
	case DEVICE_TYPE_MOUNT:
	  status_telescope (wnd, dev);
	  break;
	case DEVICE_TYPE_CCD:
	  devcli_command (dev, NULL, "chipinfo 0");
	  status_camera (wnd, dev);
	  break;
	default:
	  mvwprintw (wnd, 3, 1, "UNKNOW TYPE: %i", dev->type);
	}
    }
  wrefresh (wnd);
}

int
status_change (struct device *dev, char *status_name, int new_state)
{
  mvwprintw (status_win, 3, 1, "Updating....%s %i", status_name, new_state);
  wrefresh (status_win);
  //wprintw (dev->notifier_data, "Updatig...");
  status ((WINDOW *) dev->notifier_data, dev);
  return 0;
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
  WINDOW *wnd[6];
  uint16_t port = SERVERD_PORT;
  char *server;
  char c;
  int i;
  struct device *dev;

#ifdef DEBUG
  mtrace ();
#endif /* DEBUG */

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

  cbreak ();
  noecho ();

  start_color ();

//  curs_set (0);

/*  signal (SIGQUIT, sig_exit);
  signal (SIGINT, sig_exit);
  signal (SIGTERM, sig_exit); */

  atexit (ex_func);

  // prepare windows
  wnd[0] = newwin (LINES / 2, COLS / 3, 0, 0);
  wnd[1] = newwin (LINES / 2, COLS / 3, LINES / 2, 0);
  wnd[2] = newwin (LINES / 2, COLS / 3, 0, COLS / 3);
  wnd[3] = newwin (LINES / 2, COLS / 3, LINES / 2, COLS / 3);
  wnd[4] = newwin (LINES / 2, COLS / 3, 0, 2 * COLS / 3);
  wnd[5] = newwin (LINES / 2, COLS / 3, LINES / 2, 2 * COLS / 3);
  status_win = wnd[5];

  if (!wnd[5])
    {
      printf ("ncurser not supported\n");
      goto end;
    }

  for (i = 0, dev = devcli_devices (); dev && i < 6; dev = dev->next, i++)
    {
      devcli_set_general_notifier (dev, status_change, wnd[i]);
      status (wnd[i], dev);
    }

  while (1)
    {
      int key = wgetch (wnd[5]);
      mvwprintw (wnd[5], 1, 1, "pressed: %i", key);
      switch (key)
	{
	case 32:
	  for (i = 0, dev = devcli_devices (); dev && i < 6;
	       dev = dev->next, i++)
	    status (wnd[i], dev);
	}
    }

end:
  erase ();
  refresh ();

  nocbreak ();
  echo ();
  endwin ();
  return 0;
}
