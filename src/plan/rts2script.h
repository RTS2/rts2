#ifndef __RTS2_SCRIPT__
#define __RTS2_SCRIPT__

#include "rts2scriptelement.h"

#include "../utils/rts2block.h"
#include "../utils/rts2command.h"
#include "../utils/rts2devclient.h"
#include "../writers/rts2image.h"

#include <list>

#define NEXT_COMMAND_NEXT	       -2
#define NEXT_COMMAND_END_SCRIPT        -1
#define NEXT_COMMAND_KEEP     		1
#define NEXT_COMMAND_WAITING  		2
#define NEXT_COMMAND_RESYNC		3
#define NEXT_COMMAND_CHECK_WAIT		4
#define NEXT_COMMAND_PRECISION_FAILED   5
// you should not return NEXT_COMMAND_PRECISION_OK on first nextCommand call
#define NEXT_COMMAND_PRECISION_OK	6
#define NEXT_COMMAND_WAIT_ACQUSITION    7
#define NEXT_COMMAND_ACQUSITION_IMAGE   8

#define NEXT_COMMAND_WAIT_SIGNAL	9
#define NEXT_COMMAND_WAIT_MIRROR	10
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

class Rts2Script:public Rts2Object
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
  Rts2Conn *connection;
  // is >= 0 when script runs, will become -1 when script is deleted (in beging of script destructor
  int executedCount;
public:
    Rts2Script (char *scriptText, Rts2Conn * in_connection);
    virtual ~ Rts2Script (void);
  virtual void postEvent (Rts2Event * event);
  int nextCommand (Rts2DevClientCamera * camera,
		   Rts2Command ** new_command,
		   char new_device[DEVICE_NAME_SIZE]);
  int nextCommand (Rts2DevClientPhot * phot,
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
  int processImage (Rts2Image * image);
  Rts2Block *getMaster ()
  {
    return connection->getMaster ();
  }
  int getExecutedCount ()
  {
    return executedCount;
  }
};

#endif /* ! __RTS2_SCRIPT__ */
