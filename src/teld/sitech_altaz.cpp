/* 
 * Sidereal Technology Controller driver
 * Copyright (C) 2014 Petr Kubanek <petr@kubanek.net>
 * Copyright (C) 2012-2013 Shashikiran Ganesh <shashikiran.ganesh@gmail.com>
 * Based on John Kielkopf's xmtel linux software
 * Based on Dummy telescope for tests.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "gem.h"
#include "configuration.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <termios.h>
#include <math.h>
#include "sitech.h"
#include "connection/sitech.h"


#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif


/*!
 * Dummy teld for testing purposes.
 */

namespace rts2teld
{

/**
 * Dummy telescope class.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 * @author Markus Wildi
 */
class Sitech:public GEM
{
	public:
		Sitech (int argc, char **argv);
		~Sitech (void);

	protected:
		virtual int processOption (int in_opt);
		virtual int initValues ();
		virtual int init ();

		virtual int info ()
		{
			return Telescope::info ();
		}

		virtual int startResync ();

		virtual int startMoveFixed (double tar_az, double tar_alt)
		{
			return 0;
		}

		virtual int stopMove ()
		{
			FullStop();
			return 0;
		}

		virtual int startPark ();

		virtual int endPark ()
		{
			return 0;
		}


		virtual int isMoving ();

		virtual int isMovingFixed ()
		{
			return isMoving ();
		}

		virtual int isParking ()
		{
			return isMoving ();
		}

		virtual double estimateTargetTime ()
		{
			return getTargetDistance () * 2.0;
		}

	private:
		rts2core::ConnSerial *serConn;

		double SiteLatitude;         /* Latitude in degrees + north */  
		double SiteLongitude;        /* Longitude in degrees + west */ 
		double SiteTemperature;
		double SitePressure;
		double homera;                    /* Startup RA reset based on HA       */
		double homedec;                   /* Startup Dec                        */
		double parkra;                /* Park telescope at this HA          */
		double parkdec;              /* Park telescope at this Dec         */ 
		double offsetha;
		double offsetdec;
                 
		int pmodel;                    /* pointing model default to raw data */
	
		/* Contants */
		double mtrazmcal; 
		double mtraltcal;
		double mntazmcal;
		double mntaltcal;
	
		/* Global encoder counts */
		int mtrazm ;
		int mtralt ;
		int mntazm ;
		int mntalt ;

		/* Global pointing angles derived from the encoder counts */
		double mtrazmdeg;
		double mtraltdeg;
		double mntazmdeg;
		double mntaltdeg;
 
		/* Global tracking parameters */
		int azmtrackrate0;
		int alttrackrate0;
		int azmtracktarget;
		int azmtrackrate;
		int alttracktarget;
		int alttrackrate;
		
		/* Communications variables and routines for internal use */
		int TelPortFD;
		const char *device_file;
		int TelConnectFlag;

		double LSTNow(void);
//         double UTNow(void);
		double Map24(double hour);
		double Map12(double hour);
		double Map360(double degree);
		double Map180(double degree);
        
		void PointingFromTel (double *telra1, double *teldec1,double telra0, double teldec0);
	//void PointingToTel (double *telra0, double *teldec0,double telra1, double teldec1);
  
		void EquatorialToHorizontal(double ha, double dec, double *az, double *alt);
		void HorizontalToEquatorial(double az, double alt, double *ha, double *dec); 

		void Polar(double *ha, double *dec, int dirflag);
		void Model(double *ha, double *dec, int dirflag);
		void Refraction(double *ha, double *dec, int dirflag);
		void FullStop();
		
		int  SetLimits(int limits);
		int  GetLimits(int *limits);
		int SyncTelEncoders(void);
		void GetTel(double *telra, double *teldec);
		int GoToCoords(double newra, double newdec);
		void GetGuideTargets(int *ntarget, int *starget, int *etarget, int *wtarget);
		int CheckGoTo(double newra, double newdec);
		void StartTrack(void);
		double CalcLST(int year, int month, int day, double ut, double glong);
		double CalcJD(int ny, int nm, int nd, double ut);
};

}

using namespace rts2teld;


/* Full stop */
void Sitech:: FullStop(void)
{  
	char slewcmd[32] = "";
	
	sprintf (slewcmd,"XN\r");
	serConn->flushPortIO ();
	serConn->writePort (slewcmd,3);
	usleep (100000);
	sprintf (slewcmd,"YN\r");
	serConn->flushPortIO ();
	serConn->writePort (slewcmd,3);
	return;
}

/* Correct ha and dec for atmospheric refraction                       */
/*                                                                     */
/* Call this in the form Refraction(&ha,&dec,dirflag)                  */
/*                                                                     */
/* Input:                                                              */
/*   Pointers to ha and dec                                            */
/*   Integer dirflag >=0 for add (real to apparent)                    */
/*   Integer dirflag <0  for subtract (apparent to real)               */
/*   Valid above 15 degrees.  Below 15 degrees uses 15 degree value.   */
/*   Global local barometric SitePressure (Torr)                       */
/*   Global local air temperature SiteTemperature (C)                  */
/* Output:                                                             */
/*   In place pointers to modified ha and dec                          */

void Sitech::Refraction (double *ha, double *dec, int dirflag)
{
	double altitude, azimuth, dalt, arg;
	double hourangle, declination;
 

	/* Calculate the change in apparent altitude due to refraction */
	/* Uses 15 degree value for altitudes below 15 degrees */

	/* Real to apparent */
	/* Object appears to be higher due to refraction */
	
	hourangle = *ha;
	declination = *dec;
	EquatorialToHorizontal (hourangle, declination, &azimuth, &altitude);

	if (dirflag >= 0)
	{	
		if (altitude >= 15.)
		{
			arg = (90.0 - altitude) * PI / 180.0;
			dalt = tan (arg);
			dalt = 58.276 * dalt - 0.0824 * dalt * dalt * dalt;
			dalt = dalt / 3600.;
			dalt = dalt * (SitePressure / (760. * 1.01)) * (283. / (273. + SiteTemperature));
		} else {
			arg = (90.0 - 15.0) * PI / 180.0;
			dalt = tan (arg);
			dalt = 58.276 * dalt - 0.0824 * dalt * dalt * dalt;
			dalt = dalt / 3600.;
			dalt = dalt * (SitePressure / (760.*1.01)) * (283. / (273. + SiteTemperature));
		}
		altitude = altitude + dalt;
	}
	
	/* Apparent to real */
	/* Object appears to be higher due to refraction */
	
	else if (dirflag < 0)
	{
		if (altitude >= 15.)
		{
			arg = (90.0 - altitude) * PI / 180.0;
			dalt = tan (arg);
			dalt = 58.294 * dalt - 0.0668 * dalt * dalt * dalt;
			dalt = dalt / 3600.;
			dalt = dalt * (SitePressure / (760. * 1.01)) * (283. / (273. + SiteTemperature));
		}
	 else
		{
			arg = (90.0 - 15.0) * PI / 180.0;
			dalt = tan(arg);
			dalt = 58.294 * dalt - 0.0668 * dalt * dalt * dalt;
			dalt = dalt / 3600.;
			dalt = dalt * (SitePressure / (760. * 1.01)) * (283. / (273. + SiteTemperature));
		}

		altitude = altitude - dalt;
	}
	HorizontalToEquatorial(azimuth, altitude, &hourangle, &declination);
	*ha = hourangle;
	*dec = declination; 
}

