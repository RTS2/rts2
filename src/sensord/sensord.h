#ifndef __RTS2_CRYOCON__
#define __RTS2_CRYOCON__

#include "../utils/rts2device.h"

class Rts2DevSensor:public Rts2Device
{
public:
  Rts2DevSensor (int argc, char **argv);
    virtual ~ Rts2DevSensor (void);
};

#endif /* !__RTS2_CRYOCON__ */
