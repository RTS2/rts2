#ifndef __RTS2_MIRROR__
#define __RTS2_MIRROR__

#include "camera_cpp.h"


class Rts2DevCameraMirror:public Rts2DevCamera
{
  char *mirror_dev;
  int mirror_fd;
  FILE *mirr_log;

  int mirror_command (char cmd, int arg, char *ret_cmd, int *ret_arg);
  int mirror_get (int *pos);
  // need to set steps before calling this one
  int mirror_set ();

  int steps;
  int set_ret;
  int cmd;

  int expChip;
  int expLight;
  float expExpTime;

public:
    Rts2DevCameraMirror (int argc, char **argv);
    virtual ~ Rts2DevCameraMirror (void);
  virtual int processOption (int in_op);
  virtual int init ();
  virtual int idle ();

  virtual Rts2Conn *createConnection (int in_sock, int conn_num);

  virtual int startOpen ();
  virtual int isOpened ();
  virtual int endOpen ();

  virtual int startClose ();
  virtual int isClosed ();
  virtual int endClose ();

  // return 1, when mirror is (still) moving
  virtual int isMoving ()
  {
    return !!cmd;
  }

  virtual int camExpose (int chip, int light, float exptime);

  virtual long camWaitExpose (int chip);
};

class Rts2DevConnCameraMirror:public Rts2DevConnCamera
{
private:
  Rts2DevCameraMirror * master;
protected:
  virtual int commandAuthorized ();
public:
    Rts2DevConnCameraMirror (int in_sock,
			     Rts2DevCameraMirror *
			     in_master_device):Rts2DevConnCamera (in_sock,
								  in_master_device)
  {
    master = in_master_device;
  }
};

#endif /* ! __RTS2_MIRROR__ */