/* Allow for known polar alignment parameters           */
/*                                                      */
/* Call this in the form Polar(&ha,&dec,dirflag)        */
/*                                                      */
/* Input:                                               */
/*   Pointers to ha and dec                             */
/*   Integer dirflag >=0  real to apparent              */
/*   Integer dirflag <0   apparent to real              */
/* Output:                                              */
/*   In place pointers to modified ha and dec           */
/* Requires:                                            */
/*   SiteLatitude in degrees                            */
/*   polaralt polar axis altitude error in degrees      */
/* polaraz  polar axis azimuth  error in degrees        */
void Sitech::Polar (double *ha, double *dec, int dirflag)
{
	/*double hourangle, declination;
	double da, db;
	double epsha, epsdec;
	double latitude;
	
	// Make local copies of input 
	
	hourangle = *ha;
	declination = *dec;	
	da = polaraz;
	db = polaralt;
	latitude = SiteLatitude;
	
	// Convert to radians for the calculations 
	 
	hourangle = hourangle*PI/12.;
	declination = declination*PI/180.;
	da = da*PI/180.;
	db = db*PI/180.; 
	latitude = latitude*PI/180.; 
	
	// Evaluate the corrections in radians for this ha and dec 
	// Sign check:	on the meridian at the equator 
	//	 epsha goes as -da 
	//	 epsdec goes as +db 
	//	 polaraz and polaralt signed + from the true pole to the mount pole 
		
	epsha = db*tan(declination)*sin(hourangle) - 
		da*(sin(latitude) - tan(declination)*cos(hourangle)*cos(latitude));
	epsdec = db*cos(hourangle) - da*sin(hourangle)*cos(latitude);		
		
	// Real to apparent 
	
	if (dirflag >= 0)
	{	
		hourangle = hourangle + epsha;
		declination = declination + epsdec;
	}
	
	// Apparent to real 
	
	else if (dirflag < 0)
	{
		hourangle = hourangle - epsha;
		declination = declination - epsdec;
	}

	// Convert from radians to hours for ha and degrees for declination 

	hourangle = hourangle*12./PI;
	declination = declination*180./PI;
	
	// Check for range and adjust to -90 -> +90 and 0 -> 24	
 
	if (declination > 90. ) 
	{
		declination = 180. - declination;
		hourangle = hourangle + 12.;
	}
	 if (declination < -90. ) 
	{
		declination = -180. - declination;
		hourangle = hourangle + 12.;
	}	

	hourangle = Map24(hourangle);				 
	*ha = hourangle;
	*dec = declination;
	*/
}

/* Model corrections                                         */
/*                                                           */
/* Call this in the form Model(&ha,&dec,dirflag)             */
/*                                                           */
/* Input:                                                    */
/*   Pointers to ha and dec                                  */
/*   Integer dirflag >=0  real to apparent                   */
/*   Integer dirflag <0   apparent to real                   */
/* Output:                                                   */
/*   In place pointers to modified ha and dec                */
/* Requires global values:                                   */
/*   Model ha parameters (pixels/hour)                       */
/*   Model dec parameters (pixels/hour)                      */
/*   Scale arcsecperpix                                      */
/*                                                           */
/* Deltas add to apparent pointing to give real pointing     */
/* Values of parameters are global                           */
void Sitech::Model (double *ha, double *dec, int dirflag)
{
	/*double tmpha, tmpdec;
	double dha, ddec;
	double deltaha;
	
	// Make local copies of input 
	
	tmpha = *ha;
	tmpdec = *dec;
	
	deltaha = Map12(tmpha - modelha0);
	
	// Calculate model dec increment in degrees from the reference point 
	
	ddec = deltaha*modeldec1*arcsecperpix/3600.;
	
	// Calculate model ha increment in hours from the reference point 
	
	dha = deltaha*modelha1*arcsecperpix/54000.;
	
	if (dirflag >= 0)
	{	
		// For dir = +1 returns the apparent pointing given the real pointing 
		
		tmpha = tmpha - dha;
		tmpdec = tmpdec - ddec;		
	}
	else if (dirflag < 0)
	{
		// For dir = -1 returns the real pointing given the apparent pointing 

		tmpha = tmpha + dha;
		tmpdec = tmpdec + ddec;		
	}

	*ha = tmpha;
	*dec = tmpdec;
*/
}

/* Apply corrections to coordinates reported by the telescope              */
/* Input telescope raw coordinates assumed zero corrected                  */
/* Return real coordinates corresponding to this raw point                 */
/* Implemented in order: polar, decaxis, optaxis, flexure, refract, offset */
/* Note that the order is reversed from that in PointingFromTel            */
void Sitech::PointingFromTel (double *telra1, double *teldec1,double telra0, double teldec0)
{
	double tmpra, tmpha, tmpdec, tmplst;
	
	/* Find the LST for the pointing corrections and save it */
	/* Otherwise the lapsed time to do corrections affects the resultant HA and RA */
		
	tmplst = LSTNow ();
	tmpra = telra0;
	tmpha = tmplst - tmpra;
	tmpha = Map12 (tmpha);
	tmpdec = teldec0;
			
	/* Correct for mounting misalignment */
	/* For dir = -1 returns the real pointing given the apparent pointing */
	
	if (pmodel == (pmodel | POLAR))
	{
		Polar( &tmpha,	&tmpdec, -1);
	} 
	
	/* Correct for actual real time behavior	*/
	/* For dir = -1 returns the real pointing given the apparent pointing */
	
	if (pmodel == (pmodel | DYNAMIC))
	{
		Model( &tmpha,	&tmpdec, -1);
	} 

	/* Correct for atmospheric refraction */
	/* For dir = -1 returns the real pointing given the apparent pointing */
		
	if (pmodel == (pmodel | REFRACT))
	{
		Refraction (&tmpha, &tmpdec, -1);
	}

	/* Add offsets to apparent pointing to give real pointing */
	
	if (pmodel == (pmodel | OFFSET))
	{
		tmpha = tmpha + offsetha;
		tmpdec = tmpdec + offsetdec;
	}

	/* Now use celestial coordinates */
	
	tmpra = tmplst - tmpha;
		
	/* Handle a case that has crossed the 24 hr mark */
	
	tmpra = Map24(tmpra);
	
	/* Handle a case that has gone over the south pole */
	/* Limit set to 0.0001 to avoid inadvertent trip on roundoff error */
 
	if (tmpdec < -90.0002)
	{
		 tmpdec = -180. - tmpdec;
		 tmpra = tmpra + 12.;
		 Map24 (tmpra); 
	}
	
	/* Handle a case that has gone over the north pole */	
	/* Limit set to 0.0002 to avoid inadvertent trip on roundoff error */
	
	if (tmpdec > 90.0002)
	{
		 tmpdec = 180. - tmpdec;
		 tmpra = tmpra + 12.;
		 Map24 (tmpra); 
	} 
	
	*telra1 = tmpra;
	*teldec1 = tmpdec;

	return;
}

