/*!
 * $Id$
 *
 * @author petr
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "focuser.h"

class Rts2DevFocuserDummy:public Rts2DevFocuser
{
public:
  Rts2DevFocuserDummy (int argc, char **argv);
   ~Rts2DevFocuserDummy (void);
  virtual int ready ();
  virtual int baseInfo ();
  virtual int info ();
  virtual int stepOut (int num);
  virtual int isFocusing ();
};

Rts2DevFocuserDummy::Rts2DevFocuserDummy (int argc, char **argv):
Rts2DevFocuser (argc, argv)
{
  focPos = 3000;
  focTemp = 100;
}

Rts2DevFocuserDummy::~Rts2DevFocuserDummy ()
{
}

int
Rts2DevFocuserDummy::ready ()
{
  return 0;
}

int
Rts2DevFocuserDummy::baseInfo ()
{
  strcpy (focType, "Dummy");
  return 0;
}

int
Rts2DevFocuserDummy::info ()
{
  return 0;
}

int
Rts2DevFocuserDummy::stepOut (int num)
{
  focPos += num;
  return 0;
}

int
Rts2DevFocuserDummy::isFocusing ()
{
  return 0;
}

int
main (int argc, char **argv)
{
  Rts2DevFocuserDummy *device = new Rts2DevFocuserDummy (argc, argv);

  int ret;
  ret = device->init ();
  if (ret)
    {
      fprintf (stderr, "Cannot initialize focuser - exiting!\n");
      exit (1);
    }
  device->run ();
  delete device;
  return 0;
}
