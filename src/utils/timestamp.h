#ifndef __TIMESTAMP_CPP__
#define __TIMESTAMP_CPP__

#include "../utils/rts2block.h"

#include <ostream>
#include <libnova/libnova.h>
#include <sys/time.h>

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
    ts = 0;
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
    ts = tv->tv_sec + tv->tv_usec / USEC_SEC;
  }
  void setTs (double _ts)
  {
    ts = _ts;
  }
  friend std::ostream & operator << (std::ostream & _os, Timestamp _ts);
};

std::ostream & operator << (std::ostream & _os, Timestamp _ts);

class TimeJD:public Timestamp
{
public:
  TimeJD (double JD):Timestamp ()
  {
    time_t _ts;
      ln_get_timet_from_julian (JD, &_ts);
      setTs (_ts);
  }
};

#endif /* !__TIMESTAMP_CPP__ */
