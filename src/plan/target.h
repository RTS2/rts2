#ifndef __RTS_TARGET__
#define __RTS_TARGET__
#include <libnova/libnova.h>
#include <time.h>

class Target
{
public:
  int getPosition (struct ln_equ_posn *pos)
  {
    return getPosition (pos, ln_get_julian_from_sys ());
  };
  virtual int getPosition (struct ln_equ_posn *pos, double JD)
  {
    return -1;
  };
  virtual int getRST (struct ln_equ_posn *pos, struct ln_rst_time *rst,
		      double jd)
  {
    return -1;
  };
  int type;
  int id;
  int obs_id;
  time_t ctime;
  int tolerance;
  int moved;
  Target *next;
  int hi_precision;		// when 1, image will get imediately astrometry and telescope status will be updated
};

class ConstTarget:public Target
{
private:
  struct ln_equ_posn position;
public:
    ConstTarget (struct ln_equ_posn *in_pos);
  virtual int getPosition (struct ln_equ_posn *pos, double JD);
  virtual int getRST (struct ln_lnlat_posn *observer, struct ln_rst_time *rst,
		      double jd);
};

class EllTarget:public Target
{
private:
  struct ln_ell_orbit orbit;
public:
    EllTarget (struct ln_ell_orbit *in_orbit);
  virtual int getPosition (struct ln_equ_posn *pos, double JD);
  virtual int getRST (struct ln_lnlat_posn *observer, struct ln_rst_time *rst,
		      double jd);
};

class ParTarget:public Target
{
private:
  struct ln_par_orbit orbit;
public:
    ParTarget (struct ln_par_orbit *in_orbit);
  virtual int getPosition (struct ln_equ_posn *pos, double JD);
  virtual int getRST (struct ln_lnlat_posn *observer, struct ln_rst_time *rst,
		      double jd);
};

#endif /*! __RTS_TARGET__ */
