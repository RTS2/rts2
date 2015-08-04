/*
 *  satellites.cpp      26 January 2005
 *
 *  Important changes to install:
 *   - handle blank spaces in satellite line of TLE file.
 *   - handle no Epoch entries in first two lines of TLE file.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include "norad.h"

typedef struct {int hh; int mm; float ss;} hours;
typedef struct {int year; int month; int day;} date;
typedef struct {int deg; int mm; float ss;} degrees;

void JulianDay2Calendar(double JD, date *calendar, hours *UTC)
{
   /* Convert double precision Julian Day to calendar date & UTC */
   double dayfrac, JulianDay;
   int l, n, yr, mn;
   dayfrac=modf(JD+0.5,&JulianDay); /* JD starts at noon */
   UTC->hh=int(24.0*dayfrac);
   UTC->mm=int(60.0*(24.0*dayfrac-UTC->hh));
   UTC->ss=int((60.0*(24.0*dayfrac-UTC->hh)-UTC->mm)*60.0);
   /* compute month day year */
   /* based on routine in IDL Astro DAYCNV routine */
   l = int(JulianDay) + 68569;
   n = 4 * l / 146097;
   /* printf("\tl,n=%d,%d\n",l,n);*/
   l = l - (146097 * n + 3)/ 4;
   yr = 4000*(l + 1) / 1461001;
   /*printf("\tl,yr=%d,%d\n",l,yr);*/
   l = l - 1461*yr / 4 + 31;  /* 1461 = 365.25 * 4 */
   mn = 80 * l / 2447;
   /* printf("\tl,mn=%d,%d\n",l,mn); */
   calendar->day = int(l - 2447 * mn / 80);
   l = mn/11;
   calendar->month = int(mn + 2 - 12 * l);
   calendar->year = int(100*(n - 49) + yr + l);
   /* printf("year,month,day: %d, %d, %d\n",calendar->year,calendar->month,calendar->day); */
}

