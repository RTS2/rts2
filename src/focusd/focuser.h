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
  Rts2ValueInteger *focPos;
  int focPositionNew;
  Rts2ValueFloat *focTemp;
  // minimal steps/sec count; 5 sec will be added to top it
  int focStepSec;
  int startPosition;

  virtual int isFocusing ();
  virtual int endFocusing ();

  void setFocusTimeout (int timeout);

  virtual bool isAtStartPosition ();
  int checkStartPosition ();

  virtual int setValue (Rts2Value * old_value, Rts2Value * new_value);

  void createFocTemp ()
  {
    createValue (focTemp, "FOC_TEMP", "focuser temperature");
  }
public:
    Rts2DevFocuser (int argc, char **argv);
  virtual int processOption (int in_opt);

  // callback functions
  virtual int ready ()
  {
    return -1;
  };
  virtual int stepOut (int num) = 0;
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
  virtual int initValues ();
  virtual int idle ();
  virtual int ready (Rts2Conn * conn);
  void checkState ();
  int stepOut (Rts2Conn * conn, int num);
  int setTo (Rts2Conn * conn, int num);
  int home (Rts2Conn * conn);
  int autoFocus (Rts2Conn * conn);

  int getFocPos ()
  {
    return focPos->getValueInteger ();
  }

  virtual int commandAuthorized (Rts2Conn * conn);
};

#endif
