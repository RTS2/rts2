#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "rts2selector.h"

#include <iostream>

int
main (int argc, char **argv)
{
  int ret;
  int next_tar;
  Rts2Selector *selector;
  selector = new Rts2Selector (argc, argv);
  ret = selector->init ();
  if (ret)
    return ret;
  next_tar = selector->selectNextNight ();
  next_tar = selector->selectNextNight ();
  std::cout << "Next target:" << next_tar << std::endl;
  delete selector;
}
