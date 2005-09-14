#ifndef __RTS2_COPULA__
#define __RTS2_COPULA__

#include "dome.h"
#include "status.h"

#include <libnova/libnova.h>

class Rts2DevCopula:public Rts2DevDome
{
private:
  struct ln_equ_posn targetPos;
  struct ln_lnlat_posn *observer;	// defaults to 0, 0; will be readed from config file

  double currentAz;		// azimut to which we are pointing; counted in libnova style (S->W->N->E)

  char *configFile;

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
    currentAz = in_az;
  }
public:
  Rts2DevCopula (int argc, char **argv);

  virtual int processOption (int in_opt);
  virtual int init ();
  virtual Rts2DevConn *createConnection (int in_sock, int conn_num);
  virtual int idle ();

  virtual int sendBaseInfo (Rts2Conn * conn);
  virtual int sendInfo (Rts2Conn * conn);

  int moveTo (Rts2Conn * conn, double ra, double dec);
  virtual int moveStop ();

  // returns target current alt & az
  void getTargetAltAz (struct ln_hrz_posn *hrz);
  // returns 0 when we are satisfied with curent position, 1 when split position change is needed, -1 when we cannot reposition to given ra/dec
  virtual int needSplitChange ();
  // calculate split width in arcdeg for given altititude; when copula don't have split at given altitude, returns -1
  virtual double getSplitWidth (double alt)
  {
    return 1;
  }
};

class Rts2DevConnCopula:public Rts2DevConnDome
{
private:
  Rts2DevCopula * master;
protected:
  virtual int commandAuthorized ();
public:
    Rts2DevConnCopula (int in_sock,
		       Rts2DevCopula *
		       in_master_device):Rts2DevConnDome (in_sock,
							  in_master_device)
  {
    master = in_master_device;
  }
};

#endif /* !__RTS2_COPULA__ */
