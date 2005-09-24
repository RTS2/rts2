#ifndef __RTS2_SCRIPTELEMENT__
#define __RTS2_SCRIPTELEMENT__

#include "rts2script.h"
#include "rts2connimgprocess.h"
#include "rts2spiral.h"
#include "../writers/rts2image.h"
#include "../utils/rts2object.h"
#include "../utils/rts2block.h"

#include "status.h"

#define EVENT_PRECISION_REACHED		RTS2_LOCAL_EVENT + 250

#define EVENT_MIRROR_SET		RTS2_LOCAL_EVENT + 251
#define EVENT_MIRROR_FINISH		RTS2_LOCAL_EVENT + 252

#define EVENT_ACQUIRE_START		RTS2_LOCAL_EVENT + 253
#define EVENT_ACQUIRE_WAIT		RTS2_LOCAL_EVENT + 254
#define EVENT_ACQUIRE_QUERY		RTS2_LOCAL_EVENT + 255

// send some signal to other device..so they will
// know that something is going on
#define EVENT_SIGNAL			RTS2_LOCAL_EVENT + 256

#define EVENT_SIGNAL_QUERY		RTS2_LOCAL_EVENT + 257

// send when data we received 
#define EVENT_HAM_DATA			RTS2_LOCAL_EVENT + 258

#define EVENT_ADD_FIXED_OFFSET		RTS2_LOCAL_EVENT + 259

#define EVENT_TEL_SEARCH_START		RTS2_LOCAL_EVENT + 260
#define EVENT_TEL_SEARCH_STOP		RTS2_LOCAL_EVENT + 261
#define EVENT_TEL_SEARCH_END		RTS2_LOCAL_EVENT + 262
// successfull search
#define EVENT_TEL_SEARCH_SUCCESS	RTS2_LOCAL_EVENT + 263

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
  virtual int defnextCommand (Rts2DevClient * client,
			      Rts2Command ** new_command,
			      char new_device[DEVICE_NAME_SIZE]);
  virtual int nextCommand (Rts2DevClientCamera * camera,
			   Rts2Command ** new_command,
			   char new_device[DEVICE_NAME_SIZE]);
  virtual int nextCommand (Rts2DevClientPhot * phot,
			   Rts2Command ** new_command,
			   char new_device[DEVICE_NAME_SIZE]);
  virtual int processImage (Rts2Image * image);
  /**
   * Returns 1 if we are waiting for that signal.
   * Signal is > 0
   */
  virtual int waitForSignal (int in_sig)
  {
    return 0;
  }
  /**
   * That method will be called when we currently run that
   * command and we would like to cancel observation.
   *
   * Should be used in children when appopriate.
   */
  virtual void cancelCommands ()
  {
  }
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
  virtual int defnextCommand (Rts2DevClient * client,
			      Rts2Command ** new_command,
			      char new_device[DEVICE_NAME_SIZE]);
};

class Rts2ScriptElementWait:public Rts2ScriptElement
{
public:
  Rts2ScriptElementWait (Rts2Script * in_script);
  virtual int defnextCommand (Rts2DevClient * client,
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
  virtual int nextCommand (Rts2DevClientPhot * phot,
			   Rts2Command ** new_command,
			   char new_device[DEVICE_NAME_SIZE]);
};

class Rts2ScriptElementAcquire:public Rts2ScriptElement
{
protected:
  double reqPrecision;
  double lastPrecision;
  float expTime;
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
  virtual void cancelCommands ();
};

class Rts2ScriptElementWaitAcquire:public Rts2ScriptElement
{
public:
  Rts2ScriptElementWaitAcquire (Rts2Script * in_script);
  virtual int defnextCommand (Rts2DevClient * client,
			      Rts2Command ** new_command,
			      char new_device[DEVICE_NAME_SIZE]);
};

class Rts2ScriptElementMirror:public Rts2ScriptElement
{
private:
  int mirror_pos;
  char *mirror_name;
public:
    Rts2ScriptElementMirror (Rts2Script * in_script, char *in_mirror_name,
			     int in_mirror_pos);
    virtual ~ Rts2ScriptElementMirror (void);
  virtual void postEvent (Rts2Event * event);
  virtual int defnextCommand (Rts2DevClient * client,
			      Rts2Command ** new_command,
			      char new_device[DEVICE_NAME_SIZE]);
  void takeJob ()
  {
    mirror_pos = -1;
  }
  int getMirrorPos ()
  {
    return mirror_pos;
  }
  int isMirrorName (const char *in_name)
  {
    return !strcmp (mirror_name, in_name);
  }
};

class Rts2ScriptElementPhotometer:public Rts2ScriptElement
{
private:
  int filter;
  float exposure;
  int count;
public:
    Rts2ScriptElementPhotometer (Rts2Script * in_script, int in_filter,
				 float in_exposure, int in_count);
  virtual int nextCommand (Rts2DevClientPhot * client,
			   Rts2Command ** new_command,
			   char new_device[DEVICE_NAME_SIZE]);
};

class Rts2ScriptElementSendSignal:public Rts2ScriptElement
{
private:
  int sig;
  bool askedFor;
public:
    Rts2ScriptElementSendSignal (Rts2Script * in_script, int in_sig);
    virtual ~ Rts2ScriptElementSendSignal (void);
  virtual void postEvent (Rts2Event * event);
  virtual int defnextCommand (Rts2DevClient * client,
			      Rts2Command ** new_command,
			      char new_device[DEVICE_NAME_SIZE]);
};

class Rts2ScriptElementWaitSignal:public Rts2ScriptElement
{
private:
  int sig;
public:
  // in_sig must be > 0
    Rts2ScriptElementWaitSignal (Rts2Script * in_script, int in_sig);
  virtual int defnextCommand (Rts2DevClient * client,
			      Rts2Command ** new_command,
			      char new_device[DEVICE_NAME_SIZE]);
  virtual int waitForSignal (int in_sig);
};

/**
 * Some special handling for HAM..
 */
class Rts2ScriptElementAcquireHam:public Rts2ScriptElementAcquire
{
private:
  int maxRetries;
  int retries;
  Rts2Spiral *spiral;
public:
    Rts2ScriptElementAcquireHam (Rts2Script * in_script, int in_maxRetries,
				 float in_expTime);
    virtual ~ Rts2ScriptElementAcquireHam (void);
  virtual void postEvent (Rts2Event * event);
  virtual int processImage (Rts2Image * image);
};

/**
 * Photometer based search of stars
 */
class Rts2ScriptElementSearch:public Rts2ScriptElement
{
private:
  double searchRadius;
  double searchSpeed;
  enum
  { NEED_SEARCH, SEARCHING, SEARCH_OK, SEARCH_FAILED,
    SEARCH_FAILED2
  } processingState;
public:
    Rts2ScriptElementSearch (Rts2Script * in_script, double in_searchRadius,
			     double in_searchSpeed);
  virtual void postEvent (Rts2Event * event);
  virtual int nextCommand (Rts2DevClientPhot * phot,
			   Rts2Command ** new_command,
			   char new_device[DEVICE_NAME_SIZE]);
  double getSearchRadius ()
  {
    return searchRadius;
  }
  double getSearchSpeed ()
  {
    return searchSpeed;
  }
  void getJob ()
  {
    searchRadius = nan ("f");
  }
};

#endif /* !__RTS2_SCRIPTELEMENT__ */
