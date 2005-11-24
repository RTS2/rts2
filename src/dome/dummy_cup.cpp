#include "cupola.h"

#include <signal.h>

/*!
 * Dummy copula for testing.
 */
class Rts2DevCopulaDummy:public Rts2DevCopula
{
private:
  int mcount;
protected:
    virtual int moveStart ()
  {
    mcount = 0;
    return Rts2DevCopula::moveStart ();
  }
  virtual int moveEnd ()
  {
    struct ln_hrz_posn hrz;
    getTargetAltAz (&hrz);
    setCurrentAz (hrz.az);
    return Rts2DevCopula::moveEnd ();
  }
  virtual long isMoving ()
  {
    if (mcount >= 100)
      return -2;
    mcount++;
    return USEC_SEC;
  }

public:
Rts2DevCopulaDummy (int in_argc, char **in_argv):Rts2DevCopula (in_argc,
		 in_argv)
  {
  }

  virtual int ready ()
  {
    return 0;
  }

  virtual int info ()
  {
    return 0;
  }

  virtual int baseInfo ()
  {
    return 0;
  }

  virtual int openDome ()
  {
    mcount = 0;
    return Rts2DevCopula::openDome ();
  }

  virtual long isOpened ()
  {
    return isMoving ();
  }

  virtual int closeDome ()
  {
    mcount = 0;
    return Rts2DevCopula::closeDome ();
  }

  virtual long isClosed ()
  {
    return isMoving ();
  }

  virtual double getSplitWidth (double alt)
  {
    return 1;
  }
};

Rts2DevCopulaDummy *device;

int
main (int argc, char **argv)
{
  int ret;
  device = new Rts2DevCopulaDummy (argc, argv);
  ret = device->init ();
  if (ret)
    {
      fprintf (stderr, "Cannot initialize dummy copula - exiting!\n");
      exit (0);
    }
  device->run ();
  delete device;
}
