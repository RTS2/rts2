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
  std::list < Rts2ScriptElement * >::iterator curr_element;
  int blockScriptRet (int ret);
protected:
    virtual bool endLoop ()
  {
    return true;
  }
public:
    Rts2ScriptElementBlock (Rts2Script * in_script);
  virtual ~ Rts2ScriptElementBlock (void);

  virtual void addElement (Rts2ScriptElement * element);

  virtual void postEvent (Rts2Event * event);

  virtual int defnextCommand (Rts2DevClient * client,
			      Rts2Command ** new_command,
			      char new_device[DEVICE_NAME_SIZE]);

  virtual int nextCommand (Rts2DevClientCamera * client,
			   Rts2Command ** new_command,
			   char new_device[DEVICE_NAME_SIZE]);

  virtual int nextCommand (Rts2DevClientPhot * client,
			   Rts2Command ** new_command,
			   char new_device[DEVICE_NAME_SIZE]);

  virtual int processImage (Rts2Image * image);
  virtual int waitForSignal (int in_sig);
  virtual void cancelCommands ();
};

// will wait for signal, after signal we will end execution of that block
class Rts2SEBSignalEnd:public Rts2ScriptElementBlock
{
private:
  // sig_num will become -1, when we get signal..
  int sig_num;
protected:
    virtual bool endLoop ()
  {
    return (sig_num == -1);
  }
public:
    Rts2SEBSignalEnd (Rts2Script * in_script, int end_sig_num);
  virtual ~ Rts2SEBSignalEnd (void);

  virtual int waitForSignal (int in_sig);
};

#endif /* !__RTS2_SCRIPT_BLOCK__ */
