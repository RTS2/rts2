#ifndef __RTS2_INFOVAL__
#define __RTS2_INFOVAL__

#include <ostream>
#include <iomanip>

#include "libnova_cpp.h"
#include "timestamp.h"

template < class val > class InfoVal
{
private:
  const char *desc;
  val value;
public:
  InfoVal (const char *in_desc, val in_value)
  {
    desc = in_desc;
    value = in_value;
  }

  friend std::ostream & operator<< < val > (std::ostream & _os,
					    InfoVal < val > _val);
};

template < class val >
  std::ostream & operator << (std::ostream & _os, InfoVal < val > _val)
{
  _os << std::left << std::setw (20) << _val.desc << std::right << _val.
    value << std::endl;
  return _os;
}

#endif