/* Find the apparent coordinates to send to the telescope                  */
/* Input real target coordinates                                           */
/* Return apparent telescope coordinates to have it point at this target   */
/* Implemented in order: offset, refract, flexure, optaxis, decaxis, polar */
/* Note that the order is reversed from that in PointingFromTel            */

/*void Sitech::PointingToTel (double *telra0, double *teldec0, double telra1, double teldec1)
{
	double tmpra, tmpha, tmpdec, tmplst;
	
	tmpra = telra1;
	tmpdec = teldec1;


	// Save the LST for later use, find the HA for this LST and finish the model 

	tmplst = LSTNow();
	tmpha = tmplst - tmpra;
	tmpha = Map12(tmpha); 


	// Correct for offset 
	// Offsets are defined as added to apparent pointing to give real pointing 
	
	if ( pmodel == (pmodel | OFFSET) )
	{
		tmpha = tmpha - offsetha;
		tmpdec = tmpdec - offsetdec;
	}	

	 
	// Correct for atmospheric refraction 
	// For dir = +1 returns the apparent pointing given the real target pointing 
 
	if ( pmodel == (pmodel | REFRACT) )
	{
		Refraction( &tmpha,	&tmpdec, 1);
	}

	// Correct for real time behavior 
	// For dir = +1 returns the apparent pointing given the real pointing 
	 
	if ( pmodel == (pmodel | DYNAMIC) )
	{
		Model( &tmpha,	&tmpdec, 1);
	} 

	// Correct for mounting misalignment 
	// For dir = +1 returns the apparent pointing given the real pointing 
	
	if ( pmodel == (pmodel | POLAR) )
	{
		Polar( &tmpha,	&tmpdec, 1);
	} 
	
	tmpra = tmplst - tmpha;

	// Handle a case that has crossed the 24 hr mark 
	
	tmpra = Map24(tmpra);
	
	// Handle a case that has gone over the south pole 
	
	if (tmpdec < -90.)
	{
		 tmpdec = -180. - tmpdec;
		 tmpra = tmpra + 12.;
		 Map24(tmpra); 
	}
	
	// Handle a case that has gone over the north pole	
	
	if (tmpdec > 90.)
	{
		 tmpdec = 180. - tmpdec;
		 tmpra = tmpra + 12.;
		 Map24(tmpra); 
	} 
			 
	*telra0 = tmpra;
	*teldec0 = tmpdec;

	return;
}*/

/* Convert local az and alt to local ha and dec */
/* Geographic azimuth convention is followed: */
/*   Due north is zero and azimuth increases from north to east */
void Sitech::HorizontalToEquatorial (double az, double alt, double *ha, double *dec)
{
	double phi, hourangle, declination ;

	alt = alt * PI / 180.;
	az = Map360 (az);
	az = az * PI / 180.;
	phi = SiteLatitude * PI / 180.;

	hourangle = atan2 (-sin (az) * cos (alt), cos (phi) * sin (alt) - sin (phi) * cos (alt) * cos (az));
	declination = asin (sin (phi) * sin (alt) + cos (phi) * cos (alt) * cos (az));
	hourangle = Map12 (hourangle * 12. / PI);
	declination = Map180 (declination * 180. / PI);
		
	/* Test for hemisphere */
	
	if(declination > 90.)
	{
		*ha = Map12 (hourangle + 12.);
		*dec = 180. - declination;
	}
	else if (declination < -90.)
	{
		*ha = Map12 (hourangle + 12.);
		*dec = declination + 180;
	} 
	else
	{ 
		*ha = hourangle;
		*dec = declination;
	}
}

/* Convert local ha and dec to local az and alt  */
/* Geographic azimuth convention is followed: */
/*   Due north is zero and azimuth increases from north to east */
void Sitech::EquatorialToHorizontal (double ha, double dec, double *az, double *alt)
{
	double phi, altitude, azimuth;

	ha = ha*PI/12.;
	phi = SiteLatitude*PI/180.;
	dec = dec*PI/180.;
	altitude = asin (sin (phi) * sin (dec) + cos (phi) * cos (dec) * cos (ha));
	altitude = altitude * 180.0 / PI;
	azimuth = atan2 (-cos (dec) * sin (ha), sin (dec) * cos (phi)-sin (phi) * cos (dec) * cos(ha));
	azimuth = azimuth * 180.0 / PI;

	azimuth = Map360 (azimuth);

	*alt = altitude;
	*az = azimuth;
}

/* Map a time in hours to the range  0  to 24 */

double Sitech::Map24 (double hour)
{
	int n;
	
	if (hour < 0.0) 
	{
		 n = (int) (hour / 24.0) - 1;
		 return (hour - n * 24.0);
	} 
	else if (hour >= 24.0) 
	{
		 n = (int) (hour / 24.0);
		 return (hour - n * 24.0);
	} 
	else 
	{
		 return (hour);
	}
}


/* Map an hourangle in hours to  -12 <= ha < +12 */

double Sitech::Map12 (double hour)
{
	double hour24;
	
	hour24=Map24(hour);
	
	if (hour24 >= 12.0) 
	{
		return (hour24 - 24.0);
	}
	else
	{
		return (hour24);
	}
}

/* Map a time in hours to the range  0  to 24 */

double Map24 (double hour)
{
	int n;
	
	if (hour < 0.0) 
	{
		 n = (int) (hour / 24.0) - 1;
		 return (hour - n * 24.0);
	} 
	else if (hour >= 24.0) 
	{
		 n = (int) (hour / 24.0);
		 return (hour - n * 24.0);
	} 
	else 
	{
		 return (hour);
	}
}

/* Map an angle in degrees to  0 <= angle < 360 */

double Sitech::Map360 (double angle)
{
	int n;

	if (angle < 0.0) 
	{
		n = (int) (angle / 360.0) - 1;
		return (angle - n * 360.0);
	} 
	else if (angle >= 360.0)
	{
		n = (int) (angle / 360.0);
		return (angle - n * 360.0);
	} 
	else 
	{
		return (angle);
	}
}

/* Map an angle in degrees to -180 <= angle < 180 */

double Sitech::Map180 (double angle)
{
	double angle360;
	angle360 = Map360 (angle);
	
	if (angle360 >= 180.0) 
	{
		 return (angle360 - 360.0);
	} 
	else 
	{
		 return (angle360);
	}
}

