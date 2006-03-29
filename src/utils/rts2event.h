#ifndef __RTS2_EVENT__
#define __RTS2_EVENT__

#define EVENT_SET_TARGET_ID	1
#define EVENT_WRITE_TO_IMAGE	2
#define EVENT_SET_TARGET	3
#define EVENT_OBSERVE           4
#define EVENT_IMAGE_OK		5

#define EVENT_QUERY_WAIT	6
#define EVENT_ENTER_WAIT	7
#define EVENT_CLEAR_WAIT	8

#define EVENT_GET_RADEC		10
#define EVENT_MOUNT_CHANGE	11
// events number below that number shoudl be considered RTS2-reserved
#define RTS2_LOCAL_EVENT   1000

// local events are defined in local headers!
// so far those offeset ranges (from RTS2_LOCAL_EVENT) are reserved for:
//
// rts2execcli.h            50- 99
// rts2connimgprocess.h    200-249
// rts2scriptelement.h     250-299
// rts2devclifoc.h         500-549
// rts2devclicop.h         550-599
// grbd.h                  600-649
// rts2devcliwheel.h       650-699
// augershooter.h          700-749
// rts2devclifocuser.h     750-799

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
  void setArg (void *in_arg)
  {
    arg = in_arg;
  }
};

#endif /*! __RTS2_EVENT__ */
