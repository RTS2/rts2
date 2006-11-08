#ifndef __TIMESTAMP_CPP__
#define __TIMESTAMP_CPP__

#include "../utils/rts2block.h"

#include <ostream>
#include <libnova/libnova.h>
#include <sys/time.h>
#include <time.h>

/**
 * Provides support timestamps obtained from DB.
 * 
 * It was primary designed to enable correct streaminig of timestamps (operator <<), but
 * it might be extendet to carry other operations with timestamp.
 * 
 * @author petr
 */
class Timestamp
{
private:
  double ts;
public:
    Timestamp ()
  {
    struct timeval tv;
      gettimeofday (&tv, NULL);
      ts = tv.tv_sec + tv.tv_usec / USEC_SEC;
  }
  Timestamp (double _ts)
  {
    ts = _ts;
  }
  Timestamp (time_t _ts)
  {
    ts = _ts;
  }
  Timestamp (struct timeval *tv)
  {
    ts = tv->tv_sec + (double) tv->tv_usec / USEC_SEC;
  }
  void setTs (double _ts)
  {
    ts = _ts;
  }
  double getTs ()
  {
    return ts;
  }
  friend std::ostream & operator << (std::ostream & _os, Timestamp _ts);
};

std::ostream & operator << (std::ostream & _os, Timestamp _ts);

class TimeJD:public Timestamp
{
public:
  TimeJD ():Timestamp ()
  {
  }
  /**
   * Construct Timestamp from JD.
   *
   * We cannot put it to Timestamp, as we already have in Timestamp
   * constructor which takes one double.
   *
   * @param JD Julian Day as double variable
   */
  TimeJD (double JD):Timestamp ()
  {
    time_t _ts;
    ln_get_timet_from_julian (JD, &_ts);
    setTs (_ts);
  }
};

class TimeDiff
{
private:
  double time_1, time_2;
public:
    TimeDiff ()
  {
  }

  /**
   * Construct time diff from two doubles.
   *
   */
  TimeDiff (double in_time_1, double in_time_2)
  {
    time_1 = in_time_1;
    time_2 = in_time_2;
  }

  friend std::ostream & operator << (std::ostream & _os, TimeDiff _td);
};

std::ostream & operator << (std::ostream & _os, TimeDiff _td);

class TimeJDDiff:public TimeJD
{
private:
  time_t time_diff;
public:
  TimeJDDiff ():TimeJD ()
  {
  }

  TimeJDDiff (double in_time_jd, time_t in_time_diff):TimeJD (in_time_jd)
  {
    time_diff = in_time_diff;
  }

  friend std::ostream & operator << (std::ostream & _os, TimeJDDiff _tjd);
};

std::ostream & operator << (std::ostream & _os, TimeJDDiff _tjd);

#endif /* !__TIMESTAMP_CPP__ */
