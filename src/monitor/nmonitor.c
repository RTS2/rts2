#include <curses.h>
#include <mcheck.h>
#include <libnova.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "telescope_info.h"
#include "camera_info.h"

#include "../utils/devcli.h"
#include "../utils/devconn.h"
#include "../utils/hms.h"
#include "status.h"

WINDOW *status_win;
WINDOW *cmd_win;

int cmd_line, cmd_col;

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
  int i;
  struct client *cli;
  struct serverd_info *info = (struct serverd_info *) &dev->info;
  wclear (wnd);
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
  // print info
  wmove (wnd, 2, 0);
  for (i = 2, cli = info->clients; cli; cli = cli->next, i++)
    {
      mvwprintw (wnd, i, 1, "%3i %c %c %s", cli->priority, cli->active,
		 cli->have_priority, cli->status_txt);
    }
}

void
status_telescope (WINDOW * wnd, struct device *dev)
{
  char buf[10];
  struct ln_equ_posn object;
  struct ln_lnlat_posn observer;
  struct ln_hrz_posn position;

  struct telescope_info *info = (struct telescope_info *) &dev->info;
  double st;

  object.ra = info->ra;
  object.dec = info->dec;
  observer.lat = info->latitude;
  observer.lng = info->longtitude;

  st = info->siderealtime + info->longtitude / 15.0;

  get_hrz_from_equ_siderealtime (&object, &observer, st, &position);

  dtohms (info->ra / 15, buf);

  mvwprintw (wnd, 1, 1, "R/Az: %s %+03.3f", buf, position.az);
  mvwprintw (wnd, 2, 1, "D/Al: %+03.3f %+03.3f", info->dec, position.alt);
  mvwprintw (wnd, 3, 1, "Lon: %+03.3f", info->longtitude);
  mvwprintw (wnd, 4, 1, "Lat: %+03.3f", info->latitude);
  dtohms (info->siderealtime, buf);
  mvwprintw (wnd, 5, 1, "Sid: %s", buf);
  print_status (wnd, 6, 1, dev);
}

void
status_camera (WINDOW * wnd, struct device *dev)
{
  struct camera_info *info = (struct camera_info *) &dev->info;
  mvwprintw (wnd, 1, 1, "Typ: %s", info->type);
  mvwprintw (wnd, 2, 1, "Ser: %s", info->serial_number);
  mvwprintw (wnd, 3, 1, "Siz: [%ix%i]", info->chip_info[0].width,
	     info->chip_info[0].height);
  mvwprintw (wnd, 4, 1, "S/A: %+03.1f %+03.1f oC", info->temperature_setpoint,
	     info->air_temperature);
  mvwprintw (wnd, 5, 1, "CCD: %+03.3f oC", info->ccd_temperature);
  mvwprintw (wnd, 6, 1, "CPo: %03.1f %%", info->cooling_power / 10.0);
  mvwprintw (wnd, 7, 1, "Fan: %s", info->fan ? "on" : "off");
  print_status (wnd, 8, 1, dev);
}

void
status_send (struct device *dev, char *cmd)
{
  char *txt;
  txt = (char *) malloc (5 + strlen (dev->name) + strlen (cmd));
  strcpy (txt, "> ");
  strcat (txt, dev->name);
  strcat (txt, ".");
  strcat (txt, cmd);
  strcat (txt, "\n");
  wattrset (status_win, A_STANDOUT);
  waddstr (status_win, txt);
  wattrset (status_win, A_NORMAL);
  free (txt);
  wrefresh (status_win);
}

void
status_receive (struct device *dev, char *str)
{
  char *txt;
  txt = (char *) malloc (4 + strlen (dev->name) + strlen (str));
  strcpy (txt, dev->name);
  strcat (txt, ": ");
  strcat (txt, str);
  strcat (txt, "\n");
  waddstr (status_win, txt);
  free (txt);
  wrefresh (status_win);
}

void
status (WINDOW * wnd, struct device *dev)
{
  int ret;
  curs_set (0);
  status_send (dev, "ready");
  devcli_command (dev, &ret, "ready");
  status_send (dev, "info");
  devcli_command (dev, &ret, "info");
  wclear (wnd);
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
  wmove (cmd_win, cmd_line, cmd_col);
  wrefresh (cmd_win);
  // callback
  dev->response_handler = status_receive;
  curs_set (1);
}

