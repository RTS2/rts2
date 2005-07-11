#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
/*!
 * Used for generating new plan entries.
 *
 * @author petr
 */

#include "rts2selector.h"

#include <signal.h>

Rts2Selector *selector;

void
killSignal (int sig)
{
  if (selector)
    delete selector;
  exit (0);
}

int
main (int argc, char **argv)
{
  int ret;
  selector = new Rts2Selector (argc, argv);
  signal (SIGTERM, killSignal);
  signal (SIGINT, killSignal);
  ret = selector->init ();
  if (ret)
    return ret;
  selector->run ();
  delete selector;
  return 0;
}
