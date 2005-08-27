#ifndef __RTS2_DEVSCRIPT__
#define __RTS2_DEVSCRIPT__

#include "rts2script.h"

#include "../utils/rts2object.h"
#include "../utils/rts2block.h"
#include "../utilsdb/target.h"

/**
 * That class provide scripting interface for devices, so they can 
 * run scrips..
 */

class Rts2DevScript:public Rts2Object
{
private:
  Rts2Conn * script_connection;
  int dont_execute_for;
protected:
    Target * currentTarget;
  Rts2Script *script;
  Rts2Command *nextComd;
  char cmd_device[DEVICE_NAME_SIZE];

  enum
  { NO_WAIT, WAIT_SLAVE, WAIT_MASTER, WAIT_SIGNAL, WAIT_MIRROR } waitScript;

  int blockMove;
  int getObserveStart;

  virtual void startTarget ();

  virtual int getNextCommand () = 0;
  int nextPreparedCommand ();
  int haveNextCommand ();
  virtual void unblockWait () = 0;
  virtual void unsetWait () = 0;
  virtual void clearWait () = 0;
  virtual int isWaitMove () = 0;
  virtual void setWaitMove () = 0;

  virtual void deleteScript ();

public:
    Rts2DevScript (Rts2Conn * in_script_connection);
    virtual ~ Rts2DevScript (void);
  virtual void postEvent (Rts2Event * event);
  virtual void nextCommand () = 0;
};


#endif /* !__RTS2_DEVSCRIPT__ */
