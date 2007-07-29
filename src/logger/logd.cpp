#include "rts2loggerbase.h"
#include "../utils/rts2device.h"

class Rts2Logd:public Rts2Device, public Rts2LoggerBase
{
public:
  Rts2Logd (int in_argc, char **in_argv);
};

Rts2Logd::Rts2Logd (int in_argc, char **in_argv):
Rts2Device (in_argc, in_argv, DEVICE_TYPE_LOGD, "LOGD")
{
}

int
main (int argc, char **argv)
{
  Rts2Logd logd = Rts2Logd (argc, argv);
  return logd.run ();
}