/*  Compute the Julian Day for the given date */
/*  Julian Date is the number of days since noon of Jan 1 4713 B.C. */
double Sitech::CalcJD (int ny, int nm, int nd, double ut)
{
	double A, B, C, D, jd, day;

	day = nd + ut / 24.0;
	if ((nm == 1) || (nm == 2)) {
		ny = ny - 1;
		nm = nm + 12;
	}

	if (((double) ny + nm / 12.0 + day / 365.25) >= (1582.0 + 10.0 / 12.0 + 15.0 / 365.25)) 
	{
		A = ((int) (ny / 100.0));
		B = 2.0 - A + (int) (A / 4.0);
	} 
	else 
	{
		B = 0.0;
	}

	if (ny < 0.0) 
	{
		C = (int) ((365.25 * (double) ny) - 0.75);
	} 
	else 
	{
		C = (int) (365.25 * (double) ny);
	}

	D = (int) (30.6001 * (double) (nm + 1));
	jd = B + C + D + day + 1720994.5;
	return jd;
}

double frac (double x)
{
	x -= (int) x;
	return (x < 0) ? x + 1.0 : x;
}

/*  Compute Greenwich Mean Sidereal Time (gmst) */
/*  TU is number of Julian centuries since 2000 January 1.5 */
/*  Expression for gmst from the Astronomical Almanac Supplement */
double Sitech::CalcLST (int year, int month, int day, double ut, double glong)
{
	double TU, TU2, TU3, T0;
	double gmst,lmst;

	TU = (CalcJD (year, month, day, 0.0) - 2451545.0) / 36525.0;
	TU2 = TU * TU;
	TU3 = TU2 * TU;
	T0 = (24110.54841 / 3600.0) + 8640184.812866 / 3600.0 * TU + 0.093104 / 3600.0 * TU2 - 6.2e-6 / 3600.0 * TU3;
	T0 = Map24(T0);

	gmst = Map24 (T0 + ut * 1.002737909);
	lmst = 24.0 * frac ((gmst - glong / 15.0) / 24.0);
	return lmst;
}

/* Calculate the local sidereal time with millisecond resolution */

double Sitech::LSTNow(void)
{
	int year, month, day;
	int hours, minutes, seconds, milliseconds;
	long int usecs;

	double glong;		
	double ut, lst;
	time_t now;
	struct timeval t;
	struct tm *g;
	
	
	/* Use gettimeofday and timeval to capture microseconds */
	
	gettimeofday (&t,NULL);
	
	/* Copy seconds part to convert to UTC */
	
	now = t.tv_sec;
	
	/* Save microseconds part to handle milliseconds of UTC */
	
	usecs = t.tv_usec; 

	/* Convert unix time now to utc */
	
	g = gmtime(&now);

	/* Copy values pointed to by g to variables we will use later */

	year = g->tm_year;
	year = year + 1900;
	month = g->tm_mon;
	month = month + 1;
	day = g->tm_mday;
	hours = g->tm_hour;
	minutes = g->tm_min;
	seconds = g->tm_sec;
	
	/* Convert the microseconds part to milliseconds */
	
	milliseconds = usecs/1000;

	/* Calculate floating point ut in hours */

	ut = ((double) milliseconds ) / 3600000. + ((double) seconds) / 3600. + ((double) minutes ) / 60. + ((double) hours );

	glong = SiteLongitude;

	lst = CalcLST (year, month, day, ut, glong);
		
	return lst;
}

/* Calculate the universal time in hours with millisecond resolution */

/*double Sitech::UTNow(void)
{
	int hours,minutes,seconds,milliseconds;
	long int usecs;

	double ut;
	time_t now;
	struct timeval t;
	struct tm *g;
	
	// Use gettimeofday and timeval to capture microseconds 
	
	gettimeofday(&t,NULL);
	
	// Copy seconds part to convert to UTC 
	
	now = t.tv_sec;
	
	// Save microseconds part to handle milliseconds of UTC 
	
	usecs = t.tv_usec; 

	// Convert unix time now to utc 
	
	g=gmtime(&now);

	// Copy values pointed to by g to variables we will use later 

	hours = g->tm_hour;
	minutes = g->tm_min;
	seconds = g->tm_sec;
	
	// Convert the microseconds part to milliseconds 
	
	milliseconds = usecs/1000;

	// Calculate floating point ut in hours 

	ut = ( (double) milliseconds )/3600000. +
		( (double) seconds )/3600. + 
		( (double) minutes )/60. + 
		( (double) hours );	 

	return ut;
}*/


/* Get status of slew limits control */
int Sitech::GetLimits (int *limits)
{
	int sitechlimits;
	sitechlimits = 0;
	
	if (sitechlimits == 1)
	{
		*limits = 1;
	}
	else
	{
		*limits = 0;
	}

	return sitechlimits;
}
  
/* Set slew limits control off or on */
int Sitech::SetLimits (int limits)
{
	if (limits == TRUE)
	{
		fprintf (stderr, "Limits enabled\n");
	}
	else
	{
		limits = FALSE;
		fprintf (stderr, "Limits disabled\n");
	}

	return (limits);
}

/* Synchronize the two encoder systems */
/* Returns 1 if synchronization is within 15 counts in both axes */
int Sitech::SyncTelEncoders (void)
{
	char sendstr[32] = "";
	int nsend = 0;
	
	double nowra0, nowdec0;
	int mtrazmnew, mtraltnew;
			
	/* Zero points:																								*/
	/*	 Alt-Az -- horizon north																	 */
	/*	 Fork equatorial -- equator meridian											 */
	/*	 German equatorial -- over the mount at the pole					 */
	
	/* Signs:																											*/
	/*	 Dec or Alt -- increase from equator to pole and beyond		*/ 
	/*		 GEM dec sign is for west of pier looking east					 */ 
	/*		 GEM dec sign reverses when east of pier looking west		*/
	/*	 HA or Az	 -- increase from east to west									*/

	/* Get the current pointing																		*/
	/* This sets the global telmntazm and telmntalt counters			 */
		
	GetTel (&nowra0, &nowdec0);
	
	/* Assign the motor counts to match the mount counts					 */
		
	mtrazmnew = ((double) mntazm) * mtrazmcal / mntazmcal;
	mtraltnew = ((double) mntalt) * mtraltcal / mntaltcal;
		 
	/* Set the controller motor counters													 */
	
	serConn->flushPortIO ();
	sprintf (sendstr, "YF%d\r", mtrazmnew);
	nsend = strlen (sendstr);
	serConn->writePort (sendstr,nsend);
	
	usleep (100000);
	
	serConn->flushPortIO ();
	sprintf (sendstr, "XF%d\r", mtraltnew);
	nsend = strlen (sendstr);
	serConn->writePort (sendstr, nsend);
	
	usleep (100000);

	/* Query the controller to update the counters	*/
	
	GetTel(&nowra0, &nowdec0); 
	
	if ((abs(mtrazm - mtrazmnew) > 15) || (abs(mtralt - mtraltnew) > 15))
	{
		return 0;
	}	

	return 1;
}

