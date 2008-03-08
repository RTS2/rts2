#ifndef __RTS2_COUNT__
#define __RTS2_COUNT__

#include <ostream>
#include <sys/time.h>

/**
 * Class which hould information about one count.
 *
 * Holds one count from count devices, e.g. photometers.
 *
 * @author petr
 */

class Rts2Count
{
	private:
		int obs_id;
		struct timeval count_time;
		int count_usec;
		int count_value;
		float count_exposure;
		char count_filter;
	public:
		Rts2Count (int in_obs_id, long in_count_date, int in_count_usec,
			int in_count_value, float in_count_exposure,
			char in_count_filter);

		friend std::ostream & operator << (std::ostream & _os,
			Rts2Count & in_count);
};

std::ostream & operator << (std::ostream & _os, Rts2Count & in_count);
#endif							 /* !__RTS2_COUNT__ */
