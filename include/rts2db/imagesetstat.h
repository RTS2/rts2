#ifndef __RTS2_IMGSETSTAT__
#define __RTS2_IMGSETSTAT__

#include <string>
#include <ostream>
#include <iomanip>

#include "libnova_cpp.h"
#include "timestamp.h"

namespace rts2db {

/**
 * Holds statistics for given filters.
 */
class ImageSetStat
{
	public:
		ImageSetStat (int in_filter = -1)
		{
			filter = in_filter;
			img_alt = 0;
			img_az = 0;
			img_err = 0;
			img_err_ra = 0;
			img_err_dec = 0;
			count = 0;
			astro_count = 0;
		}

		~ImageSetStat (void) {}

		int filter;
		float img_alt;
		float img_az;
		float img_err;
		float img_err_ra;
		float img_err_dec;
		int count;
		int astro_count;

		double exposure;

		void stat ();

		friend std::ostream & operator << (std::ostream & _os, ImageSetStat & stat)
		{
			_os << std::setw (6) << stat.count;
			if (stat.count == 0)
				return _os;
			_os << " " << std::setw (20) << TimeDiff (stat.exposure);
			_os << std::setw (6) << stat.astro_count
				<< " (" << std::setw (3) << (100 * stat.astro_count / stat.count) << "%) ";
			_os << LibnovaDegArcMin (stat.img_err) << " " << LibnovaDeg90 (stat.img_alt) 
				<< " " << LibnovaDeg360 (stat.img_az);
			return _os;
		}
};

}
#endif							 /* ! __RTS2_IMGSETSTAT__ */
