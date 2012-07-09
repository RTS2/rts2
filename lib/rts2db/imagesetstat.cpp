#include "rts2db/imagesetstat.h"

#include <math.h>

using namespace rts2db;

void ImageSetStat::stat ()
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
		img_err = NAN;
		img_err_ra = NAN;
		img_err_dec = NAN;
	}
}
