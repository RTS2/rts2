#ifndef __RTS2_MIRROR__
#define __RTS2_MIRROR__

#include "../utils/rts2block.h"
#include "../utils/rts2device.h"

class Rts2DevMirror:public Rts2Device
{
public:
  Rts2DevMirror (int argc, char **argv);
    virtual ~ Rts2DevMirror (void);
  virtual int processOption (int in_opt)
  {
    return Rts2Device::processOption (in_opt);
  }
  virtual int init ();
  virtual int idle ();

  virtual Rts2DevConn *createConnection (int in_sock, int conn_num);

  virtual int startOpen ()
  {
    return maskState (0, MIRROR_MASK, MIRROR_A_B, "moving A->B");
  }
  virtual int isOpened ()
  {
    return -1;
  }
  virtual int endOpen ()
  {
    return maskState (0, MIRROR_MASK, MIRROR_B, "moved A->B");
  }

  virtual int startClose ()
  {
    return maskState (0, MIRROR_MASK, MIRROR_B_A, "moving B->A");
  }
  virtual int isClosed ()
  {
    return -1;
  }
  virtual int endClose ()
  {
    return maskState (0, MIRROR_MASK, MIRROR_A, "moved B->A");
  }

  // return 1, when mirror is (still) moving
  virtual int isMoving ()
  {
    return ((getState (0) & MIRROR_MASK) == MIRROR_A_B
	    || (getState (0) & MIRROR_MASK) == MIRROR_B_A);
  }

  virtual int ready ()
  {
    return -1;
  }

  virtual int baseInfo ()
  {
    return -1;
  }

  int startOpen (Rts2Conn * conn);
  int startClose (Rts2Conn * conn);

  virtual int sendBaseInfo (Rts2Conn * conn);
  virtual int sendInfo (Rts2Conn * conn);
};

class Rts2DevConnMirror:public Rts2DevConn
{
private:
  Rts2DevMirror * master;
protected:
  virtual int commandAuthorized ();
public:
    Rts2DevConnMirror (int in_sock,
		       Rts2DevMirror *
		       in_master_device):Rts2DevConn (in_sock,
						      in_master_device)
  {
    master = in_master_device;
  }
};

#endif /* ! __RTS2_MIRROR__ */
