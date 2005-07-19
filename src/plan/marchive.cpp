/* Processed by ecpg (3.1.1) */
/* These include files are added by the preprocessor */
#include <ecpgtype.h>
#include <ecpglib.h>
#include <ecpgerrno.h>
#include <sqlca.h>
#line 1 "marchive.ec"
/* End of automatic include section */
#include "../writers/rts2imagedb.h"

int
main (int argc, char **argv)
{
  Rts2ImageDb *image;
  {
    ECPGconnect (__LINE__, 0, "stars2", NULL, NULL, NULL, 0);
  }
#line 7 "marchive.ec"

  image = new Rts2ImageDb (argv[1]);
  image->saveImage ();
  delete image;
  return 0;
}
