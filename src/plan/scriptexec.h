#ifndef __SCRIPT_EXEC__
#define __SCRIPT_EXEC__

#include "rts2targetscr.h"
#include "../utils/rts2client.h"
#include "../utils/rts2target.h"

class Rts2TargetScr;

/**
 * Holds script for given device.
 */
class Rts2ScriptForDevice
{
private:
  std::string deviceName;
  std::string script;
public:
  Rts2ScriptForDevice (std::string in_deviceName, std::string in_script);
  virtual ~ Rts2ScriptForDevice (void);

  bool isDevice (std::string in_deviceName)
  {
    return deviceName == in_deviceName;
  }

  const char *getScript ()
  {
    return script.c_str ();
  }
};

/**
 * This is main client class. It takes care of supliing right
 * devclients and other things.
 */
class Rts2ScriptExec:public Rts2Client
{
private:
  std::vector < Rts2ScriptForDevice > scripts;
  char *deviceName;

  int waitState;

  Rts2TargetScr *currentTarget;

  time_t nextRunningQ;

  bool isScriptRunning ();

    std::string getStreamAsString (std::istream & _is);

  char *configFile;
protected:
    virtual int processOption (int in_opt);

  virtual int init ();
  virtual int doProcessing ();

public:
    Rts2ScriptExec (int in_argc, char **in_argv);
    virtual ~ Rts2ScriptExec (void);

  Rts2ScriptForDevice *findScript (std::string deviceName);

  virtual Rts2DevClient *createOtherType (Rts2Conn * conn,
					  int other_device_type);

  virtual void postEvent (Rts2Event * event);
  virtual void deviceReady (Rts2Conn * conn);
  virtual int idle ();
  virtual void deviceIdle (Rts2Conn * conn);

  int getPosition (struct ln_equ_posn *pos, double JD)
  {
    pos->ra = 20;
    pos->dec = 20;
    return 0;
  }
};

#endif /* !__SCRIPT_EXEC__ */
