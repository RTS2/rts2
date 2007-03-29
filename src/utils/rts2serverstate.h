#ifndef __RTS2_SERVER_STATE__
#define __RTS2_SERVER_STATE__

#include <string.h>
#include <status.h>

class Rts2ServerState
{
private:
  int value;
  int oldValue;
  time_t lastUpdate;
public:
    Rts2ServerState ()
  {
    lastUpdate = 0;
    oldValue = 0;
    value = DEVICE_NOT_READY;
  }
  virtual ~ Rts2ServerState (void)
  {
  }
  void setValue (int new_value)
  {
    time (&lastUpdate);
    oldValue = value;
    value = new_value;
  }
  int getValue ()
  {
    return value;
  }
  int getOldValue ()
  {
    return oldValue;
  }
  bool maskValueChanged (int q_mask)
  {
    return (getValue () & q_mask) != (getOldValue () & q_mask);
  }
};

#endif /* !__RTS2_SERVER_STATE__ */
