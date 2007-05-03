#include "rts2scriptdevice.h"

Rts2ScriptDevice::Rts2ScriptDevice (int in_argc, char **in_argv,
				    int in_device_type, char *default_name):
Rts2Device (in_argc, in_argv, in_device_type, default_name)
{
  createValue (scriptRepCount, "SCRIPREP", "script loop count", true, 0, 0,
	       true);
  scriptRepCount->setValueInteger (0);
  createValue (runningScript, "SCRIPT", "script used to take this images",
	       false, 0, 0, true);
  runningScript->setValueString ("");
}

int
Rts2ScriptDevice::setValue (Rts2Value * old_value, Rts2Value * new_value)
{
  if (old_value == scriptRepCount)
    return 0;
  if (old_value == runningScript)
    return 0;
  return Rts2Device::setValue (old_value, new_value);
}