/* Read the motor and mount encoders                                  */
/* Save encoder counters in global variables                          */
/* Convert the counters to ha and dec                                 */
/* Use an NTP synchronized clock to find lst and ra                   */
/* Correct for the pointing model to find the true direction vector   */
/* Report the ra and dec at which the telescope is pointed            */

void Sitech::GetTel (double *telra, double *teldec)
{  
	char returnstr[32] = "";
	int numread;
	 
	/* Celestial coordinates derived from the pointing angles */ 
 
	double telha0 = 0.;
	double teldec0 = 0.;
	double telra0 = 0.;
	double telra1 = 0.;
	double teldec1 = 0.;
	
	/* Buffers for decoding the binary packet */
	
	int b0, b1, b2, b3;

	/* Send an XXS command to the controller */
	/* Controller should return 20 bytes of information */

	serConn->flushPortIO ();
	serConn->writePort ("XXS\r", 4);

	numread = serConn->readPort (returnstr, 20);
	if (numread != 20)
	{
		/* Brief pause then try again with more wait time */
		usleep(100000);
		serConn->flushPortIO ();	
		serConn->writePort ("XXS\r", 4);
		numread=serConn->readPort (returnstr, 20);
		if (numread != 20)
		{
			logStream(MESSAGE_ERROR) << "Telescope control is not responding ..." << sendLog;
			return;
		}	
	}

	/* Parse motor encoder readings from the response				 */
	/* Caution that Sitech 24 bit counters seem to ignore b0	*/
	/* Little endian order																		*/
	
	/* Translate altitude packet to integers without sign */
	
	b0 = (unsigned char) returnstr[0];
	b1 = (unsigned char) returnstr[1];
	b2 = (unsigned char) returnstr[2];
	b3 = (unsigned char) returnstr[3];
	
	/* Compute an unsigned altitude encoder count */
	
	mtralt = b1 + 256 * b2 + 256 * 256 * b3;
			 
		
	/* Translate azimuth packet to integers without sign */
	 
	b0 = (unsigned char) returnstr[4];
	b1 = (unsigned char) returnstr[5];
	b2 = (unsigned char) returnstr[6];
	b3 = (unsigned char) returnstr[7];
		 

	/* Compute an unsigned azimuth encoder count */

	mtrazm = b1 + 256 * b2 + 256 * 256 * b3;

	/* Always complement the azimuth when the sign bit set */

	if (mtrazm > 8388608)
	{
		mtrazm = -(16777217 - mtrazm);
	}

	/* There is an	ambiguity on the mtralt encoder */
	/* It does not cover the full range of declination motion */

	if ((mtralt > 8388608))
	{
		mtralt = -(16777217 - mtralt);
	}
						
	/* Translate mount altitude packet to integers without sign */
	
	b0 = (unsigned char) returnstr[8];
	b1 = (unsigned char) returnstr[9];
	b2 = (unsigned char) returnstr[10];
	b3 = (unsigned char) returnstr[11];
	
	/* Compute an unsigned encoder count */
	
	mntalt = b1 + 256 * b2 + 256 * 256 * b3;

	/* Complement when sign bit set */

	if (mntalt > 8388608)
	{
		mntalt = -(16777217 - mntalt);
	}	 
	
	/* Translate mount azimuth packet to integers without sign */
	 
	b0 = (unsigned char) returnstr[12];
	b1 = (unsigned char) returnstr[13];
	b2 = (unsigned char) returnstr[14];
	b3 = (unsigned char) returnstr[15];
		 
	/* Compute an unsigned encoder count */

	mntazm = b1 + 256 * b2 + 256 * 256 * b3;

	if (mntazm > 8388608)
	{
		mntazm = -(16777217 - mntazm);
	}
				
	/* Calibration diagnostic */

	/* fprintf(stderr,"%d	%d	%d	%d	\n",mtrazm,mntazm,mtralt,mntalt); */
	
	
	/* Convert counts to degrees for all encoders						 */
	/* Scaling values of countperdeg are signed							 */
	/* Pointing angles now meet the software sign convention	*/
	
	mtrazmdeg = ( (double) mtrazm ) / mtrazmcal;
	mtraltdeg = ( (double) mtralt ) / mtraltcal;
	mntazmdeg = ( (double) mntazm ) / mntazmcal;
	mntaltdeg = ( (double) mntalt ) / mntaltcal;
			
	/* Transform mount pointing angles to ha, ra and dec						 */
	/* Assume sign convention and zero point for various mount types */
	
	if (telmount == GEM)
	{
			if (mntazmdeg == 0. ) 
			{
				if (mntaltdeg < 0.)
				{
					telha0 = -6.;
					teldec0 = 90. + mntaltdeg;
				}
				else
				{
					telha0 = +6.;
					teldec0 = 90. - mntaltdeg;
				}
			}
			else if (mntazmdeg == -90.)
			{
				if (mntaltdeg < 0.)
				{
					telha0 = 0.;
					teldec0 = 90. + mntaltdeg;
				}
				else
				{
					telha0 = -12.;
					teldec0 = 90. - mntaltdeg;
				}		
			}
			else if ( mntazmdeg == 90. )
			{
				if ( mntaltdeg > 0.)
				{
					telha0 = 0.;
					teldec0 = 90. - mntaltdeg;
				}
				else
				{
					telha0 = -12.;
					teldec0 = 90. + mntaltdeg;
				}		
			}	 
			else if ((mntazmdeg > -180.) && (mntazmdeg < -90.))
			{
				teldec0 = 90. - mntaltdeg;
				telha0 = Map12 (6. + mntazmdeg/15.);
			}
			else if ((mntazmdeg > -90.) && (mntazmdeg < 0.))
			{
				teldec0 = 90. - mntaltdeg;
				telha0 = Map12 (6. + mntazmdeg/15.);
			}		
			else if ((mntazmdeg > 0.) && (mntazmdeg < 90.))
			{
				teldec0 = 90. + mntaltdeg;
				telha0 = Map12 (-6. + mntazmdeg/15.);	
			}		
			else if ((mntazmdeg > 90.) && (mntazmdeg < 180.))
			{
				teldec0 = 90. + mntaltdeg;
				telha0 = Map12 (-6. + mntazmdeg/15.);
			}	 
			else
			{
				logStream (MESSAGE_ERROR) << "HA encoder out of range" << sendLog;			
				teldec0 = 0.;
				telha0 = 0.;
			}	 
		
			/* Flip signs for the southern sky */
		
			if (SiteLatitude < 0.) 
			{
				teldec0 = -1. * teldec0;
				telha0 = -1. * telha0;
			}
				
			telra0 = Map24 (LSTNow() - telha0); 
	}
		
	else if (telmount == EQFORK)
	{
			teldec0 = mntaltdeg;
			telha0 = Map12 (mntazmdeg / 15.);
		
			/* Flip signs for the southern sky */
		
			if (SiteLatitude < 0.)
			{
				teldec0 = -1. * teldec0;
				telha0 = -1. * telha0;
			}
				
			telra0 = Map24(LSTNow() - telha0);		
	}
	
	
	else if (telmount == ALTAZ)
	{
		HorizontalToEquatorial (mntazmdeg, mntaltdeg, &telha0, & teldec0);
		telha0 = Map12 (telha0);
		telra0 = Map24 (LSTNow() - telha0);
	}
		
	/* Handle special case if not already treated where dec is beyond a pole */
	
	if (teldec0 > 90.)
	{
		teldec0 = 180. - teldec0;
		telra0 = Map24(telra0 + 12.);
	}
	else if (teldec0 < -90.)
	{
		teldec0 = -180. - teldec0;
		telra0 = Map24(telra0 + 12.);
	}
		
	/* Apply pointing model to the coordinates that are reported by the telescope */
	
	PointingFromTel(&telra1, &teldec1, telra0, teldec0);
			
	/* Return corrected values */

	*telra=telra1;
	*teldec=teldec1;

	/* Diagnostics */

	/* fprintf(stderr,"mtraltdeg: %lf	mtrazmdeg: %lf\n", mtraltdeg, mtrazmdeg);	*/

	/* fprintf(stderr,"mntaltdeg: %lf mntazmdeg: %lf\n", mntaltdeg, mntazmdeg);	*/
	return;
}

