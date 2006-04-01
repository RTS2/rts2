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
  Rts2Script *nextScript;
  Target *nextTarget;
  int dont_execute_for;
  void setNextTarget (Target * in_target);
protected:
    Target * currentTarget;
  Rts2Script *script;
  Rts2Command *nextComd;
  char cmd_device[DEVICE_NAME_SIZE];

  enum
  { NO_WAIT, WAIT_SLAVE, WAIT_MASTER, WAIT_SIGNAL, WAIT_MIRROR,
    WAIT_SEARCH
  } waitScript;

  int blockMove;
//  enum
//  { NO_START, START_CURRENT, START_NEXT } getObserveStart;
  virtual void startTarget ();

  virtual int getNextCommand () = 0;
  int nextPreparedCommand ();
  /**
   * Entry point for script execution.
   * 
   * That's entry point to script execution. It's called when device
   * is free to do new job - e.g when camera finish exposure (as we
   * can move filter wheel during chip readout).
   *
   * It can be called more then once for one command - hence we keep
   * in nextComd prepared next command, and return it from
   * nextPreparedCommand when it's wise to return it.
   *
   * @return 0 when there isn't any next command to execute, 1 when
   * there is next command available.
   */
  int haveNextCommand ();
  virtual void unblockWait () = 0;
  virtual void unsetWait () = 0;
  virtual void clearWait () = 0;
  virtual int isWaitMove () = 0;
  virtual void setWaitMove () = 0;
  virtual int queCommandFromScript (Rts2Command * comm) = 0;

  virtual int getFailedCount () = 0;
  virtual void clearFailedCount () = 0;

  virtual void deleteScript ();

  // called when we find source..
  virtual void searchSucess ();

  virtual void clearBlockMove ()
  {
    blockMove = 0;
  }

public:
    Rts2DevScript (Rts2Conn * in_script_connection);
  virtual ~ Rts2DevScript (void);
  virtual void postEvent (Rts2Event * event);
  virtual void nextCommand () = 0;
};


#endif /* !__RTS2_DEVSCRIPT__ */
