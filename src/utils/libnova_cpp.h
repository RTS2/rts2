#ifndef __LIBNOVA_CPP__
#define __LIBNOVA_CPP__

#include <libnova/libnova.h>
#include <ostream>
#include <time.h>

class LibnovaRa
{
protected:
  double ra;
  void toHms (struct ln_hms *ra_hms);
  void fromHms (struct ln_hms *ra_hms);
public:
    LibnovaRa ()
  {
    ra = nan ("f");
  }
  LibnovaRa (double in_ra)
  {
    ra = in_ra;
  }
  double getRa ()
  {
    return ra;
  }
  friend std::ostream & operator << (std::ostream & _os, LibnovaRa l_ra);
  friend std::istream & operator >> (std::istream & _is, LibnovaRa & l_ra);
};

class LibnovaRaJ2000:public LibnovaRa
{
public:
  LibnovaRaJ2000 ():LibnovaRa ()
  {
  }
  LibnovaRaJ2000 (double in_ra):LibnovaRa (in_ra)
  {
  }
  friend std::ostream & operator << (std::ostream & _os, LibnovaRaJ2000 l_ra);
};

class LibnovaHaM:public LibnovaRa
{
public:
  LibnovaHaM ():LibnovaRa ()
  {
  }
  LibnovaHaM (double in_ha):LibnovaRa (in_ha)
  {
  }
  friend std::ostream & operator << (std::ostream & _os, LibnovaHaM l_haM);
  friend std::istream & operator >> (std::istream & _is, LibnovaHaM & l_haM);
};

class LibnovaRaComp:public LibnovaRa
{
public:
  LibnovaRaComp ():LibnovaRa ()
  {
  }
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
  void fromDms (struct ln_dms *deg_dms);
public:
    LibnovaDeg ()
  {
    deg = nan ("f");
  }
  LibnovaDeg (double in_deg)
  {
    deg = in_deg;
  }
  LibnovaDeg (struct ln_dms *deg_dms)
  {
    fromDms (deg_dms);
  }
  double getDeg ()
  {
    return deg;
  }
  friend std::ostream & operator << (std::ostream & _os, LibnovaDeg l_deg);
  friend std::istream & operator >> (std::istream & _is, LibnovaDeg & l_deg);
};

class LibnovaDeg90:public LibnovaDeg
{
public:
  LibnovaDeg90 ():LibnovaDeg ()
  {
  }
  LibnovaDeg90 (double in_deg):LibnovaDeg (in_deg)
  {
  }

  friend std::ostream & operator << (std::ostream & _os, LibnovaDeg90 l_deg);
};

class LibnovaDec:public LibnovaDeg90
{
public:
  LibnovaDec ():LibnovaDeg90 ()
  {
  }
  LibnovaDec (double in_deg):LibnovaDeg90 (in_deg)
  {
  }
  double getDec ()
  {
    return getDeg ();
  }

  friend std::ostream & operator << (std::ostream & _os, LibnovaDec l_dec);
};

class LibnovaDecJ2000:public LibnovaDec
{
public:
  LibnovaDecJ2000 ():LibnovaDec ()
  {
  }
  LibnovaDecJ2000 (double in_deg):LibnovaDec (in_deg)
  {
  }

  friend std::ostream & operator << (std::ostream & _os,
				     LibnovaDecJ2000 l_dec);
};

class LibnovaDeg90Comp:public LibnovaDeg90
{
public:
  LibnovaDeg90Comp ():LibnovaDeg90 ()
  {
  }
  LibnovaDeg90Comp (double in_deg):LibnovaDeg90 (in_deg)
  {
  }

  friend std::ostream & operator << (std::ostream & _os,
				     LibnovaDeg90Comp l_deg);
};

class LibnovaDegArcMin:public LibnovaDeg
{
public:
  LibnovaDegArcMin ():LibnovaDeg ()
  {
  }
  LibnovaDegArcMin (double in_deg):LibnovaDeg (in_deg)
  {
  }

  friend std::ostream & operator << (std::ostream & _os,
				     LibnovaDegArcMin l_deg);
};

class LibnovaDegDist:public LibnovaDeg
{
public:
  LibnovaDegDist ():LibnovaDeg ()
  {
  }
  LibnovaDegDist (double in_deg):LibnovaDeg (in_deg)
  {
  }

  friend std::ostream & operator << (std::ostream & _os,
				     LibnovaDegDist l_deg);
  friend std::istream & operator >> (std::istream & _is,
				     LibnovaDegDist & l_deg);
};

