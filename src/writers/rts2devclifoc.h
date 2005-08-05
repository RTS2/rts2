#ifndef __RTS2_DEVCLIFOC__
#define __RTS2_DEVCLIFOC__

#include "rts2devcliimg.h"
#include "../utils/rts2connfork.h"

#define EVENT_START_FOCUSING	RTS2_LOCAL_EVENT + 500
#define EVENT_FOCUSING_END	RTS2_LOCAL_EVENT + 501

class Rts2DevClientCameraFoc:public Rts2DevClientCameraImage
{
private:
  int isFocusing;
protected:
  char *exe;

  virtual void queExposure ();
public:
    Rts2DevClientCameraFoc (Rts2Conn * in_connection, const char *in_exe);
    virtual ~ Rts2DevClientCameraFoc (void);
  virtual void postEvent (Rts2Event * event);
  virtual void processImage (Rts2Image * image);
  // will cause camera to change focus by given steps BEFORE exposition
  void changeFocus (int steps);
};

class Rts2DevClientFocusFoc:public Rts2DevClientFocusImage
{
protected:
  virtual void focusingEnd ();
public:
    Rts2DevClientFocusFoc (Rts2Conn * in_connection);
  virtual void postEvent (Rts2Event * event);
};

class Rts2ConnFocus:public Rts2ConnFork
{
  char *img_path;
  Rts2DevClientCameraFoc *camera;
public:
    Rts2ConnFocus (Rts2DevClientCameraFoc * in_camera, Rts2Image * in_image,
		   const char *in_exe);
    virtual ~ Rts2ConnFocus (void);
  virtual int newProcess ();
  virtual int processLine ();
};

#endif /* !CLIFOC__ */
