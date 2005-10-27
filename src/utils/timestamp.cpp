#include "timestamp.h"
#include "../utils/rts2block.h"

#include <iomanip>
#include <math.h>
#include <time.h>
#include <sys/time.h>

std::ostream & operator << (std::ostream & _os, Timestamp _ts)
{
  // convert timestamp to timeval
  static struct timeval tv;
  struct tm *gmt;
  tv.tv_sec = (long) _ts.ts;
  tv.tv_usec = (long) ((_ts.ts - floor (_ts.ts)) * USEC_SEC);
  gmt = gmtime (&tv.tv_sec);

  std::ios_base::fmtflags old_settings = _os.flags ();
  _os.setf (std::ios_base::fixed, std::ios_base::floatfield);
  int old_precision = _os.precision (2);
  char old_fill = _os.fill ('0');

  _os << (gmt->tm_year + 1900) << "-"
    << std::setw (2) << (gmt->tm_mon + 1) << "-"
    << std::setw (2) << gmt->tm_mday << "T"
    << std::setw (2) << gmt->tm_hour << ":"
    << std::setw (2) << gmt->tm_min << ":"
    << std::setw (5) << ((double) gmt->tm_sec +
			 (double) tv.tv_usec / USEC_SEC);

  _os.flags (old_settings);
  _os.precision (old_precision);
  _os.fill (old_fill);

  return _os;
}
