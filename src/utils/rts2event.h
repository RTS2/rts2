#ifndef __RTS2_EVENT__
#define __RTS2_EVENT__

#define EVENT_SET_TARGET_ID	1
#define EVENT_WRITE_TO_IMAGE	2
#define EVENT_SET_TARGET	3
#define EVENT_OBSERVE           4
#define EVENT_IMAGE_OK		5

// events number below that number shoudl be considered RTS2-reserved
#define RTS2_LOCAL_EVENT   1000

class Rts2Event
{
private:
  int type;
  void *arg;
public:
    Rts2Event (int in_type)
  {
    type = in_type;
    arg = 0;
  }
  Rts2Event (int in_type, void *in_arg)
  {
    type = in_type;
    arg = in_arg;
  }
  Rts2Event (Rts2Event * event)
  {
    type = event->getType ();
    arg = event->getArg ();
  }

  int getType ()
  {
    return type;
  }
  void *getArg ()
  {
    return arg;
  }
};

#endif /*! __RTS2_EVENT__ */
