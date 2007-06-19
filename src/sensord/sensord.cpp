#include "sensord.h"

Rts2DevSensor::Rts2DevSensor (int in_argc, char **in_argv):
Rts2Device (in_argc, in_argv, DEVICE_TYPE_SENSOR, "S1")
{
  setIdleInfoInterval (5);
}

Rts2DevSensor::~Rts2DevSensor (void)
{
}
