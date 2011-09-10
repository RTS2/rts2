#include "rts2count.h"
#include "timestamp.h"
#include "rts2config.h"

#include <iomanip>

Rts2Count::Rts2Count (int in_obs_id, long in_count_date, int in_count_usec,
int in_count_value, float in_count_exposure,
char in_count_filter)
{
  obs_id = in_obs_id;
  count_time.tv_sec = in_count_date;
  count_time.tv_usec = in_count_usec;
  count_value = in_count_value;
  count_exposure = in_count_exposure;
  count_filter = in_count_filter;
}


std::ostream & operator << (std::ostream & _os, Rts2Count & in_count)
{
  int old_precision = _os.precision ();
  _os.precision (2);

  _os
    << Timestamp (&in_count.count_time)
    << SEP << std::setw (10) << in_count.count_value
    << SEP << std::setw (8) << in_count.count_exposure
    << SEP << std::setw (8) << in_count.count_filter
    << std::endl;

  _os.precision (old_precision);

  return _os;
}
