#ifndef __RTS2_DAEMONWINDOW__
#define __RTS2_DAEMONWINDOW__

extern "C"
{
#include <cdk/cdk.h>
}

#include "../utils/rts2conn.h"
#include "../utils/rts2devclient.h"

class Rts2DaemonWindow
{
public:
  Rts2DaemonWindow (CDKSCREEN * cdkscreen);
  virtual char *injectKey (int key) = 0;
  virtual void draw () = 0;
};

class Rts2DeviceWindow:public Rts2DaemonWindow
{
private:
  CDKALPHALIST * valueList;
  Rts2Conn *connection;
  void drawValuesList ();
  void drawValuesList (Rts2DevClient * client);
public:
    Rts2DeviceWindow (CDKSCREEN * cdkscreen, Rts2Conn * in_connection);
    virtual ~ Rts2DeviceWindow (void);
  virtual char *injectKey (int key);
  virtual void draw ();
};

#endif /*! __RTS2_DAEMONWINDOW__ */