void processSatellite(char *satelliteName,
                      double startTime, double endTime, double stepTime,
                      tle_t *tle, int ephem, int format)
{
   /* Pass startTime & endTime in in minutes since satellite ephemeris Epoch.
      Specify stepTime in minutes.
      Make sure conversion is consistent.
    */
   double vel[3], pos[3]; /* Satellite position and velocity vectors */
   double time, RightAscension, declination, radius, sign;
   double currentTime;
   int is_deep = select_ephemeris( tle);
   char *ephem_names[5] = { "SGP ", "SGP4", "SGP8", "SDP4", "SDP8" };
   double sat_params[N_SAT_PARAMS];
   hours RA, UTC;
   degrees dec;
   date calendar;

   printf("# Satellite:           %s\n",satelliteName);
   printf("# Ephemeris Epoch:     %f\n",tle->epoch);
   printf("# Inclination:         %f\n",(180.0/M_PI)*tle->xincl);
   printf("# RA Ascending Node:   %f\n",(180.0/M_PI)*tle->xnodeo);
   printf("# Eccentricity:        %f\n",tle->eo);
   printf("# Argument of Perigee: %f\n",(180.0/M_PI)*tle->omegao);

   if( is_deep && (ephem == 1 || ephem == 2))
      ephem += 2;    /* switch to an SDx */
   if( !is_deep && (ephem == 3 || ephem == 4))
      ephem -= 2;    /* switch to an SGx */
   /* printf( "%s%s", line1, line2);*/
   if( is_deep)
      printf("# Deep-Space type Ephemeris (%s) selected\n",
             ephem_names[ephem]);
   else
      printf("# Near-Earth type Ephemeris (%s) selected\n",
             ephem_names[ephem]);

   /* Print some titles for the results */
   switch(format)
   {
      case 1:
         printf("#  Julian Day      Date    Time (UTC) ---------- Position (GEI/J2000) ------------     RA           Dec      \n");
         printf("#               MM/DD/YYYY  HH:MM:SS     x(km)      y(km)      z(km)    Radius(km)(hh:mm:ss.ss)(deg:mm:ss.s)\n");
         break;
      default:
         printf("#  Julian Day      Date    Time (UTC) ---------- Position (GEI/J2000) ------------     RA           Dec      \n");
         printf("#               MM/DD/YYYY  HH:MM:SS     x(km)      y(km)      z(km)    Radius(km)  (degrees)    (degrees)\n");
   }

   /* Calling of NORAD routines */
   /* Each NORAD routine (SGP, SGP4, SGP8, SDP4, SDP8)   */
   /* will be called in turn with the appropriate TLE set */
   switch( ephem)
   {
      case 0:
         SGP_init( sat_params, tle);
         break;
      case 1:
         SGP4_init( sat_params, tle);
         break;
      case 2:
         SGP8_init( sat_params, tle);
         break;
      case 3:
         SDP4_init( sat_params, tle);
         break;
      case 4:
         SDP8_init( sat_params, tle);
         break;
   }
   /* iterate over time steps */
   for(time=startTime; time<=endTime; time+=stepTime)
   {
      currentTime=(tle->epoch)+(time/24.0/60.0); /* compute JD of current time */
      switch( ephem)
      {
         case 0:
            SGP(time, tle, sat_params, pos, vel);
            break;
         case 1:
            SGP4(time, tle, sat_params, pos, vel);
            break;
         case 2:
            SGP8(time, tle, sat_params, pos, vel);
            break;
         case 3:
            SDP4(time, tle, sat_params, pos, vel);
            break;
         case 4:
            SDP8(time, tle, sat_params, pos, vel);
            break;
      }

      /* compute additional coordinate points */
      radius=sqrt(pos[0]*pos[0]+pos[1]*pos[1]+pos[2]*pos[2]); /* km */
      declination=(180.0/M_PI)*asin(pos[2]/radius);
      RightAscension=(180.0/M_PI)*atan2(pos[1],pos[0]);
      if(RightAscension<0.0) RightAscension+=360.0;
      /* compute date & UTC */
      JulianDay2Calendar(currentTime, &calendar, &UTC);

      switch (format)
      {
         case 1:

            /* compute position in hexigesimal (sp?) */
            sign=declination/fabs(declination);
            dec.deg=int(fabs(declination));
            dec.mm=int(60.0*(fabs(declination)-dec.deg));
            dec.ss=(60.0*(fabs(declination)-dec.deg)-dec.mm)*60.0;
            dec.deg=int(sign*dec.deg); /* restore appropriate sign */
            RA.hh=int(RightAscension);
            RA.mm=int(60.0*(RightAscension-RA.hh));
            RA.ss=(60.0*(RightAscension-RA.hh)-RA.mm)*60.0;

            printf(" %13.5f  %02d/%02d/%04d  %02d:%02d:%02.0f  "
                   "%10.2f %10.2f %10.2f %10.2f  %02d:%02d:%05.2f  %3d:%02d:%04.1f\n",
                   currentTime, calendar.month, calendar.day, calendar.year, UTC.hh, UTC.mm, UTC.ss,
                   pos[0],pos[1],pos[2], radius, RA.hh, RA.mm, RA.ss, dec.deg, dec.mm, dec.ss );
            break;
         default:
            printf(" %13.5f  %02d/%02d/%04d  %02d:%02d:%02.0f  "
                   "%10.2f %10.2f %10.2f %10.2f  %11.5f  %11.5f\n",
                   currentTime, calendar.month, calendar.day, calendar.year, UTC.hh, UTC.mm, UTC.ss,
                   pos[0],pos[1],pos[2], radius, RightAscension, declination );
      }
   } /* End of time loop  */
}

void helptext(void)
{
   printf("Command: satellites\n");
   printf("\t Generate a list of satellite positions between specified times.\n\n");
   printf("\tsatellites [options] <satname> <start> <end> <step> <ephemfile>\n");
   printf("\t\t<satname>:   satellite tag name (same as in TLE)\n");
   printf("\t\t<start>:     start time (Julian Day)\n");
   printf("\t\t<end>:       end time (Julian Day)\n");
   printf("\t\t<step>:      time step (minutes)\n");
   printf("\t\t<ephemfile>: satellite ephemeris file (TLE format)\n");
   printf("\tOptions\n");
   printf("\t\t -v:            verbose\n");
   printf("\t\t -f <fcode>:    format\n");
}

