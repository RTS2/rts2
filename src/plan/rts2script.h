#ifndef __RTS2_SCRIPT__
#define __RTS2_SCRIPT__

#include "rts2scriptelement.h"

#include "../utils/rts2block.h"
#include "../utils/rts2command.h"
#include "../utils/rts2devclient.h"

#include <list>

/*!
 * Holds script to execute on given device.
 * Script might include commands to other devices; in such case device
 * name must be given in script, separated from command with .
 * (point).
 *
 * It should be extended in future to enable actions depending on
 * results of observations etc..now we have only small core to build
 * on.
 *
 * @author Petr Kubanek <petr@lascaux.asu.cas.cz>
 */

class Rts2ScriptElement;

class Rts2Script
{
private:
  char *cmdBuf;
  char *cmdBufTop;
  char defaultDevice[DEVICE_NAME_SIZE];

  char *nextElement ();
  int getNextParamFloat (float *val);
  int getNextParamDouble (double *val);
  int getNextParamInteger (int *val);
  Rts2ScriptElement *parseBuf ();
    std::list < Rts2ScriptElement * >elements;
public:
    Rts2Script (char *scriptText,
		const char in_defaultDevice[DEVICE_NAME_SIZE]);
    virtual ~ Rts2Script (void);
  int nextCommand (Rts2Block * in_master, Rts2DevClientCamera * camera,
		   Rts2Command ** new_command,
		   char new_device[DEVICE_NAME_SIZE]);
  int isLastCommand (void)
  {
    return (elements.size () == 0) ? 1 : 0;
  }
  void getDefaultDevice (char new_device[DEVICE_NAME_SIZE])
  {
    strncpy (new_device, defaultDevice, DEVICE_NAME_SIZE);
  }
};

#endif /* ! __RTS2_SCRIPT__ */
