#ifndef __RTS2_PHOT__
#define __RTS2_PHOT__

#include "../utils/rts2device.h"
#include "status.h"

#include <sys/time.h>

class Rts2DevPhot:public Rts2Device
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

  virtual Rts2DevConn *createConnection (int in_sock);

  virtual int deleteConnection (Rts2Conn * conn)
  {
    if (integrateConn == conn)
      integrateConn = NULL;
    return Rts2Device::deleteConnection (conn);
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
  int moveFilter (Rts2Conn * conn, int new_filter);
  int enableFilter (Rts2Conn * conn);

  virtual void cancelPriorityOperations ();

  virtual int changeMasterState (int new_state);

};

class Rts2DevConnPhot:public Rts2DevConn
{
private:
  Rts2DevPhot * master;
  int keepInformed;
protected:
    virtual int commandAuthorized ();
public:
    Rts2DevConnPhot (int in_sock, Rts2DevPhot * in_master_device);
};



#endif /* !__RTS2_PHOT__ */
