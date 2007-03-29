#include "rts2imgsetstat.h"

#include <math.h>
#include <iomanip>

#include "../utils/libnova_cpp.h"

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
  _os << "images:" << stat.count;
  if (stat.count == 0)
    return _os;
  _os << " with exposure " << stat.exposure << " seconds ";
  int old_precision = _os.precision (0);
  _os << " with astrometry:" << stat.astro_count
    << " (" << std::setw (3) << (100 * stat.astro_count / stat.count) << "%)";
  _os.precision (2);
  _os << " avg. alt:" << LibnovaDeg90 (stat.img_alt)
    << " avg. err:" << LibnovaDegArcMin (stat.img_err);
  _os.precision (old_precision);
  return _os;
}
