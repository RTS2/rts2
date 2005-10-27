#include "libnova_cpp.h"

#include <iomanip>

std::ostream & operator << (std::ostream & _os, LibnovaRa l_ra)
{
  struct ln_hms ra_hms;
  ln_deg_to_hms (l_ra.ra, &ra_hms);
  char old_fill = _os.fill ('0');
  _os << std::setw (2) << ra_hms.hours << ":"
    << std::setw (2) << ra_hms.minutes << ":"
    << std::setw (5) << ra_hms.seconds;
  _os.fill (old_fill);
  return _os;
}

std::ostream & operator << (std::ostream & _os, LibnovaDeg l_deg)
{
  struct ln_dms deg_dms;
  ln_deg_to_dms (l_deg.deg, &deg_dms);
  char old_fill = _os.fill ('0');
  _os << (deg_dms.neg ? '-' : '+')
    << std::setw (3) << deg_dms.degrees << ":"
    << std::setw (2) << deg_dms.minutes << ":"
    << std::setw (5) << deg_dms.seconds;
  _os.fill (old_fill);
  return _os;
}

std::ostream & operator << (std::ostream & _os, LibnovaDeg90 l_deg)
{
  struct ln_dms deg_dms;
  ln_deg_to_dms (l_deg.deg, &deg_dms);
  char old_fill = _os.fill ('0');
  _os << (deg_dms.neg ? '-' : '+')
    << std::setw (2) << deg_dms.degrees << ":"
    << std::setw (2) << deg_dms.minutes << ":"
    << std::setw (5) << deg_dms.seconds;
  _os.fill (old_fill);
  return _os;
}
