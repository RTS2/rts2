/*
    sat_id.cpp     8 March 2003,  with updates as listed below

   An example 'main' function illustrating how to find which satellite(s)
are within a given radius of a given RA/dec,  as seen from a given
point.  The code reads in a file of observations in MPC format (name
provided as the first command-line argument).  For example:

sat_id mpc_obs.txt

   would hunt through the file 'mpc_obs.txt' for MPC-formatted
observations.  It would then read the file 'alldat.tle',  looking
for corresponding satellites within .2 degrees of said observations.
It then spits out the original file,  with satellite IDs added
(when found) after each observation line.  For each IDed satellite,
the international and NORAD designations are given,  along with
its angular distance from the search point,  position angle of
motion,  and apparent angular rate of motion in arcminutes/second
(or,  equivalently,  degrees/minute). */

/* 2 July 2003:  fixed the day/month/year to JD part of 'get_mpc_data()'
so it will work for all years >= 0 (previously,  it worked for years
2000 to 2099... plenty for the practical purpose of ID'ing recently-found
satellites,  but this is also an 'example' program.) */

/* 3 July 2005:  revised the check on the return value for parse_elements().
Now elements with bad checksums won't be rejected. */

/* 23 June 2006:  after comment from Eric Christensen,  revised to use
names 'ObsCodes.html' or 'ObsCodes.htm',  with 'stations.txt' being a
third choice.  Also added the '-a' command line switch to cause the program
to show all lines from input (default is now that only MPC astrometric
input gets echoed.)   */

/* 30 June 2006:  further comment from Eric Christensen:  when computing
object motion from two consecutive observations,  if the second one has
a date/time preceding the first,  you get a negative rate of motion that's
off by 180 degrees.  Fixed this. */

/* 17 Nov 2006:  artificial satellite data is now being provided in a
file named 'ALL_TLE.TXT'.  I've modified the default TLE to match. */

/* 22 Oct 2012:  minor cosmetic changes,  such as making constant variables
of type 'const',  updating URL for the MPC station code file,  adding a
comment or two. */

/* 7 Jan 2013:  revised output to show satellite name if available,  plus
the eccentricity,  orbital period,  and inclination. */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "norad.h"
#include "observe.h"

#define PI 3.1415926535897932384626433832795028841971693993751058209749445923
#define TIME_EPSILON (1./86400.)

static int get_mpc_data( const char *buff, double *jd, double *ra, double *dec)
{
   int i1, i2, i, year, month;
   double tval, day;
   static const char month_len[12] = { 31, 28, 31, 30, 31, 30,
                                       31, 31, 30, 31, 30, 31 };

   if( strlen( buff) < 80 || strlen( buff) > 82)
      return( -1);
   if( sscanf( buff + 32, "%d %d %lf", &i1, &i2, &tval) != 3)
      return( -2);
   *ra = ((double)i1 + (double)i2 / 60. + tval / 3600.) * (PI / 12.);

   if( sscanf( buff + 45, "%d %d %lf", &i1, &i2, &tval) != 3)
      return( -3);
   *dec = ((double)i1 + (double)i2 / 60. + tval / 3600.) * (PI / 180.);
   if( buff[44] == '-')
      *dec = -*dec;
   else if( buff[44] != '+')
      return( -4);

               /* Read in the day/month/year from the record... */
   if( sscanf( buff + 15, "%d %d %lf", &year, &month, &day) != 3)
      return( -5);
               /* ...and convert to a JD: */
   *jd = 1721059.5 + (double)( year * 365 + year / 4 - year / 100 + year / 400) + day;
   for( i = 0; i < month - 1; i++)
      *jd += (double)month_len[i];
   if( month < 3 && !(year % 4))    /* leap years,  January and February */
      if( !(year % 400) || (year % 100))
         (*jd)--;
   return( 0);
}

/* Quick and dirty computation of apparent motion,  good enough
   for our humble purposes.  If you want absolute perfection,  see
   the 'dist_pa.cpp' code at http://www.projectpluto.com/source.htm . */

static int compute_motion( const double delta_t,
                const double d_ra,  const double d_dec,
                double *arcmin_per_sec, double *posn_ang)
{
   int rval = 0;

   if( delta_t && (d_ra || d_dec))
      {
      double total_motion = sqrt( d_ra * d_ra + d_dec * d_dec);

      *posn_ang = atan2( d_ra, d_dec) * 180. / PI;
      if( *posn_ang < 0.)
         *posn_ang += 360.;
      if( delta_t < 0.)
         {
         *posn_ang = fmod( *posn_ang + 180., 360.);
         total_motion *= -1.;
         }
      *arcmin_per_sec = (total_motion * 180. / PI) / (delta_t * 1440.);
      }
   else     /* undefined or no motion */
      {
      rval = -1;
      *arcmin_per_sec = *posn_ang = 0.;
      }
   return( rval);
}

