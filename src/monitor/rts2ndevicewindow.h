#ifndef __RTS2_NDEVICEWINDOW__
#define __RTS2_NDEVICEWINDOW__

#include "rts2daemonwindow.h"
#include "rts2nvaluebox.h"

class Rts2NDeviceWindow:public Rts2NSelWindow
{
private:
  WINDOW * valueList;
  Rts2Conn *connection;
  void printState ();
  void drawValuesList ();
  void drawValuesList (Rts2DevClient * client);
  Rts2Value *getSelValue ();
  void printValueDesc (Rts2Value * val);
  void endValueBox ();
  void createValueBox ();
  Rts2NValueBox *valueBox;
public:
    Rts2NDeviceWindow (Rts2Conn * in_connection);
    virtual ~ Rts2NDeviceWindow (void);

  virtual keyRet injectKey (int key);
  virtual void draw ();
  virtual bool setCursor ();
};

#endif /* ! __RTS2_NDEVICEWINDOW__ */
