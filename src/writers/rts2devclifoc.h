#ifndef __RTS2_DEVCLIFOC__
#define __RTS2_DEVCLIFOC__

#include "rts2devcliimg.h"
#include "../utils/rts2connfork.h"

class Rts2DevClientCameraFoc:public Rts2DevClientCameraImage
{
public:
  Rts2DevClientCameraFoc (Rts2Conn * in_connection);
  virtual void processImage (Rts2Image * image);
};

class Rts2ConnFocus:public Rts2ConnFork
{
  char *img_path;
public:
    Rts2ConnFocus (Rts2DevClientCameraFoc * in_camera, Rts2Image * in_image);
    virtual ~ Rts2ConnFocus (void);
  virtual int newProcess ();
  virtual int processLine ();
};

#endif /* !CLIFOC__ */
