#ifndef __RTS2_VALUESTAT__
#define __RTS2_VALUESTAT__

#include "rts2value.h"

#include <list>

/**
 * This class holds values which were obtained from quick measurements.
 */
class Rts2ValueDoubleStat:public Rts2ValueDouble
{
private:
  int numMes;
  double mean;
  double min;
  double max;
  double stdev;
    std::list < double >valueList;
  void clearStat ();
public:
    Rts2ValueDoubleStat (std::string in_val_name);
    Rts2ValueDoubleStat (std::string in_val_name, std::string in_description,
			 bool writeToFits = true, int32_t flags = 0);
  virtual int setValue (Rts2Conn * connection);
  virtual const char *getValue ();
  virtual const char *getDisplayValue ();
  virtual void setFromValue (Rts2Value * newValue);

  int getNumMes ()
  {
    return numMes;
  }

  double getMean ()
  {
    return mean;
  }

  double getMin ()
  {
    return min;
  }

  double getMax ()
  {
    return max;
  }

  double getStdev ()
  {
    return stdev;
  }

  std::list < double >&getMesList ()
  {
    return valueList;
  }
};

#endif /* !__RTS2_VALUESTAT__ */
