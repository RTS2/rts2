#include <sstream>
#include "status.h"

#include "rts2centralstate.h"

const char *
Rts2CentralState::getStringShort ()
{
  switch (getValue () & SERVERD_STATUS_MASK)
    {
    case SERVERD_DAY:
      return "day";
      break;
    case SERVERD_EVENING:
      return "evening";
    case SERVERD_DUSK:
      return "dusk";
    case SERVERD_NIGHT:
      return "night";
    case SERVERD_DAWN:
      return "dawn";
    case SERVERD_MORNING:
      return "morning";
    }
  return "unknow";
}

std::string Rts2CentralState::getString ()
{
  std::ostringstream os;
  if (getValue () == SERVERD_OFF)
    {
      os << "OFF";
      return os.str ();
    }
  if ((getValue () & SERVERD_STANDBY_MASK) == SERVERD_STANDBY)
    {
      os << "standby ";
    }
  else
    {
      os << "ready ";
    }
  os << getStringShort ();
  return os.str ();
}


std::ostream & operator << (std::ostream & _os, Rts2CentralState c_state)
{
  _os << c_state.getStringShort ();
  return _os;
}
