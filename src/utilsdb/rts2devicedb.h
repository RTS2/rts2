#ifndef __RTS2_DEVICEDB__
#define __RTS2_DEVICEDB__

#include "../utils/rts2device.h"
#include "../utils/rts2config.h"

class Rts2DeviceDb:public Rts2Device
{
  char *connectString;
  char *configFile;
protected:
    virtual int willConnect (Rts2Address * in_addr);
  virtual int processOption (int in_opt);
public:
    Rts2DeviceDb (int in_argc, char **in_argv, int in_device_type,
		  char *default_name);
    virtual ~ Rts2DeviceDb (void);
  int initDB ();
  virtual int init ();
  virtual void forkedInstance ();
};

#endif /* !__RTS2_DEVICEDB__ */
