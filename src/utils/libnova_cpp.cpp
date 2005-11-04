#include "libnova_cpp.h"

#include <math.h>
#include <iomanip>

std::ostream & operator << (std::ostream & _os, LibnovaRa l_ra)
{
  if (isnan (l_ra.ra))
    {
      _os << std::setw (11) << "nan";
      return _os;
    }
  struct ln_hms ra_hms;
  ln_deg_to_hms (l_ra.ra, &ra_hms);
  char old_fill = _os.fill ('0');
  int old_precison = _os.precision (2);
  _os << std::setw (2) << ra_hms.hours << ":"
    << std::setw (2) << ra_hms.minutes << ":"
    << std::setw (5) << ra_hms.seconds;
  _os.precision (old_precison);
  _os.fill (old_fill);
  return _os;
}

std::ostream & operator << (std::ostream & _os, LibnovaDeg l_deg)
{
  if (isnan (l_deg.deg))
    {
      _os << std::setw (13) << "nan";
      return _os;
    }
  struct ln_dms deg_dms;
  ln_deg_to_dms (l_deg.deg, &deg_dms);
  char old_fill = _os.fill ('0');
  int old_precison = _os.precision (2);
  _os << (deg_dms.neg ? '-' : '+')
    << std::setw (3) << deg_dms.degrees << "o"
    << std::setw (2) << deg_dms.minutes << "'"
    << std::setw (5) << deg_dms.seconds;
  _os.precision (old_precison);
  _os.fill (old_fill);
  return _os;
}

std::ostream & operator << (std::ostream & _os, LibnovaDeg90 l_deg)
{
  if (isnan (l_deg.deg))
    {
      _os << std::setw (12) << "nan";
      return _os;
    }
  struct ln_dms deg_dms;
  ln_deg_to_dms (l_deg.deg, &deg_dms);
  char old_fill = _os.fill ('0');
  int old_precison = _os.precision (2);
  _os << (deg_dms.neg ? '-' : '+')
    << std::setw (2) << deg_dms.degrees << "o"
    << std::setw (2) << deg_dms.minutes << "'"
    << std::setw (5) << deg_dms.seconds;
  _os.precision (old_precison);
  _os.fill (old_fill);
  return _os;
}

std::ostream & operator << (std::ostream & _os, LibnovaDegArcMin l_deg)
{
  if (isnan (l_deg.deg))
    {
      _os << std::setw (11) << "nan";
      return _os;
    }
  struct ln_dms deg_dms;
  ln_deg_to_dms (l_deg.deg, &deg_dms);
  int old_precison = _os.precision (2);
  if (deg_dms.degrees == 0 && deg_dms.minutes == 0)
    {
      _os << "   " << (deg_dms.neg ? '-' : '+') << "0'";
    }
  else
    {
      std::ios_base::fmtflags old_settings = _os.flags ();
      _os.setf (std::ios_base::fixed | std::ios_base::showpos,
		std::ios_base::floatfield);
      _os << std::setw (5) << ((deg_dms.neg ? -1 : 1) *
			       (deg_dms.degrees * 60 +
				deg_dms.minutes)) << "'";
      _os.setf (old_settings);
    }
  char old_fill = _os.fill ('0');
  _os << std::setw (5) << deg_dms.seconds;
  _os.precision (old_precison);
  _os.fill (old_fill);
  return _os;
}
