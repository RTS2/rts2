#include "../writers/rts2imagedb.h"

int
main (int argc, char **argv)
{
  Rts2ImageDb *image;
  EXEC SQL CONNECT TO stars;
  image = new Rts2ImageDb (argv[1]);
  image->saveImage ();
  delete image;
  return 0;
}
