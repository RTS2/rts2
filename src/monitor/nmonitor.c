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

#define ROW_SIZE	10
#define COL_SIZE	25

int wnd_col;
int wnd_row;

int cmd_line, cmd_col;

void
print_status (WINDOW * wnd, int start_row, int col, struct device *dev)
{
  int i;
  for (i = 0; i < dev->status_num; i++)
    {
      struct devconn_status *st;
      st = &dev->statutes[i];

      mvwprintw (wnd, start_row + i, col, "st: %s - %i", st->name,
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
  if (has_colors ())
    wcolor_set (wnd, 1, NULL);
  else
    wstandout (wnd);
  mvwprintw (wnd, 1, 1, "status: ");
  if (dev->statutes[0].status == SERVERD_OFF)
    mvwprintw (wnd, 1, 9, "Off");
  else
    {
      mvwprintw (wnd, 1, 9,
		 dev->statutes[0].
		 status & SERVERD_STANDBY_MASK ? "STD" : "RDY");
      mvwprintw (wnd, 1, 13, serverd_status_string (dev->statutes[0].status));
    }
  if (has_colors ())
    wcolor_set (wnd, 2, NULL);
  else
    wstandend (wnd);
  // print info
  wmove (wnd, 2, 0);
  for (i = 2, cli = info->clients; cli; cli = cli->next, i++)
    {
      if (cli->have_priority == '*')
	{
	  if (has_colors ())
	    wcolor_set (wnd, 3, NULL);
	}
      mvwprintw (wnd, i, 1, "%3i%c%s", cli->priority,
		 cli->have_priority, cli->status_txt);
      if (has_colors ())
	wcolor_set (wnd, 2, NULL);
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

  get_hrz_from_equ_sidereal_time (&object, &observer, st, &position);

  mvwprintw (wnd, 1, 1, "Typ: %s", info->type);
  mvwprintw (wnd, 2, 1, "R+D/f: %.3f%+.3f/%c",
	     info->ra, info->dec, info->flip ? 'f' : 'n');
  mvwprintw (wnd, 3, 1, "Az/Al/D: %i %+i %s",
	     (int) position.az, (int) position.alt, hrz_to_nswe (&position));

  mvwprintw (wnd, 4, 1, "Lon/Lat: %+03.3f %+03.3f", info->longtitude,
	     info->latitude);
  dtohms (info->siderealtime, buf);
  mvwprintw (wnd, 5, 1, "Lsid: %.3f (%s)", 15.0 * info->siderealtime, buf);
  st = st - (int) (st / 24) * 24;
  dtohms (st, buf);
  mvwprintw (wnd, 6, 1, "Gsid: %.3f (%s)", 15.0 * st, buf);
  print_status (wnd, 7, 1, dev);
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
  if (!devcli_command (dev, &ret, "ready"))
    {
      status_send (dev, "info");
      devcli_command (dev, &ret, "base_info");
      devcli_command (dev, &ret, "info");
    }
  wclear (wnd);

  if (has_colors ())
    wcolor_set (wnd, 1, NULL);
  else
    wstandout (wnd);

  mvwprintw (wnd, 0, 1, "==== Name: %s ======", dev->name);
  if (ret)
    {
      if (has_colors ())
	wcolor_set (wnd, 2, NULL);
      else
	wstandend (wnd);
      //    wclear (wnd);
      mvwprintw (wnd, 2, 6, "NOT READY");
    }
  else
    {
      if (has_colors ())
	wcolor_set (wnd, 2, NULL);
      else
	wstandend (wnd);

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
  WINDOW **wnd;
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

  if (has_colors ())
    {
      init_pair (1, 2, 0);
      init_pair (2, 7, 0);
      init_pair (3, COLOR_BLUE, 0);
    }
  // prepare windows

  l = LINES - 2;

  wnd_row = LINES / ROW_SIZE;
  wnd_col = COLS / COL_SIZE;
  wnd = (WINDOW *) malloc (sizeof (WINDOW) * wnd_row * wnd_col);

  cmd_win = newwin (2, COLS, 0, 0);

  keypad (cmd_win, TRUE);
  scrollok (cmd_win, TRUE);
  idlok (cmd_win, TRUE);

  for (cmd_col = 0; cmd_col < wnd_col; cmd_col++)
    for (cmd_line = 0; cmd_line < wnd_row; cmd_line++)
      wnd[cmd_col * wnd_row + cmd_line] =
	newwin (l / wnd_row, COLS / wnd_col, 2 + cmd_line * l / wnd_row,
		cmd_col * COLS / wnd_col);
  cmd_line = 0;
  cmd_col = 0;
  status_win = wnd[wnd_col * wnd_row - 1];
  box (status_win, 0, 0);
  scrollok (status_win, TRUE);

  if (!wnd[0])
    {
      printf ("ncurser not supported\n");
      goto end;
    }

  devcli_set_general_notifier (devcli_server (), status_change,
			       wnd[wnd_col * wnd_row - 2]);
  status (wnd[wnd_col * wnd_row - 2], devcli_server ());

  for (i = 0, dev = devcli_devices (); dev && i < (wnd_col - 1) * wnd_row;
       dev = dev->next, i++)
    {
      devcli_set_general_notifier (dev, status_change, wnd[i]);
      status (wnd[i], dev);
    }

  while (1)
    {
      int key = wgetch (cmd_win);
      switch (key)
	{
	case KEY_F (2):
	  devcli_server_command (NULL, "off");
	  break;
	case KEY_F (3):
	  devcli_server_command (NULL, "standby");
	  break;
	case KEY_F (4):
	  devcli_server_command (NULL, "on");
	  break;
	case KEY_F (5):
	  status (wnd[wnd_col * wnd_row - 2], devcli_server ());
	  for (i = 0, dev = devcli_devices ();
	       dev && i < (wnd_col - 1) * wnd_row; dev = dev->next, i++)
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
