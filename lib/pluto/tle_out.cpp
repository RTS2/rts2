#include <stdio.h>
#include <string.h>
#include <math.h>
#include "pluto/norad.h"

      /* Useful constants to define,  in case the value of PI or the number
         of minutes in a day should change: */
#define PI 3.141592653589793238462643383279502884197169399375105
#define MINUTES_PER_DAY 1440.
#define MINUTES_PER_DAY_SQUARED (MINUTES_PER_DAY * MINUTES_PER_DAY)
#define MINUTES_PER_DAY_CUBED (MINUTES_PER_DAY_SQUARED * MINUTES_PER_DAY)

#define AE 1.0
#define J1900 (2451545.5 - 36525. - 1.)

static int add_tle_checksum_data( char *buff)
{
   int count = 69, rval = 0;

   if( (*buff != '1' && *buff != '2') || buff[1] != ' ')
      return( 0);    /* not a .TLE */
   while( --count)
      {
      if( *buff < ' ' || *buff > 'Z')
         return( 0);             /* wups!  those shouldn't happen in .TLEs */
      if( *buff > '0' && *buff <= '9')
         rval += *buff - '0';
      else if( *buff == '-')
         rval++;
      buff++;
      }
   if( *buff != 10 && buff[1] != 10 && buff[2] != 10)
      return( 0);                              /* _still_ not a .TLE */
   *buff++ = (char)( '0' + (rval % 10));
   *buff++ = 13;
   *buff++ = 10;
   *buff++ = '\0';
   return( 1);
}

static double zero_to_two_pi( double angle_in_radians)
{
   angle_in_radians = fmod( angle_in_radians, PI + PI);
   if( angle_in_radians < 0.)
      angle_in_radians += PI + PI;
   return( angle_in_radians);
}

static void put_sci( char *obuff, double ival)
{
   if( !ival)
      memcpy( obuff, " 00000-0", 7);
   else
      {
      int oval = 0, exponent = 0;

      if( ival > 0.)
         *obuff++ = ' ';
      else
         {
         *obuff++ = '-';
         ival = -ival;
         }

      while( ival > 1.)    /* start exponent search in floats,  to evade */
         {                 /* an integer overflow */
         ival /= 10.;
         exponent++;
         }                 /* then do it in ints,  to evade roundoffs */
      while( oval > 99999 || oval < 10000)
         {
         oval = (int)( ival * 100000. + .5);
         if( oval > 99999)
            {
            ival /= 10;
            exponent++;
            }
         if( oval < 10000)
            {
            ival *= 10;
            exponent--;
            }
         }
      sprintf( obuff, "%5d", oval);
      if( exponent > 0)
         {
         obuff[5] = '+';
         obuff[6] = (char)( '0' + exponent);
         }
      else
         {
         obuff[5] = '-';
         obuff[6] = (char)( '0' - exponent);
         }
      }
}

void DLL_FUNC write_elements_in_tle_format( char *buff, const tle_t *tle)
{
   long year;
   double day_of_year, deriv_mean_motion;

   year = (int)( tle->epoch - J1900) / 365 + 1;
   do
      {
      double start_of_year;

      year--;
      start_of_year = J1900 + (double)year * 365. + (double)((year - 1) / 4);
      day_of_year = tle->epoch - start_of_year;
      }
      while( day_of_year < 1.);
   sprintf( buff,
/*                                     xndt2o    xndd6o   bstar  eph bull */
           "1 %05d%c %-8s %02ld%12.8lf -.00000000 +00000-0 +00000-0 %c %4dZ\n",
           tle->norad_number, tle->classification, tle->intl_desig,
           year % 100L, day_of_year,
           tle->ephemeris_type, tle->bulletin_number);
   if( buff[20] == ' ')       /* fill in leading zeroes for day of year */
      buff[20] = '0';
   if( buff[21] == ' ')
      buff[21] = '0';
   deriv_mean_motion = tle->xndt2o * MINUTES_PER_DAY_SQUARED / (2. * PI);
   if( deriv_mean_motion >= 0)
      buff[33] = ' ';
   deriv_mean_motion = fabs( deriv_mean_motion * 100000000.) + .5;
   sprintf( buff + 35, "%08ld", (long)deriv_mean_motion);
   buff[43] = ' ';
   put_sci( buff + 44, tle->xndd6o * MINUTES_PER_DAY_CUBED / (2. * PI));
   put_sci( buff + 53, tle->bstar / AE);
   add_tle_checksum_data( buff);
   buff += strlen( buff);
   sprintf( buff, "2 %05d %8.4lf %8.4lf %07ld %8.4lf %8.4lf %11.8lf%5dZ\n",
           tle->norad_number,
           zero_to_two_pi( tle->xincl) * 180. / PI,
           zero_to_two_pi( tle->xnodeo) * 180. / PI,
           (long)( tle->eo * 10000000. + .5),
           zero_to_two_pi( tle->omegao) * 180. / PI,
           zero_to_two_pi( tle->xmo) * 180. / PI,
                           tle->xno * MINUTES_PER_DAY / (2. * PI),
           tle->revolution_number);
   add_tle_checksum_data( buff);
}
