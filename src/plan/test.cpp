#include "target.h"
#include <stdio.h>
#include <stdlib.h>
#include <libnova/libnova.h>

void
print_jd (double JD, struct ln_lnlat_posn *obs)
{
  struct ln_date date;
  struct ln_equ_posn posn;
  struct ln_hrz_posn hrz;

  ln_get_equ_solar_coords (JD, &posn);
  ln_get_hrz_from_equ (&posn, obs, JD, &hrz);

  ln_get_date (JD, &date);
  printf ("%i/%i/%i %i:%i:%02.3f %02.2f", date.years, date.months, date.days,
	  date.hours, date.minutes, date.seconds, hrz.alt);
}

void
print_rst (struct ln_rst_time *rst, struct ln_lnlat_posn *obs)
{
  print_jd (rst->rise, obs);
  printf ("|");
  print_jd (rst->transit, obs);
  printf ("|");
  print_jd (rst->set, obs);
  printf ("\n");
}

int main (int argc, char **argv)
{
        ParTarget *target;
        struct ln_par_orbit orbit;
        time_t t;
        if (argc == 2)
                t = atoi (argv[1]);
        else
                time (&t);
        double jd = ln_get_julian_from_timet (&t);
        orbit.q = 0.16749;
        orbit.w = 333.298;
        orbit.omega = 222.992;
        orbit.i = 64.118;
        orbit.JD = 2453112.629;
        target = new ParTarget (&orbit);
        struct ln_equ_posn pos;
        printf ("Date: %s\n", ctime (&t));
        target->getPosition (&pos, jd);
        printf ("RA: %f DEC: %f\n", pos.ra, pos.dec);
        struct ln_lnlat_posn observer;
        observer.lng = -14;
        observer.lat = 50;
        struct ln_hrz_posn hrz;
        ln_get_hrz_from_equ (&pos, &observer, jd, &hrz);
        printf ("Alt: %f Az: %f\n", hrz.alt, hrz.az);
        for (int i = 0; i < 5; i++)
        {
                struct ln_rst_time rst;
                target->getRST (&observer, &rst, jd);
                print_rst (&rst, &observer);
                jd++;
        }
        return 0;
}
