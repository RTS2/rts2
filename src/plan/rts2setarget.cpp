#include "rts2setarget.h"

int
Rts2SETDisable::defnextCommand (Rts2DevClient * client,
				Rts2Command ** new_command,
				char new_device[DEVICE_NAME_SIZE])
{
  getTarget ()->setTargetEnabled (false);
  getTarget ()->save (true);
  return NEXT_COMMAND_NEXT;
}

int
Rts2SETTempDisable::defnextCommand (Rts2DevClient * client,
				    Rts2Command ** new_command,
				    char new_device[DEVICE_NAME_SIZE])
{
  time_t now;
  time (&now);
  now += seconds;
  getTarget ()->setNextObservable (&now);
  getTarget ()->save (true);
  return NEXT_COMMAND_NEXT;
}