/* Get guide targets for different pointings                        */
/* Returns targets with correct sense on the sky for N, S, E, and W */
/* With a GEM it follows pier side swap based on HA encoder reading */

void Sitech::GetGuideTargets(int *ntarget, int *starget, int *etarget, int *wtarget)
{
  double latitude, colatitude;
  int ntarget1, starget1, etarget1, wtarget1;
  
  latitude = fabs(SiteLatitude);
  colatitude = 90. - latitude;
  
  /* Depends on global mtrazmdeg and mtraltdeg set in GetTel            */
  /* First set limits for northern hemisphere                           */
  /* Then change assignments for southern hemisphere if needed          */
  
  /* Sign convention for mnt and  mtr degrees is:                       */
  /*   altdeg = 0 at the pole                                           */
  /*   azmdeg = 0 over the top of the GEM                               */
  /*   altdeg <0 going south from the pole for OTA west looking east    */
  /*   azmdeg <0 for rotation toward the east from above the GEM        */
  
  /* Parameters are set based on signed mtrazmcal and mtraltcal         */
  /* For our A200HR mtrazmcal is >0 and mtraltcal is <0                 */
  
  if (telmount == GEM)
  {
    /* Handle limits for different sides of the pier */
        
    if (mntazmdeg >= 0.)
    {
      /* Telescope west of pier pointed east  */
            
      wtarget1 = 100.*mtrazmcal;
      etarget1 = -10.*mtrazmcal;
      
      if (mntaltdeg <= 0.)
      {
        /* Telescope pointed between pole and south horizon */
                
        ntarget1 = 10.*mtraltcal;
        starget1 = -1.*(90. + colatitude)*mtraltcal;
      }
      else
      {
        /* Telescope pointed beyond the pole */
                
        ntarget1 = -1.*(90. + colatitude)*mtraltcal;
        starget1 = latitude*mtraltcal;
      }  
    }
    else
    {
      /* Telescope east of pier pointed west */
                  
      wtarget1 = 10.*mtrazmcal;
      etarget1 = -100.*mtrazmcal;
      
      if (mntaltdeg > 0.)
      {
        /* Telescope pointed between pole and south horizon */
        
        ntarget1 = -10.*mtraltcal;
        starget1 = (90. + colatitude)*mtraltcal;
      }
      else
      {
        /* Telescope pointed beyond the pole */
        
        ntarget1 = (90. + colatitude)*mtraltcal;
        starget1 = -1.*latitude*mtraltcal;
      } 
    }    
  }
  else if (telmount == EQFORK)
  { 
    /* Should be refined for specific installation */
    /* Here set to allow going  through fork to point under the pole */
         
    ntarget1 = 10.*mtraltcal;
    starget1 = -1.*(90. + colatitude)*mtraltcal;
    etarget1 = -100.*mtrazmcal;  
    wtarget1 = 100.*mtrazmcal;
  }
  else
  {
    /* Should be refined for specific installation */
    /* Here set to allow  going through azimuth 0 */
    
    ntarget1 = 90.*mtraltcal;
    starget1 = -90.*mtraltcal;
    etarget1 = -190.*mtrazmcal;  
    wtarget1 = 190.*mtrazmcal;  
  }

  if (SiteLatitude >= 0.)
  {
    /* Northern hemisphere */
    
    *ntarget = ntarget1;
    *starget = starget1;
    *etarget = etarget1;
    *wtarget = wtarget1;
  }
  else
  {
    /* Southern hemisphere */
    /* Counter directions are reversed */
    
    *ntarget = starget1;
    *starget = ntarget1;
    *etarget = wtarget1;
    *wtarget = etarget1;        
  }


  return;
}



Sitech::Sitech (int argc, char **argv):Telescope (argc,argv)
{
    //SiteLatitude = LATITUDE;        /* Latitude in degrees + north */  
    //SiteLongitude = LONGITUDE;      /* Longitude in degrees + west */ 
    SiteTemperature= TEMPERATURE;
    SitePressure= PRESSURE;
        
    telmount = GEM;                   /* One of GEM, EQFORK, ALTAZ          */
    homera = 0.;                    /* Startup RA reset based on HA       */
    homedec = 0.;                   /* Startup Dec                        */
    parkra = PARKRA;                /* Park telescope        */
    parkdec = PARKDEC;
    offsetha=0.;
    offsetdec=0.;

    pmodel=RAW;
    mtrazmcal = MOTORAZMCOUNTPERDEG; 
    mtraltcal = MOTORALTCOUNTPERDEG;
    mntazmcal = MOUNTAZMCOUNTPERDEG;
    mntaltcal = MOUNTALTCOUNTPERDEG;


    /* Global encoder counts */
    mtrazm = 0;
    mtralt = 0;
    mntazm = 0;
    mntalt = 0;

   /* Global pointing angles derived from the encoder counts */

    mtrazmdeg = 0.;
    mtraltdeg = 0.;
    mntazmdeg = 0.;
    mntaltdeg = 0.;
 
    /* Global tracking parameters */
    azmtrackrate0   = 0;
    alttrackrate0   = 0;
    azmtracktarget  = 0;
    azmtrackrate    = 0;
    alttracktarget  = 0;
    alttrackrate    = 0;
	
    TelConnectFlag = FALSE;

    device_file = "/dev/ttyUSB0";

    addOption ('f', "device_file", 1, "device file (ussualy /dev/ttySx");
}


/*!
 * Init telescope, connect on given tel_desc.
 *
 * @return 0 on succes, -1 & set errno otherwise
 */
