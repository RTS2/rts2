#ifndef __RTS2_DEVICEDB__
#define __RTS2_DEVICEDB__

#include "../utils/rts2device.h"
#include "../utils/rts2config.h"

class Rts2DeviceDb:public Rts2Device
{
  char *connectString;
  char *configFile;
public:
    Rts2DeviceDb (int in_argc, char **in_argv, int in_device_type,
		  int default_port, char *default_name);
    virtual ~ Rts2DeviceDb (void);
  virtual int processOption (int in_opt);
  virtual int init ();
};

#endif /* !__RTS2_DEVICEDB__ */
