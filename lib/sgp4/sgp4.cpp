/* 
 * Simplified perturbations model
 * Copyright (C) 2016 Petr Kubanek <petr@kubanek.net>
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

#define EARTH_MAJOR_AXIS 6378140.
#define EARTH_MINOR_AXIS 6356755.
#define EARTH_AXIS_RATIO (EARTH_MINOR_AXIS / EARTH_MAJOR_AXIS)

#include <string.h>
#include <libnova/libnova.h>

#include "sgp4.h"
#include "sgp4unit.h"

/* -----------------------------------------------------------------------------
*
*                           procedure days2mdhms
*
*  this procedure converts the day of the year, days, to the equivalent month
*    day, hour, minute and second.
*
*  algorithm     : set up array for the number of days per month
*                  find leap year - use 1900 because 2000 is a leap year
*                  loop through a temp value while the value is < the days
*                  perform int conversions to the correct day and month
*                  convert remainder into h m s using type conversions
*
*  author        : david vallado                  719-573-2600    1 mar 2001
*
*  inputs          description                    range / units
*    year        - year                           1900 .. 2100
*    days        - julian day of the year         0.0  .. 366.0
*
*  outputs       :
*    mon         - month                          1 .. 12
*    day         - day                            1 .. 28,29,30,31
*    hr          - hour                           0 .. 23
*    min         - minute                         0 .. 59
*    sec         - second                         0.0 .. 59.999
*
*  locals        :
*    dayofyr     - day of year
*    temp        - temporary extended values
*    inttemp     - temporary int value
*    i           - index
*    lmonth[12]  - int array containing the number of days per month
*
*  coupling      :
*    none.
* --------------------------------------------------------------------------- */

void days2mdhms (int year, double days, int& mon, int& day, int& hr, int& minute, double& sec)
{
	int i, inttemp, dayofyr;
	double    temp;
	int lmonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

	dayofyr = (int)floor(days);
	/* ----------------- find month and day of month ---------------- */
	if ( (year % 4) == 0 )
		lmonth[1] = 29;

	i = 1;
	inttemp = 0;
	while ((dayofyr > inttemp + lmonth[i-1]) && (i < 12))
	{
		inttemp = inttemp + lmonth[i-1];
		i++;
	}
	mon = i;
	day = dayofyr - inttemp;

	/* ----------------- find hours minutes and seconds ------------- */
	temp = (days - dayofyr) * 24.0;
	hr   = (int)floor(temp);
	temp = (temp - hr) * 60.0;
	minute  = (int)floor(temp);
	sec  = (temp - minute) * 60.0;
}  // end days2mdhms

static inline double ThetaG (double jd)
{
	/* Reference:  The 1992 Astronomical Almanac, page B6. */
	const double omega_E = 1.00273790934;
	/* Earth rotations per sidereal day (non-constant) */
	const double UT = fmod (jd + .5, 1.);
	const double seconds_per_day = 86400.;
	const double jd_2000 = 2451545.0;   /* 1.5 Jan 2000 = JD 2451545. */
	double t_cen, GMST, rval;

	t_cen = (jd - UT - jd_2000) / 36525.;
	GMST = 24110.54841 + t_cen * (8640184.812866 + t_cen * (0.093104 - t_cen * 6.2E-6));
	GMST = fmod( GMST + seconds_per_day * omega_E * UT, seconds_per_day);
	if (GMST < 0.)
		GMST += seconds_per_day;
	rval = 2 * M_PI * GMST / seconds_per_day;
	return( rval);
} /*Function thetag*/

using namespace rts2sgp4;

