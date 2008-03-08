#include <stdio.h>

#include "mirror.h"

int
main (int argc, char **argv)
{
  int fd;
  int ret;

  int c;

  ret = mirror_open_dev (argv[1]);

  printf ("ret: %i\n", ret);

  while ((c = getchar ()) != EOF)
    {
      switch (c)
	{
	case 'l':
	  ret = mirror_open ();
	  break;
	case 'k':
	  ret = mirror_close ();
	  break;
	case 'q':
	  exit (1);
	default:
	  printf ("unknow command: %i\n", c);
	}
      printf ("ret: %i %i\n", ret, c);
    }
  printf ("c: %i\n", c);
}
