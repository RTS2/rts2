#ifndef __RTS2_SCRIPT__
#define __RTS2_SCRIPT__

#include "../utils/rts2block.h"
#include "../utils/rts2command.h"
#include "../utils/rts2devclient.h"

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
public:
    Rts2Script (char *scriptText,
		const char in_defaultDevice[DEVICE_NAME_SIZE]);
    virtual ~ Rts2Script (void);
  int nextCommand (Rts2Block * in_master, Rts2DevClientCamera * camera,
		   Rts2Command ** new_command,
		   char new_device[DEVICE_NAME_SIZE]);
  int isLastCommand (void)
  {
    return !(*cmdBufTop);
  }
};

#endif /* ! __RTS2_SCRIPT__ */
