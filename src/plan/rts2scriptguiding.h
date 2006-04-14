#ifndef __RTS2_SCRIPT_GUIDING__
#define __RTS2_SCRIPT_GUIDING__

#include "rts2scriptelement.h"

/**
 * Class for guiding.
 *
 * This class takes care of executing guiding script.
 * It works in similar fashion to acquireHAM class - read out image,
 * gets star list, find the brightest star in field, and center on it.
 * You are free to specify start exposure time.
 *
 * @author petr
 */
class Rts2ScriptElementGuiding:public Rts2ScriptElement
{
private:
  float expTime;
  int endSignal;

  int obsId;
  int imgId;

  char defaultImgProccess[2000];
  enum
  { NO_IMAGE, NEED_IMAGE, WAITING_IMAGE, FAILED
  } processingState;

  // will become -1 in case guiding goes other way then we wanted
  double ra_mult;
  double dec_mult;

  double last_ra;
  double last_dec;

  double min_change;
  double bad_change;

  // this will check sign..
  void checkGuidingSign (double &last, double &mult, double act);
public:
    Rts2ScriptElementGuiding (Rts2Script * in_script, float init_exposure,
			      int in_endSignal);
    virtual ~ Rts2ScriptElementGuiding (void);

  virtual void postEvent (Rts2Event * event);

  virtual int nextCommand (Rts2DevClientCamera * client,
			   Rts2Command ** new_command,
			   char new_device[DEVICE_NAME_SIZE]);

  virtual int processImage (Rts2Image * image);
  virtual int waitForSignal (int in_sig);
  virtual void cancelCommands ();
};

#endif /* !__RTS2_SCRIPT_GUIDING__ */
