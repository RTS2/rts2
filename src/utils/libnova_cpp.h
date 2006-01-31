#ifndef __LIBNOVA_CPP__
#define __LIBNOVA_CPP__

#include <libnova/libnova.h>
#include <ostream>

class LibnovaRa
{
protected:
  double ra;
  void toHms (struct ln_hms *ra_hms);
public:
    LibnovaRa (double in_ra)
  {
    ra = in_ra;
  }
  friend std::ostream & operator << (std::ostream & _os, LibnovaRa l_ra);
};

class LibnovaRaComp:public LibnovaRa
{
public:
  LibnovaRaComp (double in_ra):LibnovaRa (in_ra)
  {
  }
  friend std::ostream & operator << (std::ostream & _os, LibnovaRaComp l_ra);
};

class LibnovaDeg
{
protected:
  double deg;
  void toDms (struct ln_dms *deg_dms);
public:
    LibnovaDeg (double in_deg)
  {
    deg = in_deg;
  }
  double getDeg ()
  {
    return deg;
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

class LibnovaDeg90Comp:public LibnovaDeg90
{
public:
  LibnovaDeg90Comp (double in_deg):LibnovaDeg90 (in_deg)
  {
  }

  friend std::ostream & operator << (std::ostream & _os,
				     LibnovaDeg90Comp l_deg);
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

class LibnovaDegDist:public LibnovaDeg
{
public:
  LibnovaDegDist (double in_deg):LibnovaDeg (in_deg)
  {
  }

  friend std::ostream & operator << (std::ostream & _os,
				     LibnovaDegDist l_deg);
  friend std::istream & operator >> (std::istream & _is,
				     LibnovaDegDist & l_deg);
};

class LibnovaDate
{
private:
  struct ln_date date;
public:
    LibnovaDate (double JD)
  {
    ln_get_date (JD, &date);
  }

  LibnovaDate (struct ln_date *in_date)
  {
    date.years = in_date->years;
    date.months = in_date->months;
    date.days = in_date->days;
    date.hours = in_date->hours;
    date.minutes = in_date->minutes;
    date.seconds = in_date->seconds;
  }

  friend std::ostream & operator << (std::ostream & _os, LibnovaDate l_date);
};

std::ostream & operator << (std::ostream & _os, LibnovaRa l_ra);
std::ostream & operator << (std::ostream & _os, LibnovaRaComp l_ra);
std::ostream & operator << (std::ostream & _os, LibnovaDeg l_deg);
std::ostream & operator << (std::ostream & _os, LibnovaDeg90 l_deg);
std::ostream & operator << (std::ostream & _os, LibnovaDeg90Comp l_deg);
std::ostream & operator << (std::ostream & _os, LibnovaDegArcMin l_deg);
std::ostream & operator << (std::ostream & _os, LibnovaDegDist l_deg);
std::istream & operator >> (std::istream & _is, LibnovaDegDist & l_deg);
std::ostream & operator << (std::ostream & _os, LibnovaDate l_date);

#endif /* !__LIBNOVA_CPP__ */
