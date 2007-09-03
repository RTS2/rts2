#include "rts2spiral.h"

#include <curses.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

void
endW (int sig)
{
  erase ();
  refresh ();

  nocbreak ();
  echo ();
  endwin ();

  exit (0);
}

int
main (int argc, char **argv)
{
  int step;
  short next_x, next_y;
  int x, y;
  Rts2Spiral *spiral;

  initscr ();
  cbreak ();
  keypad (stdscr, TRUE);
  noecho ();

  signal (SIGINT, endW);
  signal (SIGTERM, endW);

  y = LINES / 2;
  x = COLS / 6;

  spiral = new Rts2Spiral ();

  for (step = 0; step < 100; step++)
    {
      mvwprintw (stdscr, y, x * 3, "%i", step);
      spiral->getNextStep (next_x, next_y);
      refresh ();
      sleep (1);
      x += next_x;
      y += next_y;
    }

  endW (0);

  return 0;
}
