#ifndef __RTS2_SCRIPTELEMENT__
#define __RTS2_SCRIPTELEMENT__

#include "rts2script.h"
#include "../utils/rts2object.h"
#include "../utils/rts2block.h"

#include "status.h"

class Rts2Script;

/**
 * This class defines one element in script, which is equal to one command in script.
 *
 * @author Petr Kubanek <pkubanek@asu.cas.cz>
 */
class Rts2ScriptElement:public Rts2Object
{
protected:
  Rts2Script * script;
  inline void getDevice (char new_device[DEVICE_NAME_SIZE]);
public:
    Rts2ScriptElement (Rts2Script * in_script);
    virtual ~ Rts2ScriptElement (void);
  virtual int nextCommand (Rts2Block * in_master,
			   Rts2DevClientCamera * camera,
			   Rts2Command ** new_command,
			   char new_device[DEVICE_NAME_SIZE]) = 0;
};

class Rts2ScriptElementExpose:public Rts2ScriptElement
{
private:
  float expTime;
public:
    Rts2ScriptElementExpose (Rts2Script * in_script, float in_expTime);
  virtual int nextCommand (Rts2Block * in_master,
			   Rts2DevClientCamera * camera,
			   Rts2Command ** new_command,
			   char new_device[DEVICE_NAME_SIZE]);
};

class Rts2ScriptElementDark:public Rts2ScriptElement
{
private:
  float expTime;
public:
    Rts2ScriptElementDark (Rts2Script * in_script, float in_expTime);
  virtual int nextCommand (Rts2Block * in_master,
			   Rts2DevClientCamera * camera,
			   Rts2Command ** new_command,
			   char new_device[DEVICE_NAME_SIZE]);
};

class Rts2ScriptElementChange:public Rts2ScriptElement
{
private:
  double ra;
  double dec;
public:
    Rts2ScriptElementChange (Rts2Script * in_script, double in_ra,
			     double in_dec);
  virtual int nextCommand (Rts2Block * in_master,
			   Rts2DevClientCamera * camera,
			   Rts2Command ** new_command,
			   char new_device[DEVICE_NAME_SIZE]);
};

class Rts2ScriptElementFilter:public Rts2ScriptElement
{
private:
  int filter;
public:
    Rts2ScriptElementFilter (Rts2Script * in_script, int in_filter);
  virtual int nextCommand (Rts2Block * in_master,
			   Rts2DevClientCamera * camera,
			   Rts2Command ** new_command,
			   char new_device[DEVICE_NAME_SIZE]);
};

#endif /* !__RTS2_SCRIPTELEMENT__ */
