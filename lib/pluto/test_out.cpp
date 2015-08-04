#include <stdio.h>
#include <math.h>
#include "norad.h"
#include "norad_in.h"

/* Example code to add BSTAR data using Ted Molczan's method.  It just
   reads in TLEs,  computes BSTAR if possible,  then writes out the
   resulting modified TLE.                                           */

int main( const int argc, const char **argv)
{
   FILE *ifile = fopen( argv[1], "rb");
   char line1[100], line2[100];

   while( fgets( line1, sizeof( line1), ifile))
      if( *line1 == '1' && fgets( line2, sizeof( line2), ifile)
                  && *line2 == '2')
         {
         tle_t tle;

         if( parse_elements( line1, line2, &tle) >= 0)
            {
            char obuff[200];
            double params[N_SGP4_PARAMS], c2;

            SGP4_init( params, &tle);
            c2 = params[0];
            if( c2 && tle.xno)
               tle.bstar = tle.xndt2o / (tle.xno * c2 * 1.5);
            write_elements_in_tle_format( obuff, &tle);
            printf( "%s", obuff);
            }
         }
   fclose( ifile);
   return( 0);
}
