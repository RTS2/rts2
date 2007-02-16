#include "sensord.h"

#include <gpib/ib.h>

class Rts2DevSensorCryocon:public Rts2DevSensor
{
private:

public:
  Rts2DevSensorCryocon (int argc, char **argv);
};

Rts2DevSensorCryocon::Rts2DevSensorCryocon (int in_argc, char **in_argv):
Rts2DevSensor (in_argc, in_argv)
{

}

int
main (int argc, char **argv)
{
  Rts2DevSensorCryocon device = Rts2DevSensorCryocon (argc, argv);
  return device.run ();
}
