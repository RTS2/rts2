#include "rts2askchoice.h"

std::ostream & operator << (std::ostream & _os, Rts2Choice & choice)
{
  _os << "  " << choice.key << " .. " << choice.desc << std::endl;
  return _os;
}

Rts2AskChoice::Rts2AskChoice (Rts2App * in_app)
{
  app = in_app;
}

Rts2AskChoice::~Rts2AskChoice (void)
{
  choices.clear ();
}

void
Rts2AskChoice::addChoice (char key, const char *desc)
{
  choices.push_back (Rts2Choice (key, desc));
}

char
Rts2AskChoice::query (std::ostream & _os)
{
  char selection;
  for (std::list < Rts2Choice >::iterator iter = choices.begin ();
       iter != choices.end (); iter++)
    {
      _os << (*iter);
    }
  app->askForChr ("Your selection?", selection);
  return selection;
}
