#include <stdio.h>

#include "tcf.h"


int
main (char argc, char **argv)
{
  int fd;
  int pos = 0;
  float temp;

  set_focuser_port (argv[1]);
  fd = tcf_set_manual ();

  printf ("fd: %i\n", fd);

  tcf_get_pos (&pos);

  printf ("postion: %i\n", pos);

//  tcf_step_out (1000, 1);

  tcf_get_pos (&pos);

  printf ("postion: %i\n", pos);

  tcf_get_temperature (&temp);

  printf ("temp: %f\n", temp);

// tcf_step_out (1000, -1);

}
