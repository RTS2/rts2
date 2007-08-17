#ifndef __RTS2_EXPANDER__
#define __RTS2_EXPANDER__

#include <string>
#include <time.h>
#include <sys/time.h>
/**
 * This class is common ancestor to expending mechanism.
 * Short one-letter variables are prefixed with %, two letters and longer
 * variables are prefixed with $. Double % or $ works as escape characters.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Rts2Expander
{
private:
  struct tm expandDate;
  struct timeval expandTv;

    std::string getEpochString ();

  int getYear ()
  {
    return expandDate.tm_year + 1900;
  }

  int getMonth ()
  {
    return expandDate.tm_mon + 1;
  }

  int getDay ()
  {
    return expandDate.tm_mday;
  }

  int getYDay ()
  {
    return expandDate.tm_yday;
  }

  int getHour ()
  {
    return expandDate.tm_hour;
  }

  int getMin ()
  {
    return expandDate.tm_min;
  }

  int getSec ()
  {
    return expandDate.tm_sec;
  }

protected:
  int epochId;
  virtual std::string expandVariable (char var);
  virtual std::string expandVariable (std::string expression);
public:
  Rts2Expander ();
  Rts2Expander (Rts2Expander * in_expander);
  virtual ~ Rts2Expander (void);
  std::string expand (std::string expression);
  void setExpandDate (const struct timeval *tv);
  double getExpandDateCtime ();
  const struct timeval *getExpandDate ();

  // date related functions
  std::string getYearString ();
  std::string getMonthString ();
  std::string getDayString ();
  std::string getYDayString ();

  std::string getHourString ();
  std::string getMinString ();
  std::string getSecString ();
  std::string getMSecString ();

  long getCtimeSec ()
  {
    return expandTv.tv_sec;
  }

  long getCtimeUsec ()
  {
    return expandTv.tv_usec;
  }
};

#endif /* !__RTS2_EXPANDER__ */
