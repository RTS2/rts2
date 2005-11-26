#ifndef __LIBNOVA_CPP__
#define __LIBNOVA_CPP__

#include <libnova/libnova.h>
#include <ostream>

class LibnovaRa
{
private:
  double ra;
public:
    LibnovaRa (double in_ra)
  {
    ra = in_ra;
  }
  friend std::ostream & operator << (std::ostream & _os, LibnovaRa l_ra);
};

class LibnovaDeg
{
protected:
  double deg;
public:
    LibnovaDeg (double in_deg)
  {
    deg = in_deg;
  }
  friend std::ostream & operator << (std::ostream & _os, LibnovaDeg l_deg);
};

std::ostream & operator << (std::ostream & _os, LibnovaRa l_ra);
std::ostream & operator << (std::ostream & _os, LibnovaDeg l_deg);

class LibnovaDeg90:public LibnovaDeg
{
public:
  LibnovaDeg90 (double in_deg):LibnovaDeg (in_deg)
  {
  }

  friend std::ostream & operator << (std::ostream & _os, LibnovaDeg90 l_deg);
};

class LibnovaDegArcMin:public LibnovaDeg
{
public:
  LibnovaDegArcMin (double in_deg):LibnovaDeg (in_deg)
  {
  }

  friend std::ostream & operator << (std::ostream & _os,
				     LibnovaDegArcMin l_deg);
};

std::ostream & operator << (std::ostream & _os, LibnovaRa l_ra);
std::ostream & operator << (std::ostream & _os, LibnovaDeg l_deg);
std::ostream & operator << (std::ostream & _os, LibnovaDeg90 l_deg);
std::ostream & operator << (std::ostream & _os, LibnovaDegArcMin l_deg);

#endif /* !__LIBNOVA_CPP__ */
