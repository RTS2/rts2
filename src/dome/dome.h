#ifndef __RTS2_DOME__
#define __RTS2_DOME__

#include "../utils/rts2block.h"
#include "../utils/rts2device.h"

class Rts2DevDome:public Rts2Device
{
public:
  Rts2DevDome (int argc, char **argv);
  virtual int openDome ()
  {
    maskState (0, DOME_DOME_MASK, DOME_OPENING, "opening dome");
    return 0;
  }
  virtual int endOpen ()
  {
    maskState (0, DOME_DOME_MASK, DOME_OPENED, "dome opened");
    return 0;
  };
  virtual long isOpened ()
  {
    return -2;
  };
  virtual int closeDome ()
  {
    maskState (0, DOME_DOME_MASK, DOME_CLOSING, "closing dome");
    return 0;
  };
  virtual int endClose ()
  {
    maskState (0, DOME_DOME_MASK, DOME_CLOSED, "dome closed");
    return 0;
  };
  virtual long isClosed ()
  {
    return -2;
  };
  int checkOpening ();
  virtual int init ();
  virtual Rts2Conn *createConnection (int in_sock, int conn_num);
  virtual int idle ();

  virtual int ready ()
  {
    return -1;
  };
  virtual int baseInfo ()
  {
    return -1;
  };
  virtual int info ()
  {
    return -1;
  };

  // callback function from dome connection
  int ready (Rts2Conn * conn);
  int baseInfo (Rts2Conn * conn);
  int info (Rts2Conn * conn);

  virtual int observing ();
  virtual int standby ();
  virtual int off ();

  virtual int setMasterState (int new_state);
};

class Rts2DevConnDome:public Rts2DevConn
{
private:
  Rts2DevDome * master;
protected:
  virtual int commandAuthorized ();
public:
    Rts2DevConnDome (int in_sock,
		     Rts2DevDome * in_master_device):Rts2DevConn (in_sock,
								  in_master_device)
  {
    master = in_master_device;
  };
};

#endif /* ! __RTS2_DOME__ */
