/*! 
 * @file Planer client.
 * $Id$
 * 
 * Implements planner client. That kind of client reads plan from stdin
 * or file, waits for time points marked in such datas, and send
 * commands written below that time points to clients.
 *
 * It prints non-fatal informations to stdout, fatal errors and
 * warnings goes to stderr.
 *
 * One line in plan ought to have at maximum MAX_LINE_SIZE characters, others
 * will be ignored.
 * 
 * @author petr
 */

#define _GNU_SOURCE
#define _XOPEN_SOURCE

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <malloc.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

#include "plancom.h"

#define MAX_LINE_SIZE	1024

time_t last_time;		//! Last time, used for + times.
/*! 
 * Waiting cycle
 * Differ if we are writing to file or to terminal.
 */
void
wait (time_t end_time)
{
  printf ("Time: %li\n", end_time);
  if (isatty (1))
    {
      while (end_time > time (NULL))
	{
	  printf ("Sleeping for %5i seconds\r",
		  (int) (end_time - time (NULL)));
	  fflush (stdout);
	  sleep (1);
	}
    }
  else
    {
      while (end_time > time (NULL))
	sleep (1);
    }
  printf ("Sleeping done.                 \n");
  fflush (stdout);
}

/*! 
 * Process one line from code. 
 * @param line Line to process, without end-of-line symbol
 */
void
process_line (char *line, int line_num)
{
  char *ptr;
  struct tm t;
  int ret;
  time_t tim;
  if ((ptr = strchr (line, '#')))	// ignore comments
    *ptr = '\0';
  // tabulator expanded chars, time entries
  switch (*line)
    {
    case '\t':			// tabulators denotes commands..
      printf ("Command: %s..", line++);
      if ((ret = plancom_process_command (line)) < 0)
	printf ("ERROR %i (%s)\n", ret, strerror (errno));
      else
	printf ("OK %i\n", ret);
      break;
      // wait some time after last event
    case '+':
      if (last_time < 0)
	{
	  printf
	    ("Finding '+' entry, but last_time isn't specified. Ignoring.\n");
	  return;
	}
      tim = strtol (line, &line, 10);
      if (*line != ' ' || *line != 0)
	{
	  printf ("Some bargage after end of seconds found. Ignoring %i\n",
		  (int) tim);
	}
      tim += last_time;
      wait (tim);
      break;
    case '\0':
      break;
    default:			// default entries are for time..
      if (strptime (line, "%d.%m.%Y %H:%M:%S", &t) == NULL)
	{
	  fprintf (stderr, "Bad time format: %s\n", line);
	  return;
	}
      last_time = tim = mktime (&t);
      wait (tim);
    }
}

int
main (int argc, char **argv)
{
  char line[MAX_LINE_SIZE + 1];
  int line_num = 1;
  last_time = -1;

  plancom_add_namespace ("tel", "lascaux", 5555);
  plancom_add_namespace ("wf", "lascaux", 5556);
  plancom_add_namespace ("nf", "lascaux", 5557);

  while (fgets (line, MAX_LINE_SIZE + 1, stdin))
    {
      line[strlen (line) - 1] = 0;	// delete ending \n
      printf ("line:%i -- %s\n", line_num, line);
      process_line (line, line_num);
      line_num++;
    }
  return 0;
}