static void error_exit( const int exit_code)
{
   printf(
"sat_id takes the name of an input file of MPC-formatted (80-column)\n\
astrometry as a command-line argument.  It searches for matches between\n\
the observation data and satellites in 'ALL_TLE.TXT'.  By default,  matches\n\
within .2 degrees are shown.\n\n\
Additional command-line arguments are:\n\
   -r(radius)    Reset search distance from the default of .2 degrees.\n\
   -t(filename)  Reset the filename of the .tle file.\n\
   -a            Show all lines from input,  not just those with astrometry.\n");
   exit( exit_code);
}

int main( const int argc, const char **argv)
{
   const char *tle_file_name = "ALL_TLE.TXT";
   FILE *ifile = fopen( argv[1], "rb"), *tle_file;
   FILE *stations;
   char line1[100], line2[100], buff[90];
   double search_radius = .2;     /* default to .2-degree search */
   double lon = 0., rho_sin_phi = 0., rho_cos_phi = 0.;
   char curr_mpc_code[4];
   int i, debug_level = 0, show_non_mpc_report_lines = 0;
   char prev_obj[20];
   double prev_jd = 0., prev_ra = 0., prev_dec = 0.;

   if( argc == 1)
      error_exit( -2);

   if( !ifile)
      {
      printf( "Couldn't open input file %s\n", argv[1]);
      exit( -1);
      }

   stations = fopen( "ObsCodes.html", "rb");
   if( !stations)          /* perhaps stored with truncated extension? */
      stations = fopen( "ObsCodes.htm", "rb");
   if( !stations)          /* Or as a text file? */
      stations = fopen( "stations.txt", "rb");
   if( !stations)
      {
      printf( "Failed to find MPC station list 'ObsCodes.html'\n");
      printf( "This can be downloaded at:\n\n");
      printf( "http://www.minorplanetcenter.org/iau/lists/ObsCodes.html\n");
      exit( -1);
      }
   curr_mpc_code[0] = curr_mpc_code[3] = '\0';
   for( i = 1; i < argc; i++)
      if( argv[i][0] == '-')
         switch( argv[i][1])
            {
            case 'r':
               search_radius = atof( argv[i] + 2);
               break;
            case 't':
               tle_file_name = argv[i] + 2;
               break;
               break;
            case 'd':
               debug_level = atoi( argv[i] + 2);
               break;
            case 'a':
               show_non_mpc_report_lines = 1;
               break;
            default:
               printf( "Unrecognized command-line option '%s'\n", argv[i]);
               exit( -2);
               break;
            }

   tle_file = fopen( tle_file_name, "rb");
   if( !tle_file)
      {
      printf( "Couldn't open TLE file %s\n", tle_file_name);
      exit( -1);
      }

   *prev_obj = '\0';
   while( fgets( buff, sizeof( buff), ifile))
      {
      double target_ra, target_dec, jd;

      if( !get_mpc_data( buff, &jd, &target_ra, &target_dec))
         {
         char preceding_line[80], line0[100];
         double observer_loc[3], observer_loc2[3];

         printf( "\n%s", buff);
         if( !memcmp( prev_obj, buff, 12) && fabs( jd - prev_jd) < .3)
            {
            double motion, posn_ang;

            if( !compute_motion( jd - prev_jd,
                  (target_ra - prev_ra) * cos( (prev_dec + target_dec) / 2.),
                  (target_dec - prev_dec), &motion, &posn_ang))
               printf( "    Object motion is %.3lf'/sec at PA %.1lf\n",
                  motion, posn_ang);
            }

         memcpy( prev_obj, buff, 12);
         prev_ra = target_ra;
         prev_dec = target_dec;
         prev_jd = jd;
         if( memcmp( curr_mpc_code, buff + 77, 3))
            {
            char tbuff[100];
            int got_it = 0;

            memcpy( curr_mpc_code, buff + 77, 3);
            fseek( stations, 0L, SEEK_SET);
            while( !got_it && fgets( tbuff, sizeof( tbuff), stations))
               got_it = !memcmp( tbuff, curr_mpc_code, 3);
            if( got_it)
               sscanf( tbuff + 3, "%lf %lf %lf",
                                     &lon, &rho_cos_phi, &rho_sin_phi);
            if( !got_it)
               printf( "FAILED to find MPC code %s\n", curr_mpc_code);
            }

         if( debug_level)
            printf( "lon = %.5lf rho cos phi = %.5lf rho sin phi = %.5lf\n",
                                      lon,  rho_cos_phi, rho_sin_phi);
         observer_cartesian_coords( jd,
                lon * PI / 180., rho_cos_phi, rho_sin_phi, observer_loc);
         observer_cartesian_coords( jd + TIME_EPSILON,
                lon * PI / 180., rho_cos_phi, rho_sin_phi, observer_loc2);

         fseek( tle_file, 0L, SEEK_SET);
         *line0 = '\0';
         if( fgets( line1, sizeof( line1), tle_file))
            while( fgets( line2, sizeof( line2), tle_file))
               {
               tle_t tle;  /* Structure for two-line elements set for satellite */

               if( parse_elements( line1, line2, &tle) >= 0)
                  {                           /* hey! we got a TLE! */
                  int is_deep = select_ephemeris( &tle);
                  double sat_params[N_SAT_PARAMS], radius, d_ra, d_dec;
                  double ra, dec, dist_to_satellite, t_since;
                  double pos[3]; /* Satellite position vector */
                  double unused_delta2;

                  if( debug_level > 1)
                     printf( "%s", line1);
                  t_since = (jd - tle.epoch) * 1440.;
                  if( is_deep)
                     {
                     SDP4_init( sat_params, &tle);
                     SDP4( t_since, &tle, sat_params, pos, NULL);
                     }
                  else
                     {
                     SGP4_init( sat_params, &tle);
                     SGP4( t_since, &tle, sat_params, pos, NULL);
                     }
                  if( debug_level > 1)
                     printf( "%s", line2);
                  if( debug_level > 2)
                     printf( " %.5lf %.5lf %.5lf\n", pos[0], pos[1], pos[2]);
                  get_satellite_ra_dec_delta( observer_loc, pos,
                                          &ra, &dec, &dist_to_satellite);
                  if( debug_level > 3)
                     printf( "RA: %.5lf dec: %.5lf\n", ra * 180. / PI,
                                                      dec * 180. / PI);
                  epoch_of_date_to_j2000( jd, &ra, &dec);
                  d_ra = (ra - target_ra + PI * 4.);
                  while( d_ra > PI)
                     d_ra -= PI + PI;
                  d_dec = dec - target_dec;
                  radius = sqrt( d_ra * d_ra + d_dec * d_dec) * 180. / PI;
                  if( radius < search_radius)      /* good enough for us! */
                     {
                     double arcmin_per_sec, posn_ang;


                                       /* Compute position one second later,  so we */
                                       /* can show speed/PA of motion: */
                     t_since += TIME_EPSILON * 1440.;
                     if( is_deep)
                        SDP4( t_since, &tle, sat_params, pos, NULL);
                     else
                        SGP4( t_since, &tle, sat_params, pos, NULL);
                     get_satellite_ra_dec_delta( observer_loc2, pos,
                                             &d_ra, &d_dec, &unused_delta2);
                     epoch_of_date_to_j2000( jd, &d_ra, &d_dec);
                     d_ra -= ra;
                     d_dec -= dec;
                     while( d_ra > PI)
                        d_ra -= PI + PI;
                     while( d_ra < -PI)
                        d_ra += PI + PI;
                                /* Put RA into 0 to 2pi range: */
                     if( !compute_motion( TIME_EPSILON, d_ra * cos( dec), d_dec,
                                    &arcmin_per_sec, &posn_ang))
                        {
                        line1[8] = line1[16] = '\0';
                        memcpy( line1 + 30, line1 + 11, 6);
                        line1[11] = '\0';
                        printf( "   %s = %s%s-%s",
                              line1 + 2, (line1[9] >= '6' ? "19" : "20"),
                              line1 + 9, line1 + 30);
                        printf( " e=%.2lf; P=%.1lf min; i=%.1lf",
                                     tle.eo,
                                     2. * PI / tle.xno,
                                     tle.xincl * 180. / PI);
                        if( strlen( line0) < 30)         /* object name given... */
                           printf( ": %s\n", line0);     /* not all TLEs do this */
                        else
                           printf( "\n");
                        printf( "   delta=%8.1lf km; offset=%5.2lf degrees; motion %6.3lf'/sec at PA=%.1lf\n",
                              dist_to_satellite, radius, arcmin_per_sec,
                              posn_ang);
                                    /* "Speed" is displayed in arcminutes/second,
                                       or in degrees/minute */
                        }
                     }
                  }
               strcpy( preceding_line, line1);
               strcpy( line0, line1);
               strcpy( line1, line2);
               for( i = 0; line0[i] >= ' '; i++)
                  ;
               line0[i] = '\0';        /* remove trailing CR/LF */
               }
         }
      else if( show_non_mpc_report_lines)
         printf( "%s", buff);
      }
   fclose( tle_file);
   fclose( stations);
   fclose( ifile);
   return( 0);
} /* End of main() */

