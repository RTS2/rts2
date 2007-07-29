#include "rts2displayvalue.h"
#include <sstream>

std::string
getDisplayValue (Rts2Value * value)
{
  std::ostringstream _os;
  const char *tmp_val;
  switch (value->getValueDisplayType ())
    {
    case RTS2_DT_RA:
      _os << LibnovaRa (value->getValueDouble ());
      break;
    case RTS2_DT_DEC:
      _os << LibnovaDeg90 (value->getValueDouble ());
      break;
    case RTS2_DT_DEGREES:
      _os << LibnovaDeg (value->getValueDouble ());
      break;
    case RTS2_DT_DEG_DIST:
      _os << LibnovaDegDist (value->getValueDouble ());
      break;
    default:
      tmp_val = value->getDisplayValue ();
      if (tmp_val)
	return std::string (tmp_val);
      return std::string ("");
    }
  return _os.str ();
}