int Sitech::init ()
{
	int ret;
	char returnstr[256] = "";
	int numread;
	int limits, flag;


	ret = Telescope::init ();
	if (ret)
	    return ret;

        
	if (TelConnectFlag != FALSE)
	{
	   return 0;
	}

	

  
	/* Software allows for different mount types but the A200HR requires "GEM" */
	/* GEM:    over the pier pointing at the pole                              */
	/* EQFORK: pointing at the equator on the meridian                         */
	/* ALTAZ:  level and pointing north                                        */

	if (telmount != GEM)
	{
		logStream(MESSAGE_ERROR)<<"Request made to connect A200HR controller to the wrong mount type."<<sendLog;
		return -1;
	}
	else
	{
		fprintf(stderr, "On a cold start A200HR must be over the pier\n");
		fprintf(stderr, "  pointing at the pole.\n\n");
		fprintf(stderr, "We always read the precision encoder and \n"); 
		fprintf(stderr, "  set the motor encoders to match.\n");
	}
	/* Make the connection */
	
	/* On the Sidereal Technologies controller                                */
	/*   there is an FTDI USB to serial converter that appears as             */
	/*   /dev/ttyUSB0 on Linux systems without other USB serial converters.   */
	/*   The serial device is known to the program that calls this procedure. */
	/* TelPortFD = open\("/dev/ttyUSB0",O_RDWR); */

	
	serConn = new rts2core::ConnSerial (device_file, this, rts2core::BS19200, rts2core::C8, rts2core::NONE, 5, 5);
	ret = serConn->init ();

	if (ret)
		return -1;
	serConn->flushPortIO ();
	
	
	/* Test connection by asking for firmware version    */
  	if(serConn->writePort("XV\r",3)==-1)
	    return -1;
	
     
	numread=serConn->readPort(returnstr,4);
	if (numread !=0 ) 
	{
	  returnstr[numread] = '\0';
	  fprintf(stderr,"Planewave A200HR\n");
	  fprintf(stderr,"Sidereal Technology %s\n",returnstr);
	  fprintf(stderr,"Telescope connected \n");  
	  TelConnectFlag = TRUE;   
	}
	else
	{
	    logStream(MESSAGE_ERROR)<<"A200HR drive control did not respond."<<sendLog;
	    return -1;
	}  

	/* Flush the input buffer */

	serConn->flushPortIO ();
  
	/* Set global tracking parameters */
          
	/* Set rates for defaults */
    
	azmtrackrate0 = AZMSIDEREALRATE;
	alttrackrate0 = ALTSIDEREALRATE;
	azmtrackrate = 0;
	alttrackrate = 0;
       
	/* Perform startup tests for slew limits if any */

	flag = GetLimits(&limits);
	usleep(500000);
	limits = FALSE;
	flag = SetLimits(limits);
	usleep(500000);  
	flag = GetLimits(&limits);
	flag = SyncTelEncoders();
	if (flag != TRUE)
	{
	   logStream(MESSAGE_ERROR)<<"Initial telescope pointing request was out of range ..."<<sendLog;
	   logStream(MESSAGE_ERROR)<<"Cycle power, set telescope to home position, restart."<<sendLog;
	   return -1;
	}
  
	/* Pause for telescope controller to initialize */
   
	usleep(500000);
  
	/* Read encoders and confirm pointing */
  
	GetTel(&homera, &homedec); 
  
	fprintf(stderr, "Mount type: %d\n", telmount);
	fprintf(stderr, "Mount motor encoder RA: %lf\n", homera);
	fprintf(stderr, "Mount motor encoder Dec: %lf\n", homedec);
  

	logStream(MESSAGE_INFO)<<"The telescope is on line .."<<sendLog;
    
	/* Flush the input buffer in case there is something left from startup */

	serConn->flushPortIO();
	return 0;

}

Sitech:: ~Sitech(void)
{delete serConn;
TelConnectFlag=FALSE;
}

int Sitech::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'f':
			device_file = optarg;
			break;

		default:
			return Telescope::processOption (in_opt);
	}
	return 0;
}



int Sitech:: initValues()
{
	rts2core::Configuration *config;
	config = rts2core::Configuration::instance ();
	config->loadFile ();
	telLatitude->setValueDouble (config->getObserver ()->lat);
	telLongitude->setValueDouble (config->getObserver ()->lng);
	telAltitude->setValueDouble (config->getObservatoryAltitude ());
	SiteLatitude = telLatitude->getValueDouble();
	SiteLongitude= telLongitude->getValueDouble();
	return Telescope::initValues ();
}


int Sitech::startResync ()
{
	CheckGoTo (getTelTargetRa (), getTelTargetDec ()); 
	return 0;
}


/* Test whether the destination was reached                  */
/* Return 0 if underway                                      */
/* Return 1 if done                                          */
/* Return 2 if outside tolerance                             */

int Sitech::CheckGoTo(double newra, double newdec)
{

  int status;
  double telra, teldec;
  double tolra, toldec;
  double raerr, decerr;

  status = GoToCoords(newra, newdec);

  if (status == 1)
  {
    /* Slew is in progress */
    return(0);
  }
  
  StartTrack();
  
  /* Get the telescope coordinates now */
  
  GetTel(&telra, &teldec);
  
  /* Find pointing errors in seconds of arc for both axes   */

  /* Allow for 24 hour ra wrap and trap dec at the celestial poles */
  
  raerr  = 3600.*15.*Map12(telra - newra);
  if (fabs(teldec) < 89. )
  {
    decerr = 3600.*(teldec - newdec);
  }
  else
  {
    decerr = 0.;
  }
  /* Slew tolerances in seconds of arc */
  
  tolra = SLEWTOLRA;
  toldec = SLEWTOLDEC;

  /* Within tolerance? */

  if ( ( fabs(raerr) < tolra ) && ( fabs(decerr) < toldec ) )
  {
    logStream(MESSAGE_INFO)<<"Goto completed within tolerance"<<sendLog;
    return(1);
  }
  else
  {    
    logStream(MESSAGE_INFO)<<"Goto completed outside tolerance"<<sendLog;
    return(2);
  }  
  
}  





/* Go to new celestial coordinates                                            */
/* Based on CenterGuide algorithm rather than controller goto function        */
/* Evaluate if target coordinates are valid                                   */
/* Test slew limits in altitude, polar, and hour angles                       */
/* Query if target is above the horizon                                       */
/* Return without action for invalid requests                                 */
/* Interrupt any slew sequence in progress                                    */
/* Check current pointing                                                     */
/* Find encoder readings required to point at target                          */
/* Repeated calls are required when more than one segment is needed           */


