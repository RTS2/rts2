#include "cupola.h"

/*!
 * Dummy copula for testing.
 */
class Rts2DevCupolaDummy:public Rts2DevCupola
{
private:
  int mcount;
protected:
    virtual int moveStart ()
  {
    mcount = 0;
    return Rts2DevCupola::moveStart ();
  }
  virtual int moveEnd ()
  {
    struct ln_hrz_posn hrz;
    getTargetAltAz (&hrz);
    setCurrentAz (hrz.az);
    return Rts2DevCupola::moveEnd ();
  }
  virtual long isMoving ()
  {
    if (mcount >= 100)
      return -2;
    mcount++;
    return USEC_SEC;
  }

public:
Rts2DevCupolaDummy (int in_argc, char **in_argv):Rts2DevCupola (in_argc,
		 in_argv)
  {
  }

  virtual int ready ()
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
    return Rts2DevCupola::openDome ();
  }

  virtual long isOpened ()
  {
    return isMoving ();
  }

  virtual int closeDome ()
  {
    mcount = 0;
    return Rts2DevCupola::closeDome ();
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

int
main (int argc, char **argv)
{
  Rts2DevCupolaDummy *device = new Rts2DevCupolaDummy (argc, argv);
  int ret = device->run ();
  delete device;
  return ret;
}
