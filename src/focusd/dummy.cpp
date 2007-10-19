/*!
 * $Id$
 *
 * @author petr
 */

#include "focuser.h"

class Rts2DevFocuserDummy:public Rts2DevFocuser
{
private:
  int steps;
public:
    Rts2DevFocuserDummy (int argc, char **argv);
   ~Rts2DevFocuserDummy (void);
  virtual int initValues ();
  virtual int ready ();
  virtual int stepOut (int num);
  virtual int isFocusing ();
};

Rts2DevFocuserDummy::Rts2DevFocuserDummy (int in_argc, char **in_argv):
Rts2DevFocuser (in_argc, in_argv)
{
  focStepSec = 1;
  strcpy (focType, "Dummy");
  createFocTemp ();
}

Rts2DevFocuserDummy::~Rts2DevFocuserDummy ()
{
}

int
Rts2DevFocuserDummy::initValues ()
{
  focPos->setValueInteger (3000);
  focTemp->setValueFloat (100);
  return Rts2DevFocuser::initValues ();
}


int
Rts2DevFocuserDummy::ready ()
{
  return 0;
}

int
Rts2DevFocuserDummy::stepOut (int num)
{
  steps = 1;
  if (num < 0)
    steps *= -1;
  return 0;
}

int
Rts2DevFocuserDummy::isFocusing ()
{
  if (fabs (getFocPos () - focPositionNew) < fabs (steps))
    focPos->setValueInteger (focPositionNew);
  else
    focPos->setValueInteger (getFocPos () + steps);
  return Rts2DevFocuser::isFocusing ();
}

int
main (int argc, char **argv)
{
  Rts2DevFocuserDummy device = Rts2DevFocuserDummy (argc, argv);
  return device.run ();
}
