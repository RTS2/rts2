#ifndef __RTS2_SCRIPTELEMENT__
#define __RTS2_SCRIPTELEMENT__

#include "rts2script.h"
#include "rts2connimgprocess.h"
#include "../writers/rts2image.h"
#include "../utils/rts2object.h"
#include "../utils/rts2block.h"

#include "status.h"

#define EVENT_PRECISION_REACHED		RTS2_LOCAL_EVENT + 250

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
  virtual int nextCommand (Rts2DevClientCamera * camera,
			   Rts2Command ** new_command,
			   char new_device[DEVICE_NAME_SIZE]) = 0;
  virtual int processImage (Rts2Image * image);
};

class Rts2ScriptElementExpose:public Rts2ScriptElement
{
private:
  float expTime;
public:
    Rts2ScriptElementExpose (Rts2Script * in_script, float in_expTime);
  virtual int nextCommand (Rts2DevClientCamera * camera,
			   Rts2Command ** new_command,
			   char new_device[DEVICE_NAME_SIZE]);
};

class Rts2ScriptElementDark:public Rts2ScriptElement
{
private:
  float expTime;
public:
    Rts2ScriptElementDark (Rts2Script * in_script, float in_expTime);
  virtual int nextCommand (Rts2DevClientCamera * camera,
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
  virtual int nextCommand (Rts2DevClientCamera * camera,
			   Rts2Command ** new_command,
			   char new_device[DEVICE_NAME_SIZE]);
};

class Rts2ScriptElementWait:public Rts2ScriptElement
{
public:
  Rts2ScriptElementWait (Rts2Script * in_script);
  virtual int nextCommand (Rts2DevClientCamera * camera,
			   Rts2Command ** new_command,
			   char new_device[DEVICE_NAME_SIZE]);
};

class Rts2ScriptElementFilter:public Rts2ScriptElement
{
private:
  int filter;
public:
    Rts2ScriptElementFilter (Rts2Script * in_script, int in_filter);
  virtual int nextCommand (Rts2DevClientCamera * camera,
			   Rts2Command ** new_command,
			   char new_device[DEVICE_NAME_SIZE]);
};

class Rts2ScriptElementAcquire:public Rts2ScriptElement
{
private:
  double reqPrecision;
  double lastPrecision;
  float expTime;
  Rts2ConnImgProcess *processor;
  enum
  { NEED_IMAGE, WAITING_IMAGE, WAITING_ASTROMETRY, WAITING_MOVE, PRECISION_OK,
    PRECISION_BAD, FAILED
  } processingState;
  char defaultImgProccess[2000];
  int obsId;
  int imgId;
public:
    Rts2ScriptElementAcquire (Rts2Script * in_script, double in_precision,
			      float in_expTime);
  virtual void postEvent (Rts2Event * event);
  virtual int nextCommand (Rts2DevClientCamera * camera,
			   Rts2Command ** new_command,
			   char new_device[DEVICE_NAME_SIZE]);
  virtual int processImage (Rts2Image * image);
};

class Rts2ScriptElementWaitAcquire:public Rts2ScriptElement
{
public:
  Rts2ScriptElementWaitAcquire (Rts2Script * in_script);
  virtual int nextCommand (Rts2DevClientCamera * camera,
			   Rts2Command ** new_command,
			   char new_device[DEVICE_NAME_SIZE]);
};

#endif /* !__RTS2_SCRIPTELEMENT__ */