int
status_change (struct device *dev, char *status_name, int new_state)
{
  status ((WINDOW *) dev->notifier_data, dev);
  return 0;
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
#define BUF_LEN		60
  char buf[BUF_LEN];

  int l;

#ifdef DEBUG
  mtrace ();
#endif /* DEBUG */

  /* get attrs */
  while (1)
    {
      static struct option long_option[] = {
	{"help", 0, 0, 'h'},
	{0, 0, 0, 0}
      };
      c = getopt_long (argc, argv, "p:h", long_option, NULL);

      if (c == -1)
	break;

      switch (c)
	{
	case 'h':
	  printf ("Options:\n\tport|p <port_num>\t\tport of the server");
	  return EXIT_FAILURE;
	default:
	  printf ("?? getopt returned unknow character %o ??\n", c);
	}
    }
  if (optind != argc - 1)
    {
      server = "localhost";
    }
  else
    {
      server = argv[optind++];
    }

  printf ("connecting to %s:%i\n", server, port);

  /* connect to the server */
  if (devcli_server_login (server, port, "petr", "petr") < 0)
    {
      perror ("devcli_server_login");
      exit (EXIT_FAILURE);
    }

  devcli_server_command (NULL, "status_txt nmonitor");

  if (!initscr ())
    {
      printf ("ncurses not supported!");
      exit (EXIT_FAILURE);
    }

  cbreak ();
  noecho ();

  start_color ();

  // prepare windows

  l = LINES - 2;

  cmd_win = newwin (2, COLS, 0, 0);

  keypad (cmd_win, true);
  scrollok (cmd_win, true);
  idlok (cmd_win, true);

  cmd_line = 0;
  cmd_col = 0;

  wnd[0] = newwin (l / 2, COLS / 3, 2, 0);
  wnd[1] = newwin (l / 2, COLS / 3, 2 + l / 2, 0);
  wnd[2] = newwin (l / 2, COLS / 3, 2, COLS / 3);
  wnd[3] = newwin (l / 2, COLS / 3, 2 + l / 2, COLS / 3);
  wnd[4] = newwin (l / 2, COLS / 3, 2, 2 * COLS / 3);
  wnd[5] = newwin (l / 2, COLS / 3, 2 + l / 2, 2 * COLS / 3);
  status_win = wnd[5];
  box (status_win, 0, 0);
  scrollok (status_win, true);

  if (!wnd[5])
    {
      printf ("ncurser not supported\n");
      goto end;
    }

  devcli_set_general_notifier (devcli_server (), status_change, wnd[4]);
  status (wnd[4], devcli_server ());

  for (i = 0, dev = devcli_devices (); dev && i < 5; dev = dev->next, i++)
    {
      devcli_set_general_notifier (dev, status_change, wnd[i]);
      status (wnd[i], dev);
    }

  while (1)
    {
      int key = wgetch (cmd_win);
      switch (key)
	{
	case KEY_F (5):
	  status (wnd[4], devcli_server ());
	  for (i = 0, dev = devcli_devices (); dev && i < 6;
	       dev = dev->next, i++)
	    status (wnd[i], dev);
	  break;
	case KEY_F (10):
	  goto end;
	  break;
	case KEY_BACKSPACE:
	  if (cmd_col > 0)
	    {
	      cmd_col--;
	      wmove (cmd_win, cmd_line, cmd_col);
	      wdelch (cmd_win);
	      wrefresh (cmd_win);
	    }
	  else
	    {
	      beep ();
	    }
	  break;
	case KEY_ENTER:
	case '\n':
	  beep ();
	  wclear (cmd_win);
	  buf[cmd_col] = 0;
	  mvwprintw (cmd_win, 1, 1, "executing: %s", buf);
	  wrefresh (cmd_win);
	  if (devcli_execute (buf, &i))
	    {
	      wprintw (cmd_win, " error");
	    }
	  else
	    {
	      wprintw (cmd_win, " return code: %i", i);
	    }
	  cmd_col = 0;
	  wmove (cmd_win, 0, 0);
	  wrefresh (cmd_win);
	  break;
	default:
	  if (cmd_line >= BUF_LEN)
	    {
	      beep ();
	    }
	  else
	    {
	      buf[cmd_col++] = key;
	      wechochar (cmd_win, key);
	    }
	}
    }

end:
  erase ();
  refresh ();

  devcli_server_disconnect ();

  nocbreak ();
  echo ();
  endwin ();
  return 0;
}
