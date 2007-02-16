#include "sensord.h"

class Rts2DevSensorDummy:public Rts2DevSensor
{
public:
  Rts2DevSensorDummy (int in_argc, char **in_argv):Rts2DevSensor (in_argc,
								  in_argv)
  {
  }
  virtual int ready ()
  {
    return -1;
  }
};

int
main (int argc, char **argv)
{
  Rts2DevSensorDummy device = Rts2DevSensorDummy (argc, argv);
  return device.run ();
}