class LibnovaRaDec
{
private:
  LibnovaRa * ra;
  LibnovaDec *dec;
public:
    LibnovaRaDec ()
  {
    ra = NULL;
    dec = NULL;
  }

  LibnovaRaDec (LibnovaRaDec & in_libnova)
  {
    ra = new LibnovaRa (in_libnova.getRa ());
    dec = new LibnovaDec (in_libnova.getDec ());
  }

  LibnovaRaDec (struct ln_equ_posn *pos)
  {
    ra = new LibnovaRa (pos->ra);
    dec = new LibnovaDec (pos->dec);
  }

  virtual ~ LibnovaRaDec (void)
  {
    delete ra;
    delete dec;
  }

  double getRa ()
  {
    if (ra)
      return ra->getRa ();
    return nan ("f");
  }

  double getDec ()
  {
    if (dec)
      return dec->getDec ();
    return nan ("f");
  }

  void getPos (struct ln_equ_posn *pos)
  {
    pos->ra = getRa ();
    pos->dec = getDec ();
  }

  void setPos (struct ln_equ_posn *pos)
  {
    delete ra;
    delete dec;
    ra = new LibnovaRa (pos->ra);
    dec = new LibnovaDec (pos->dec);
  }

  friend std::ostream & operator << (std::ostream & _os,
				     LibnovaRaDec l_radec);
  friend std::istream & operator >> (std::istream & _is,
				     LibnovaRaDec & l_radec);
};

class LibnovaDate
{
private:
  struct ln_date date;
public:
    LibnovaDate ()
  {
    ln_get_date_from_sys (&date);
  }

  LibnovaDate (double JD)
  {
    ln_get_date (JD, &date);
  }

  LibnovaDate (time_t * t)
  {
    ln_get_date_from_timet (t, &date);
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

  void getTimeT (time_t * t)
  {
    ln_get_timet_from_julian (ln_get_julian_day (&date), t);
  }

  friend std::ostream & operator << (std::ostream & _os, LibnovaDate l_date);
  friend std::istream & operator >> (std::istream & _is,
				     LibnovaDate & l_date);
};

/** 
 * Calculate from date night - e.g. date/time at which night started and at which it ends
 */
class Rts2Night
{
private:
  time_t from;
  time_t to;
public:
    Rts2Night ()
  {
    time (&to);
    from = to - 86400;
  }
  Rts2Night (struct tm *in_date, struct ln_lnlat_posn *obs);

  time_t *getFrom ()
  {
    return &from;
  }
  time_t *getTo ()
  {
    return &to;
  }

  friend std::ostream & operator << (std::ostream & _os, Rts2Night night);
};

std::ostream & operator << (std::ostream & _os, LibnovaRa l_ra);
std::istream & operator >> (std::istream & _os, LibnovaRa & l_ra);
std::ostream & operator << (std::ostream & _os, LibnovaRaJ2000 l_ra);
std::ostream & operator << (std::ostream & _os, LibnovaHaM l_haM);
std::istream & operator >> (std::istream & _is, LibnovaHaM & l_haM);
std::ostream & operator << (std::ostream & _os, LibnovaRaComp l_ra);
std::ostream & operator << (std::ostream & _os, LibnovaDeg l_deg);
std::istream & operator >> (std::istream & _is, LibnovaDeg & l_deg);
std::ostream & operator << (std::ostream & _os, LibnovaDeg90 l_deg);
std::ostream & operator << (std::ostream & _os, LibnovaDec l_dec);
std::ostream & operator << (std::ostream & _os, LibnovaDecJ2000 l_dec);
std::ostream & operator << (std::ostream & _os, LibnovaDeg90Comp l_deg);
std::ostream & operator << (std::ostream & _os, LibnovaDegArcMin l_deg);
std::ostream & operator << (std::ostream & _os, LibnovaDegDist l_deg);
std::istream & operator >> (std::istream & _is, LibnovaDegDist & l_deg);

std::ostream & operator << (std::ostream & _os, LibnovaRaDec l_radec);
std::istream & operator >> (std::istream & _is, LibnovaRaDec & l_radec);

std::ostream & operator << (std::ostream & _os, LibnovaDate l_date);
std::istream & operator >> (std::istream & _is, LibnovaDate & l_date);
std::ostream & operator << (std::ostream & _os, Rts2Night night);

#endif /* !__LIBNOVA_CPP__ */
