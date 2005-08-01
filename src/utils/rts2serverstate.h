#ifndef __RTS2_SERVER_STATE__
#define __RTS2_SERVER_STATE__

#include <string.h>

class Rts2ServerState
{
private:
  int value;
  time_t lastUpdate;
public:
  char *name;
    Rts2ServerState (char *in_name)
  {
    name = new char[strlen (in_name) + 1];
      strcpy (name, in_name);
  }
   ~Rts2ServerState (void)
  {
    delete[]name;
  }
  int isName (const char *in_name)
  {
    return !strcmp (name, in_name);
  }
  const char *getName ()
  {
    return name;
  }
  void setValue (int new_value)
  {
    time (&lastUpdate);
    value = new_value;
  }
  int getValue ()
  {
    return value;
  }
};

#endif /* !__RTS2_SERVER_STATE__ */
