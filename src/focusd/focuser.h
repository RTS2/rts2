/*!
 * $Id$
 *
 * @author standa
 */

#ifndef __RTS2_FOCUSD_CPP__
#define __RTS2_FOCUSD_CPP__

#include "../utils/rts2block.h"
#include "../utils/rts2device.h"

class Rts2DevFocuser:public Rts2Device
{
private:
  time_t focusTimeout;
protected:
  char *device_file;
  char focCamera[20];
  char focType[20];
  int focPos;
  int focPositionNew;
  float focTemp;
  // minimal steps/sec count; 5 sec will be added to top it
  int focStepSec;

  virtual int isFocusing ();
  virtual int endFocusing ();
public:
    Rts2DevFocuser (int argc, char **argv);
  virtual int processOption (int in_opt);
  virtual Rts2DevConn *createConnection (int in_sock, int conn_num);

  // callback functions
  virtual int ready ()
  {
    return -1;
  };
  virtual int info ()
  {
    return -1;
  };
  virtual int baseInfo ()
  {
    return -1;
  };
  virtual int stepOut (int num)
  {
    return -1;
  };

  // callback functions from focuser connection
  int idle ();
  int ready (Rts2Conn * conn);
  int sendInfo (Rts2Conn * conn);
  int baseInfo (Rts2Conn * conn);
  int checkState ();
  int stepOut (Rts2Conn * conn, int num);
  int autoFocus (Rts2Conn * conn);
};

class Rts2DevConnFocuser:public Rts2DevConn
{
private:
  Rts2DevFocuser * master;
protected:
  virtual int commandAuthorized ();
public:
    Rts2DevConnFocuser (int in_sock, Rts2DevFocuser * in_master_device);
};
#endif
