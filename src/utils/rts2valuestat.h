#ifndef __RTS2_VALUESTAT__
#define __RTS2_VALUESTAT__

#include "rts2value.h"

#include <vector>

/**
 * This class holds values which were obtained from quick measurements.
 */
class Rts2ValueDoubleStat:public Rts2ValueDouble
{
private:
  size_t numMes;
  double mean;
  double min;
  double max;
  double stdev;
    std::vector < double >valueList;
  void clearStat ();
public:
    Rts2ValueDoubleStat (std::string in_val_name);
    Rts2ValueDoubleStat (std::string in_val_name, std::string in_description,
			 bool writeToFits = true, int32_t flags = 0);
  void calculate ();

  virtual int setValue (Rts2Conn * connection);
  virtual const char *getValue ();
  virtual const char *getDisplayValue ();
  virtual int send (Rts2Conn * connection);
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

  std::vector < double >&getMesList ()
  {
    return valueList;
  }

  void addValue (double in_val)
  {
    valueList.push_back (in_val);
  }
};

#endif /* !__RTS2_VALUESTAT__ */
