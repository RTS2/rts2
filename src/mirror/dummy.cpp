#include "mirror.h"

class Rts2DevMirrorDummy:public Rts2DevMirror
{
public:
  Rts2DevMirrorDummy (int argc, char **argv):Rts2DevMirror (argc, argv)
  {
  }
  virtual int endOpen ()
  {
    sleep (10);
    return Rts2DevMirror::endOpen ();
  }
  virtual int isOpened ()
  {
    return -2;
  }
  virtual int isClosed ()
  {
    return -2;
  }
  virtual int endClose ()
  {
    sleep (10);
    return Rts2DevMirror::endClose ();
  }

  virtual int ready ()
  {
    return -1;
  }

  virtual int baseInfo ()
  {
    return -1;
  }

  virtual int info ()
  {
    return -1;
  }
};


main (int argc, char **argv)
{
  Rts2DevMirrorDummy *device = new Rts2DevMirrorDummy (argc, argv);

  int ret;
  ret = device->init ();
  if (ret)
    {
      syslog (LOG_ERR, "Cannot initialize FRAM mirror - exiting!\n");
      exit (1);
    }
  device->run ();
  delete device;
}
