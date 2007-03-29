#include "rts2imgsetstat.h"

#include <math.h>
#include <iomanip>

#include "../utils/libnova_cpp.h"
#include "../utils/timestamp.h"

void
Rts2ImgSetStat::stat ()
{
  img_alt /= count;
  img_az /= count;
  if (astro_count > 0)
    {
      img_err /= astro_count;
      img_err_ra /= astro_count;
      img_err_dec /= astro_count;
    }
  else
    {
      img_err = nan ("f");
      img_err_ra = nan ("f");
      img_err_dec = nan ("f");
    }
}

std::ostream & operator << (std::ostream & _os, Rts2ImgSetStat & stat)
{
  _os << std::setw (6) << stat.count;
  if (stat.count == 0)
    return _os;
  _os << " " << std::setw (20) << TimeDiff (stat.exposure);
  _os << std::setw (6) << stat.astro_count
    << " (" << std::setw (3) << (100 * stat.astro_count /
				 stat.count) << "%) ";
  _os << LibnovaDeg90 (stat.img_alt) << " " << LibnovaDegArcMin (stat.
								 img_err);
  return _os;
}
