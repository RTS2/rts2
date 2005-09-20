#ifndef __RTS2_SCRIPT_BLOCK__
#define __RTS2_SCRIPT_BLOCK__

#include "rts2scriptelement.h"

/**
 * Block - text surrounded by {}.
 *
 * @author petr
 */
class Rts2ScriptElementBlock:public Rts2ScriptElement
{
private:
  std::list < Rts2ScriptElement * >blockElements;
  Rts2ScriptElement *currScriptElement;
public:
    Rts2ScriptElementBlock (Rts2Script * in_script);
    virtual ~ Rts2ScriptElementBlock (void);

  virtual void postEvent (Rts2Event * event);

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

  virtual int waitForSignal (int in_sig);

  virtual void cancelCommands ();
};

#endif /* !__RTS2_SCRIPT_BLOCK__ */
