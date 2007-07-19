#include "rts2swaitfor.h"

void
Rts2SWaitFor::getDevice (char new_device[DEVICE_NAME_SIZE])
{
  strncpy (new_device, deviceName.c_str (), DEVICE_NAME_SIZE);
}

Rts2SWaitFor::Rts2SWaitFor (Rts2Script * in_script, const char *new_device,
			    char *in_valueName, double in_tarval,
			    double in_range):
Rts2ScriptElement (in_script)
{
  valueName = std::string (in_valueName);
  deviceName = std::string (new_device);
  // if value contain device..
  tarval = in_tarval;
  range = in_range;
  setIdleTimeout (1);
}

int
Rts2SWaitFor::defnextCommand (Rts2DevClient * client,
			      Rts2Command ** new_command,
			      char new_device[DEVICE_NAME_SIZE])
{
  return idle ();
}

int
Rts2SWaitFor::idle ()
{
  Rts2Value *val =
    script->getMaster ()->getValue (deviceName.c_str (), valueName.c_str ());
  if (!val)
    {
      logStream (MESSAGE_ERROR) << "Cannot get value " << deviceName << "." <<
	valueName << sendLog;
      return NEXT_COMMAND_NEXT;
    }
  if (fabs (val->getValueDouble () - tarval) <= range)
    {
      return NEXT_COMMAND_NEXT;
    }
  return Rts2ScriptElement::idle ();
}
