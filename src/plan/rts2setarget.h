#ifndef __RTS2_SETARGET__
#define __RTS2_SETARGET__

#include "rts2script.h"
#include "rts2scriptelementacquire.h"
#include "rts2scriptelement.h"
#include "../utils/rts2target.h"

class Rts2SETarget:public Rts2ScriptElement
{
private:
  Rts2Target * target;
protected:
  Rts2Target * getTarget ()
  {
    return target;
  }
public:
    Rts2SETarget (Rts2Script * in_script,
		  Rts2Target * in_target):Rts2ScriptElement (in_script)
  {
    target = in_target;
  }
};

class Rts2SETDisable:public Rts2SETarget
{
public:
  Rts2SETDisable (Rts2Script * in_script,
		  Rts2Target * in_target):Rts2SETarget (in_script, in_target)
  {
  }
  virtual int defnextCommand (Rts2DevClient * client,
			      Rts2Command ** new_command,
			      char new_device[DEVICE_NAME_SIZE]);
};

class Rts2SETTempDisable:public Rts2SETarget
{
private:
  int seconds;
public:
    Rts2SETTempDisable (Rts2Script * in_script, Rts2Target * in_target,
			int in_seconds):Rts2SETarget (in_script, in_target)
  {
    seconds = in_seconds;
  }
  virtual int defnextCommand (Rts2DevClient * client,
			      Rts2Command ** new_command,
			      char new_device[DEVICE_NAME_SIZE]);
};

class Rts2SETTarBoost:public Rts2SETarget
{
private:
  int seconds;
  int bonus;
public:
    Rts2SETTarBoost (Rts2Script * in_script, Rts2Target * in_target,
		     int in_seconds, int in_bonus):Rts2SETarget (in_script,
								 in_target)
  {
    seconds = in_seconds;
    bonus = in_bonus;
  }
  virtual int defnextCommand (Rts2DevClient * client,
			      Rts2Command ** new_command,
			      char new_device[DEVICE_NAME_SIZE]);
};

#endif /* !__RTS2_SETARGET__ */