int Sitech::GoToCoords(double newra, double newdec)
{
  char slewcmd[32] = ""; 
  int nsend;
  double telra, teldec;
  double telha;
  double newha, newalt, newaz;
  double raerr, decerr;
  double rarate, decrate;
  double rarate0, decrate0;
  double deltime;
  double tolra, toldec;
  int ntarget, starget, etarget, wtarget;

  newha = LSTNow() - newra;
  newha = Map12(newha);
  
  /* Convert HA and Dec to Alt and Az */
  /* Test for visibility              */
  
  EquatorialToHorizontal(newha, newdec, &newaz, &newalt);
  
  /* Check altitude limit */
  
  if (newalt < MINTARGETALT)
  {
    StartTrack();
    logStream(MESSAGE_ERROR)<<"Target is below the telescope horizon";
    return(0);
  }

  /* Set directions to target */
  
  GetGuideTargets(&ntarget, &starget, &etarget, &wtarget);
  
  /* Set the time allowed for slew                                     */
  /* This time should be greater than or equal to the polling interval */
  
  deltime = 1.;
   
  /* Get the telescope coordinates now */
  
  GetTel(&telra, &teldec);
  
  telha = LSTNow() - telra;
  telha = Map12(telha);
  
  /* Find pointing errors in seconds of arc for both axes   */

  /* Allow for 24 hour ra wrap and trap dec at the celestial poles */
  
  raerr  = 3600.*15.*Map12(telra - newra);
  if (fabs(teldec) < 89. )
  {
    decerr = 3600.*(teldec - newdec);
  }
  else
  {
    decerr = 0.;
  }

  /* Slew tolerances in seconds of ard */
  
  tolra = SLEWTOLRA;
  toldec = SLEWTOLDEC;

  /* Within tolerance? */

  if ( ( fabs(raerr) < tolra ) && ( fabs(decerr) < toldec ) )
  {
    FullStop();
    usleep(1000000);
    StartTrack();
    fprintf(stderr,"Slew completed within tolerance\n");
    usleep(1000000);
    return(0);
  }  

  /* Check GEM mount basis and refine target if needed */
  
  if ( telmount == GEM )
  {
    if ( telha*newha < 0. )
    {
      if ( newha > 0. )
      {
        newha = +0.1;
        newdec = SiteLatitude;
      }
      else
      {
        newha = -0.1;
        newdec = SiteLatitude;
      }
      fprintf(stderr,"HA: %f (Tel)  %f (Target)\n", telha, newha);
      fprintf(stderr,"Changing mount basis ...\n");
    }  
  }  
    

  /* Calculate new signed rates based on these errors                 */
    
  /* Set the default axis slew rates to the azimuth sidereal rate */
  
  rarate0  = azmtrackrate0;
  decrate0 = azmtrackrate0;
  
  /* Select fast slew command if allowed */
  /* Not recommended:  places larger inertial loads on the gear train */

  if ( SLEWFAST )
  {
    rarate0  = 2.*rarate0;
    decrate0 = 2.*decrate0;
  }
  
  /* Calculate a new rate based on 15 arcseconds/second sidereal rate    */
  /* At this rate the telescope will reach the target in 5 deltimes      */
  
  rarate = rarate0 + 0.2*rarate0*(raerr/15.)*(1./deltime);
  decrate = -0.2*decrate0*(decerr/15.)*(1./deltime);
  
  /* Limit rates to 150 times sidereal */
  /* Large slew angles will require repeated calls to GoToCoords */
    
  if (rarate > 150.*rarate0)
  {
    rarate = 150.*rarate0;
  }
  else if (rarate < -150.*rarate0)
  {
    rarate = -150.*rarate0;
  }
  
  if (decrate > 150.*rarate0)
  {
    decrate = 150.*rarate0;
  }
  else if (decrate < -150.*rarate0)
  {
    decrate = -150.*rarate0;
  }  
  
  /* Convert to integer rates for drive controller */  
  
  azmtrackrate = (int) rarate;
  azmtracktarget = wtarget;
  if ( azmtrackrate < 0 )
  {
    azmtrackrate = -1*azmtrackrate;
    azmtracktarget = etarget;
  }
  
  alttrackrate = (int) decrate;
  alttracktarget = ntarget;
  if ( alttrackrate < 0 )
  {
    alttrackrate = - 1*alttrackrate;
    alttracktarget = starget;  
  }
  
  /* Use the ASCII set velocity command for the azimuth (Y) axis */
  /* Issue ra corrections if raflag is true */

  sprintf(slewcmd,"Y%dS%d\r",azmtracktarget,azmtrackrate);  
  nsend = strlen(slewcmd);
  serConn->flushPortIO ();
  serConn->writePort(slewcmd,nsend);  
  usleep(100000);
  sprintf(slewcmd,"X%dS%d\r",alttracktarget,alttrackrate); 
  nsend = strlen(slewcmd);
  serConn->flushPortIO ();
  serConn->writePort(slewcmd,nsend);  
  usleep(100000);
  
  /* A slew is in progress */

  fprintf(stderr,"A slew is in progress\n");
      
  return(1);
}
 
void Sitech::StartTrack(void)
{
  
  char slewcmd[32] = ""; 
  int nsend;
  int ntarget, starget, etarget, wtarget;
  
  GetGuideTargets(&ntarget, &starget, &etarget, &wtarget);
  
  /* Default rates and targets should be set elsewhere to reasonable values */
  /* CenterGuide will update rates based on mount encoder readings          */
  /* A call to StartTrack will restore default rates                        */
  /* For an equatorial mount it sets                                        */
  /* RA at the sidereal rate and Dec off                                    */
    
  if (telmount != ALTAZ)
  {
  
    /* Start azimuth (RA) axis tracking at sidereal rate                    */
    /* Use the ASCII set velocity command for the azimuth (Y) axis          */
        
    sprintf(slewcmd,"Y%dS%d\r",wtarget,azmtrackrate0);  
    nsend = strlen(slewcmd);
    serConn->flushPortIO ();
    serConn->writePort(slewcmd,nsend);
    
    usleep(100000);
    
    /* Turn off any altitude (Dec) axis tracking */
    
    sprintf(slewcmd,"XS0\r");  
    nsend = strlen(slewcmd);
    serConn->flushPortIO (); 
    serConn->writePort(slewcmd,nsend);  
    
  }
  else
  {  
    /* This branch would not be executed for an A200HR mount             */
    /* It may be useful for other applications of the Sitech  controller */
    /* Set the tracking rates and targets based on altitude and azimuth  */
        
    /* Code to start triaxial tracking goes here                         */
  }
  
  return;
} 



/*** Check if telescope is moving. Called during telescope
 * movement to detect if the target destination was reached.
 * @return -2 when destination was reached, -1 on failure, >= 0
 * return value is number of milliseconds for next isMoving
 * call.
 */

int Sitech::isMoving ()
{
	int ret;
	ret=CheckGoTo(getTelTargetRa (), getTelTargetDec ());
	switch (ret)
	{
	  case 1:
	      return -2;
	  case 2:
	      return -1;
	  default:
	    return USEC_SEC;
	}
}

int Sitech::startPark()
{
  GoToCoords(parkra,parkdec);
  return 0;
}

int main (int argc, char **argv)
{	
	Sitech device = Sitech (argc, argv);
	return device.run ();
}
