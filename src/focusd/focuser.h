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
  int homePos;
protected:
  char *device_file;
  char focCamera[20];
  char focType[20];
  int focPos;
  int focPositionNew;
  float focTemp;
  int focSwitches;		// bitfield holding power switches state - for Robofocus
  int switchNum;
  // minimal steps/sec count; 5 sec will be added to top it
  int focStepSec;

  virtual int isFocusing ();
  virtual int endFocusing ();

  void setFocusTimeout (int timeout);
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
  // set to given number
  // default to use stepOut function
  virtual int setTo (int num);
  virtual int home ();

  // set switch state
  virtual int setSwitch (int switch_num, int new_state)
  {
    return -1;
  }

  // callback functions from focuser connection
  int idle ();
  int ready (Rts2Conn * conn);
  int sendInfo (Rts2Conn * conn);
  int sendBaseInfo (Rts2Conn * conn);
  void checkState ();
  int stepOut (Rts2Conn * conn, int num);
  int setTo (Rts2Conn * conn, int num);
  int home (Rts2Conn * conn);
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