int rts2sgp4::init (const char *tle1, const char *tle2, elsetrec *satrec)
{
	double tumin, mu, radiusearthkm, xke, j2, j3, j4, j3oj2;
	getgravconst (wgs84, tumin, mu, radiusearthkm, xke, j2, j3, j4, j3oj2);

	satrec->error = 0;

	char longstr1[130], longstr2[130];

	strcpy (longstr1, tle1);
	strcpy (longstr2, tle2);

	// set the implied decimal points since doing a formated read
	// fixes for bad input data values (missing, ...)
	int j;
	for (j = 10; j <= 15; j++)
		if (longstr1[j] == ' ')
			longstr1[j] = '_';

	if (longstr1[44] != ' ')
		longstr1[43] = longstr1[44];
	longstr1[44] = '.';
	if (longstr1[7] == ' ')
		longstr1[7] = 'U';
	if (longstr1[9] == ' ')
		longstr1[9] = '.';
	for (j = 45; j <= 49; j++)
		if (longstr1[j] == ' ')
			longstr1[j] = '0';
	if (longstr1[51] == ' ')
		longstr1[51] = '0';
	if (longstr1[53] != ' ')
		longstr1[52] = longstr1[53];
	longstr1[53] = '.';
	longstr2[25] = '.';
	for (j = 26; j <= 32; j++)
		if (longstr2[j] == ' ')
			longstr2[j] = '0';
	if (longstr1[62] == ' ')
		longstr1[62] = '0';
	if (longstr1[68] == ' ')
		longstr1[68] = '0';

	int cardnumb, numb;
	char classification, intldesg[11];
	int nexp, ibexp;
	long elnum = 0;

	int ret = sscanf (longstr1,"%2d %5ld %1c %10s %2d %12lf %11lf %7lf %2d %7lf %2d %2d %6ld ", &cardnumb, &satrec->satnum, &classification, intldesg, &satrec->epochyr, &satrec->epochdays, &satrec->ndot, &satrec->nddot, &nexp, &satrec->bstar, &ibexp, &numb, &elnum);
	if (ret != 13)
		return -1;

	if (longstr2[52] == ' ')
		ret = sscanf (longstr2,"%2d %5ld %9lf %9lf %8lf %9lf %9lf %10lf", &cardnumb, &satrec->satnum, &satrec->inclo, &satrec->nodeo, &satrec->ecco, &satrec->argpo, &satrec->mo, &satrec->no);
	else
		ret = sscanf (longstr2,"%2d %5ld %9lf %9lf %8lf %9lf %9lf %11lf", &cardnumb, &satrec->satnum, &satrec->inclo, &satrec->nodeo, &satrec->ecco, &satrec->argpo, &satrec->mo, &satrec->no);
	if (ret != 8)
		return -2;

	const double xpdotp   =  1440.0 / (2.0 *pi);  // 229.1831180523293

	// ---- find no, ndot, nddot ----
	satrec->no   = satrec->no / xpdotp; //* rad/min
	satrec->nddot= satrec->nddot * pow(10.0, nexp);
	satrec->bstar= satrec->bstar * pow(10.0, ibexp);

	// ---- convert to sgp4 units ----
	satrec->a    = pow (satrec->no*tumin , (-2.0/3.0));
	satrec->ndot = satrec->ndot  / (xpdotp*1440.0);  //* ? * minperday
	satrec->nddot= satrec->nddot / (xpdotp*1440.0*1440);

	// ---- find standard orbital elements ----
	satrec->inclo = ln_deg_to_rad (satrec->inclo);
	satrec->nodeo = ln_deg_to_rad (satrec->nodeo);
	satrec->argpo = ln_deg_to_rad (satrec->argpo);
	satrec->mo    = ln_deg_to_rad (satrec->mo);

	satrec->alta = satrec->a * (1.0 + satrec->ecco) - 1.0;
	satrec->altp = satrec->a * (1.0 - satrec->ecco) - 1.0;

	// ----------------------------------------------------------------
	// find sgp4epoch time of element set
	// remember that sgp4 uses units of days from 0 jan 1950 (sgp4epoch)
	// and minutes from the epoch (time)
	// ----------------------------------------------------------------

	struct ln_date epoch;
	epoch.years= 0;
	// ---------------- temp fix for years from 1957-2056 -------------------
	// --------- correct fix will occur when year is 4-digit in tle ---------
	if (satrec->epochyr < 57)
		epoch.years= satrec->epochyr + 2000;
	else
		epoch.years= satrec->epochyr + 1900;

	days2mdhms (epoch.years, satrec->epochdays, epoch.months, epoch.days, epoch.hours, epoch.minutes, epoch.seconds);
	satrec->jdsatepoch = ln_get_julian_day (&epoch);

	char opsmode = 0;
	sgp4init (wgs84, opsmode, satrec->satnum, satrec->jdsatepoch-2433281.5, satrec->bstar, satrec->ecco, satrec->argpo, satrec->inclo, satrec->mo, satrec->no, satrec->nodeo, *satrec);

	return 0;
}

int rts2sgp4::propagate (elsetrec *satrec, double JD, struct ln_rect_posn *ro, struct ln_rect_posn *vo)
{
	double t_since = JD - satrec->jdsatepoch;
	
	// time is in minutes..
	return sgp4 (wgs84, *satrec, t_since * 1440, ro, vo) == true ? 0 : -1;
}

void rts2sgp4::ln_get_satellite_ra_dec_delta (struct ln_rect_posn *observer_loc, struct ln_rect_posn *satellite_loc, struct ln_equ_posn *equ, double *delta)
{
	struct ln_rect_posn d;

	d.X = satellite_loc->X - observer_loc->X;
	d.Y = satellite_loc->Y - observer_loc->Y;
	d.Z = satellite_loc->Z - observer_loc->Z;

	*delta = sqrt (d.X * d.X + d.Y * d.Y + d.Z * d.Z);
   	equ->ra = atan2 (d.Y, d.X);
	equ->ra = ln_range_degrees (ln_rad_to_deg (equ->ra));
	equ->dec = asin (d.Z / *delta);
	equ->dec = ln_rad_to_deg (asin (d.Z / *delta));
}

void rts2sgp4::ln_lat_alt_to_parallax (struct ln_lnlat_posn *observer, double altitude, double *rho_cos, double *rho_sin)
{
	double lat = ln_deg_to_rad (observer->lat);
	const double u = atan (sin (lat) * EARTH_AXIS_RATIO / cos (lat));
	*rho_sin = EARTH_AXIS_RATIO * sin( u) + (altitude / EARTH_MAJOR_AXIS) * sin( lat);
	*rho_cos = cos (u) + (altitude / EARTH_MAJOR_AXIS) * cos (lat);
}

void rts2sgp4::ln_observer_cartesian_coords (struct ln_lnlat_posn *observer, double rho_cos, double rho_sin, double JD, struct ln_rect_posn *observer_loc)
{
	const double angle = ln_deg_to_rad (observer->lng) + ThetaG (JD);
	observer_loc->X = cos (angle) * rho_cos * EARTH_MAJOR_AXIS / 1000.;
	observer_loc->Y = sin (angle) * rho_cos * EARTH_MAJOR_AXIS / 1000.;
	observer_loc->Z = rho_sin               * EARTH_MAJOR_AXIS / 1000.;
}