/* Main program */
int main( int argc, char **argv)
{
   tle_t tle; /* Pointer to two-line elements set for satellite */
   char satelliteName[10], ephemerisFile[255];
   double startTime, endTime, stepTime, startEpoch, endEpoch;
   double startTimeMin, endTimeMin;
   char header1[100], header2[100], line1[100], line2[100], line3[100];
   char *outputFile=NULL, tempstr[100];
   int optchar, length;
   int verbose=0;
   int formatcode=0;
   int offset=1; /* pointer into command line argument list */
   int ephem = 1;       /* default to SGP4 */

   /* parse optional command line parameters */
   while((optchar = getopt(argc, argv, "vf:o:h"))!=-1)
   {
      switch (optchar)
      {
         case 'v':
            verbose=1;
            offset++;
            break;
         case 'f':
            formatcode=atoi(strdup(optarg));
            offset++;
            break;
         case 'o':
            outputFile=(char *)strdup(optarg);
            offset++;
            break;
            /* insufficient parameters found,  print help message */
         default:
         case 'h':
         case '?':
            helptext();
            exit(0);
      }
   }

   if(argc < 5)
   {
      /* insufficient parameters found,  print help message */
      helptext();
      exit(0);
   }
   else
   {
      /* extract parameters from command line */
      strcpy(satelliteName,argv[offset++]);
      startTime=atof(argv[offset++]);
      endTime=atof(argv[offset++]);
      stepTime=atof(argv[offset++]);
      strcpy(ephemerisFile,argv[offset++]);
   }

   /* print command line parameters found */
   printf("# File Epoch Start: JD %18.10f\n",startTime);
   printf("# File Epoch End:   JD %18.10f\n",endTime);
   printf("# Step Time:           %f minutes\n",stepTime);
   printf("# TLE Ephemeris:       %s\n",ephemerisFile);

   /* test start, end & step times for validity */
   if(endTime<=startTime)
   {
      printf("ERROR: end time must be later than start time\n");
      exit(-1);
   }
   /* step <(end-start) */

   /* open ephemeris file as binary since it has a DOS line-ending */
   FILE *ifile = fopen(ephemerisFile, "rb");
   /* die horribly if unable to find ephemerisFile */
   if( !ifile)
      {
      printf( "Couldn't open input TLE file\n");
      exit( -1);
      }

   /* First two lines of ephemerisFile may contain start and end Epoch for
      the elements (julian days).  Subsequent lines are
        satellite name
        tle line 1
        tle line 2
      */

   /* read header lines */
   fgets(header1, sizeof( header1), ifile);
   if(!memcmp(header1,"#Start Epoch:",13))
   {
      strncpy(tempstr,&header1[14],18);
      startEpoch=atof(tempstr);
      fgets(header2, sizeof( header2), ifile);
      if(!memcmp(header2,"#End Epoch:",10))
      {
         strncpy(tempstr,&header2[12],18);
         endEpoch=atof(tempstr);
         if(verbose)
         {
            printf("# TLE Epoch (JD): %18.10f - %18.10f\n",startEpoch,endEpoch);
         }
         /* issue warning if date range outside TLE epoch */
         if((startEpoch > startTime) || (endEpoch < endTime))
         {
            printf("#\t***WARNING: TLE Epoch out of range for request.\n");
         }
      }
   }

   /* continue prowling through the file */
   while(!feof(ifile))
      {
      /* read data in groups of three lines */
      fgets( line1, sizeof( line1), ifile);
      fgets( line2, sizeof( line2), ifile);
      fgets( line3, sizeof( line3), ifile);
      /* compare to satellite name */
      length=strcspn(line1,"\r"); /* look for end of text line */
      if(strncmp(line1,satelliteName,length)==0)
      {

         /* parse the TLE lines */
         if( !parse_elements( line2, line3, &tle))    /* hey! we got a TLE! */
            {
            /* convert start and end times to minutes since start of
            satellite ephemeris Epoch */
            startTimeMin=(startTime-(tle.epoch))*60.0*24.0;
            endTimeMin=(endTime-(tle.epoch))*60.0*24.0;
            processSatellite(satelliteName, startTimeMin, endTimeMin, stepTime,
                           &tle, ephem, formatcode);
            printf("# END: Processing complete.\n");
            exit(0);
            }
         else
            {
               printf("ERROR: Incomprensible TLE entry.  Exiting...\n");
               exit(-1);
            }
      }
      }
   printf("Satellite %s not found in ephemeris %s.\n",satelliteName,ephemerisFile);
   if( outputFile)
      free( outputFile);
   fclose( ifile);
   return(0);
} /* End of main() */

/*------------------------------------------------------------------*/
