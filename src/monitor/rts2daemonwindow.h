#ifndef __RTS2_DAEMONWINDOW__
#define __RTS2_DAEMONWINDOW__

extern "C"
{
#include <cdk/cdk.h>
}

class Rts2DaemonWindow
{
public:
  Rts2DaemonWindow (CDKSCREEN * cdkscreen);
  virtual int injectKey (int key) = 0;
};

class Rts2DeviceWindow:public Rts2DaemonWindow
{
private:
  CDKALPHALIST * valueList;
public:
  Rts2DaemonWindow (CDKSCREEN * cdkscreen);
  virtual int injectKey (int key);
};

#endif /*! __RTS2_DAEMONWINDOW__ */
