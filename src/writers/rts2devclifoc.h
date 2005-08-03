#ifndef __RTS2_DEVCLIFOC__
#define __RTS2_DEVCLIFOC__

#include "rts2devcliimg.h"
#include "../utils/rts2connfork.h"

class Rts2DevClientCameraFoc:public Rts2DevClientCameraImage
{
protected:
  char *exe;
public:
    Rts2DevClientCameraFoc (Rts2Conn * in_connection, const char *in_exe);
    virtual ~ Rts2DevClientCameraFoc (void);
  virtual void processImage (Rts2Image * image);
};

class Rts2ConnFocus:public Rts2ConnFork
{
  char *img_path;
public:
    Rts2ConnFocus (Rts2DevClientCameraFoc * in_camera, Rts2Image * in_image,
		   const char *in_exe);
    virtual ~ Rts2ConnFocus (void);
  virtual int newProcess ();
  virtual int processLine ();
};

#endif /* !CLIFOC__ */
