#ifndef __RTS2_PHOT__
#define __RTS2_PHOT__

#include "../utils/rts2scriptdevice.h"
#include "status.h"

#include <sys/time.h>

class Rts2DevPhot:public Rts2ScriptDevice
{
private:
  int req_count;
  struct timeval nextCountDue;
protected:
    Rts2Conn * integrateConn;

  Rts2ValueInteger *filter;
  float req_time;
  void setReqTime (float in_req_time);

  char *photType;
  char *serial;

  virtual int setValue (Rts2Value * old_value, Rts2Value * new_value);

  int sendCount (int count, float exp, int is_ov);
  virtual int startIntegrate ();
  virtual int endIntegrate ();

public:
    Rts2DevPhot (int argc, char **argv);
  // return time till next getCount call in usec, or -1 when failed
  virtual long getCount ()
  {
    return -1;
  }
  virtual int initValues ();

  virtual int idle ();

  virtual int deleteConnection (Rts2Conn * conn)
  {
    if (integrateConn == conn)
      integrateConn = NULL;
    return Rts2ScriptDevice::deleteConnection (conn);
  }

  virtual int homeFilter ();

  void checkFilterMove ();

  virtual int startFilterMove (int new_filter);
  virtual long isFilterMoving ();
  virtual int endFilterMove ();
  virtual int enableMove ();
  virtual int disableMove ();

  int startIntegrate (Rts2Conn * conn, float in_req_time, int in_req_count);
  virtual int stopIntegrate ();

  int homeFilter (Rts2Conn * conn);
  int moveFilter (int new_filter);
  int enableFilter (Rts2Conn * conn);

  virtual void cancelPriorityOperations ();

  virtual int changeMasterState (int new_state);

  virtual int commandAuthorized (Rts2Conn * conn);
};

#endif /* !__RTS2_PHOT__ */
