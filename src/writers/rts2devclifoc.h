#ifndef __RTS2_DEVCLIFOC__
#define __RTS2_DEVCLIFOC__

#include "rts2devcliimg.h"
#include "../utils/rts2connfork.h"

#define EVENT_START_FOCUSING	RTS2_LOCAL_EVENT + 500
#define EVENT_FOCUSING_END	RTS2_LOCAL_EVENT + 501
#define EVENT_CHANGE_FOCUS	RTS2_LOCAL_EVENT + 502

class Rts2ConnFocus;

class Rts2DevClientCameraFoc:public Rts2DevClientCameraImage
{
private:
  int isFocusing;
  Rts2Image *darkImage;
  Rts2ConnFocus *focConn;
protected:
  char *exe;
  int autoDark;

  virtual void queExposure ();
public:
    Rts2DevClientCameraFoc (Rts2Conn * in_connection, const char *in_exe);
    virtual ~ Rts2DevClientCameraFoc (void);
  virtual void postEvent (Rts2Event * event);
  virtual void processImage (Rts2Image * image);
  // will cause camera to change focus by given steps BEFORE exposition
  // when change == INT_MAX, focusing don't converge
  virtual void focusChange (Rts2Conn * focus, Rts2ConnFocus * focConn);
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
private:
  char *img_path;
  Rts2Image *image;
  int change;
  int endEvent;
public:
    Rts2ConnFocus (Rts2Block * in_master, Rts2Image * in_image,
		   const char *in_exe, int in_endEvent);
    virtual ~ Rts2ConnFocus (void);
  virtual int newProcess ();
  virtual int processLine ();
  int getChange ()
  {
    return change;
  }
  void setChange (int new_change)
  {
    change = new_change;
  }
  const char *getCameraName ()
  {
    return image->getCameraName ();
  }
  Rts2Image *getImage ()
  {
    return image;
  }
  void nullCamera ()
  {
    image = NULL;
  }
};

class Rts2DevClientPhotFoc:public Rts2DevClientPhot
{
protected:
  virtual void addCount (int count, float exp, int is_ov);
public:
    Rts2DevClientPhotFoc (Rts2Conn * in_conn);
};

#endif /* !CLIFOC__ */
