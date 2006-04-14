#include "ir.h"

int
main (int argc, char **argv)
{
  Rts2DevTelescopeIr *device = new Rts2DevTelescopeIr (argc, argv);

  int ret;
  ret = device->init ();
  if (ret)
    {
      fprintf (stderr, "Cannot initialize telescope bridge - exiting!\n");
      exit (0);
    }
  ret = device->run ();
  delete device;

  return ret;
}
