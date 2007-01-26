#ifndef __RTS2_COPULA__
#define __RTS2_COPULA__

#include "dome.h"
#include "status.h"

#include <libnova/libnova.h>

class Rts2DevCupola:public Rts2DevDome
{
private:
  struct ln_equ_posn targetPos;
  struct ln_lnlat_posn *observer;	// defaults to 0, 0; will be readed from config file

  Rts2ValueDouble *tarRa;
  Rts2ValueDouble *tarDec;
  Rts2ValueDouble *tarAlt;
  Rts2ValueDouble *tarAz;
  Rts2ValueDouble *currentAz;

  char *configFile;

  double targetDistance;
  void synced ();

protected:
  // called to bring copula in sync with target az
    virtual int moveStart ();
  // same meaning of return values as Rts2DevTelescope::isMoving
  virtual long isMoving ()
  {
    return -2;
  }
  virtual int moveEnd ();

  void setCurrentAz (double in_az)
  {
    currentAz->setValueDouble (in_az);
  }

  double getTargetDistance ()
  {
    return targetDistance;
  }

  double getCurrentAz ()
  {
    return currentAz->getValueDouble ();
  }

public:
  Rts2DevCupola (int argc, char **argv);

  virtual int processOption (int in_opt);
  virtual int init ();
  virtual Rts2DevConn *createConnection (int in_sock);
  virtual int info ();
  virtual int idle ();

  int moveTo (Rts2Conn * conn, double ra, double dec);
  virtual int moveStop ();

  // returns target current alt & az
  void getTargetAltAz (struct ln_hrz_posn *hrz);
  // returns 0 when we are satisfied with curent position, 1 when split position change is needed.
  // set targetDistance to targetdistance in deg.. (it is in -180..+180 range)
  // -1 when we cannot reposition to given ra/dec
  virtual int needSplitChange ();
  // calculate split width in arcdeg for given altititude; when copula don't have split at given altitude, returns -1
  virtual double getSplitWidth (double alt) = 0;
};

class Rts2DevConnCupola:public Rts2DevConnDome
{
private:
  Rts2DevCupola * master;
protected:
  virtual int commandAuthorized ();
public:
    Rts2DevConnCupola (int in_sock,
		       Rts2DevCupola *
		       in_master_device):Rts2DevConnDome (in_sock,
							  in_master_device)
  {
    master = in_master_device;
  }
};

#endif /* !__RTS2_COPULA__ */
