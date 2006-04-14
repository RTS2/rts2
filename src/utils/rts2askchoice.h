#ifndef __RTS2_ASKCHOICE__
#define __RTS2_ASKCHOICE__

#include "rts2app.h"

#include <list>
#include <ostream>

class Rts2Choice
{
private:
  char key;
  const char *desc;
public:
    Rts2Choice (char in_key, const char *in_desc)
  {
    key = in_key;
    desc = in_desc;
  }

  friend std::ostream & operator << (std::ostream & _os, Rts2Choice & choice);
};

std::ostream & operator << (std::ostream & _os, Rts2Choice & choice);

class Rts2AskChoice
{
private:
  Rts2App * app;
  std::list < Rts2Choice > choices;
public:
  Rts2AskChoice (Rts2App * in_app);
  virtual ~ Rts2AskChoice (void);

  void addChoice (char key, const char *desc);
  char query (std::ostream & _os);
};

#endif /* !__RTS2_ASKCHOICE__ */
