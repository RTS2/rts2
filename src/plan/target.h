#ifndef __RTS_TARGET__
#define __RTS_TARGET__
#include <libnova/libnova.h>
#include <time.h>

#define TYPE_OPORTUNITY         'O'
#define TYPE_GRB                'G'
#define TYPE_SKY_SURVEY         'S'
#define TYPE_GPS                'P'
#define TYPE_ELLIPTICAL         'E'
#define TYPE_HETE               'H'
#define TYPE_TECHNICAL          'T'

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
  int type;			// light, dark, flat, flat_dark
  int id;
  int obs_id;
  char obs_type;		// SKY_SURVEY, GBR, .. 
  time_t ctime;
  int tolerance;
  int moved;
  int hi_precision;		// when 1, image will get imediately astrometry and telescope status will be updated
  Target *next;
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
