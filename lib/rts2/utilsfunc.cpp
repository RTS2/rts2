/*
 * Various utility functions.
 * Copyright (C) 2003-2008 Petr Kubanek <petr@kubanek.net>
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

#include "utilsfunc.h"
#include "status.h"
#include "riseset.h"

#include <errno.h>
#include <ftw.h>
#ifdef RTS2_HAVE_MALLOC_H
#include <malloc.h>
#endif
#include <iostream>
#include <cmath>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

double random_num ()
{
#ifndef sun
	return (double) random () / RAND_MAX;
#else
	return (double) random () / INT_MAX;
#endif
}

void random_salt (char *buf, int len)
{
	for (; len > 0; len--, buf++)
	{
		// 2 * 25 + 10 + 2
#ifndef sun
		int rn = random () * 62.0 / RAND_MAX;
#else
		int rn = random () * 62.0 / INT_MAX;
#endif
		if (rn < 12)
			*buf = '.' + rn;
		else if (rn < 37)
			*buf = 'A' - 12 + rn;
		else
			*buf = 'a' - 37 + rn;
	}
}

int mkpath (const char *path, mode_t mode)
{
	char *cp_path;
	char *start, *end;
	int ret = 0;
	cp_path = strdup (path);
	start = cp_path;
	while (1)
	{
		end = ++start;
		while (*end && *end != '/')
			end++;
		if (!*end)
			break;
		*end = '\0';
		ret = mkdir (cp_path, mode);
		*end = '/';
		start = ++end;
		if (ret)
		{
			if (errno != EEXIST)
				break;
			ret = 0;
		}
	}
	free (cp_path);
	return ret;
}

// remove file or directory
int rmfiledir (const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf)
{
	switch (typeflag)
	{
		case FTW_D:
		case FTW_DP:
			return rmdir (fpath);
		case FTW_F:
			return unlink (fpath);
		default:
			errno = ENOTSUP;
			return -1;
	}
}

int rmdir_r (const char *dir)
{
	return nftw (dir, rmfiledir, 50, FTW_DEPTH | FTW_MOUNT);
}

int parseLocalDate (const char *in_date, struct ln_date *out_time, bool &islocal, bool *only_date)
{
	int ret;
	int ret2;
	out_time->hours = out_time->minutes = 0;
	out_time->seconds = 0;
	while (*in_date && isblank (*in_date))
		in_date++;
	islocal = false;

	if (only_date)
		*only_date = false;

	if (in_date[0] == '+' || in_date[0] == '-')
	{
		time_t now;
		time (&now);
		now += atoi (in_date);
		ln_get_date_from_timet (&now, out_time);
		return 0;
	}

	char ch1, ch2;

	ret = sscanf (in_date, "%d%c%d%c%d%n", &out_time->years, &ch1, &out_time->months, &ch2, &out_time->days, &ret2);
	if (ret == 5 && ((ch1 == '-' && ch2 == '-') || (ch1 == '/' && ch2 == '/')))
	{
		in_date += ret2;

		// we end with is U for UT, let's check if it contains time..
		if (*in_date == 'T' || *in_date == 'L' || *in_date == 'G' || *in_date == 'U' || isspace (*in_date))
		{
			// localtime
			if (*in_date == 'T' || *in_date == 'L' || isspace (*in_date))
				islocal = true;
			in_date++;
			ret2 = 	sscanf (in_date, "%u:%u:%lf", &out_time->hours,	&out_time->minutes, &out_time->seconds);
			if (ret2 == 3)
				return 0;
			ret2 = 	sscanf (in_date, "%u:%u", &out_time->hours, &out_time->minutes);
			if (ret2 == 2)
				return 0;
			ret2 = sscanf (in_date, "%d", &out_time->hours);
			if (ret2 == 1)
				return 0;
			return -1;
		}
		// only year..
		if (*in_date == '\0')
		{
			if (only_date)
				*only_date = true;
			islocal = true;
			return 0;
		}
	}
	return -1;
}

int parseDate (const char *in_date, struct ln_date *out_time, bool forceUT, bool *only_date)
{
	bool islocal;
	int ret = parseLocalDate (in_date, out_time, islocal, only_date);
	if (forceUT == false && islocal)
	{
		double JD = ln_get_julian_day (out_time);
		struct timeval tv;
		struct timezone tz;
		gettimeofday (&tv, &tz);
		JD += tz.tz_minuteswest / 1440.0;
		ln_get_date (JD, out_time);
		if (out_time->seconds < 0.001)
			out_time->seconds = 0;
	}
	return ret;
}


int parseDate (const char *in_date, double &JD, bool forceUT, bool *only_date)
{
	struct ln_date l_date;
	int ret;
	ret = parseDate (in_date, &l_date, forceUT, only_date);
	if (ret)
		return ret;
	JD = ln_get_julian_day (&l_date);
	return 0;
}

int parseDate (const char *in_date, time_t *out_time, bool forceUT, bool *only_date)
{
	int ret;
	struct ln_date l_date;
	ret = parseDate (in_date, &l_date, forceUT, only_date);
	if (ret)
		return ret;
	ln_get_timet_from_julian (ln_get_julian_day (&l_date), out_time);
	return 0;
}

void getDateObs (const time_t t, const suseconds_t usec, char buf[25])
{
	struct tm t_tm;
	gmtime_r (&t, &t_tm);
	strftime (buf, 25, "%Y-%m-%dT%H:%M:%S.", &t_tm);
	snprintf (buf + 20, 4, "%03li", usec / 1000);
}

std::string getDateObs (const time_t t, const suseconds_t usec)
{
	char buf[25];
	getDateObs (t, usec, buf);
	return std::string (buf);
}

std::vector<std::string> SplitStr(const std::string& text, const std::string& delimeter)
{
	std::size_t pos = 0;
	std::size_t oldpos = 0;
	std::size_t delimlen = delimeter.length();

	std::vector<std::string> result;

	if (text.empty ())
		return result;

	while (pos != std::string::npos)
	{
		pos = text.find(delimeter, oldpos);
		if (pos - oldpos == 0)
		{
			// / is the only character..
			if (text.length () == 1)
				break;
			oldpos += delimlen;
			continue;
		}
		result.push_back(text.substr(oldpos, pos - oldpos));
		oldpos = pos + delimlen;
		// last character..
		if (pos == text.length () - 1)
			break;
	}

	return result;
}

std::vector<char> Str2CharVector (std::string text)
{
	std::vector<char> res;
	for (std::string::iterator iter = text.begin(); iter != text.end(); iter++)
	{
		res.push_back (*iter);
	}
	return res;
}

std::vector <int> parseRange (const char *range_str, int array_size, const char *& endp)
{
	int from = 0;
	int to = 0;
	enum {FROM, TO, ARRAY_LEFT} state = FROM;
	std::vector <int> ret;
	endp = range_str;
	while (*endp)
	{
		if (isdigit (*endp))
		{
			switch (state)
			{
				case ARRAY_LEFT:
					state = FROM;
				case FROM:
					from = from * 10 + (*endp) - '0';
					break;
				case TO:
					to = to * 10 + (*endp) - '0';
					break;
			}
		}
		else if (*endp == ':')
		{
			switch (state)
			{
				case FROM:
				case ARRAY_LEFT:
					state = TO;
					break;
				case TO:
					return ret;
			}
		}
		else if (*endp == ',')
		{
			if (from > 0)
				from--;
			if (to == 0 || to > array_size)
			{
				if (state == TO)
					to = array_size;
				else
					to = to > array_size ? from : from + 1;
			}
			for ( ; from < to; from++)
				ret.push_back (from);
			from = to = 0;
			state = ARRAY_LEFT;
		}
		else
		{
			return ret;
		}
		endp++;
	}
	if (endp == range_str)
	{
		for ( ; from < array_size; from++)
			ret.push_back (from);
	}
	else if (state == TO || from == 0 || (state == FROM && from > 0))
	{
		if (from > 0)
			from--;
		if (to == 0 || to > array_size)
		{
			if (state == TO)
				to = array_size;
			else
				to = to > array_size ? from : from + 1;
		}
		for ( ; from < to; from++)
			ret.push_back (from);
	}
	return ret;
}

int charToBool (const char *in_value, bool &ret)
{
	if (!strcasecmp (in_value, "ON") || !strcasecmp (in_value, "TRUE")
		|| !strcasecmp (in_value, "YES") || !strcmp (in_value, "1"))
		ret = true;
	else if (!strcasecmp (in_value, "OFF") || !strcasecmp (in_value, "FALSE")
		|| !strcasecmp (in_value, "NO") || !strcmp (in_value, "0"))
		ret = false;
	else
		return -1;
	return 0;
}

#ifndef RTS2_HAVE_ISINF
int isinf(double x)
{
	return !finite(x) && x==x;
}
#endif

#ifndef RTS2_HAVE_STRCASESTR
char * strcasestr(const char * haystack, const char * needle)
{
	const char *p = haystack;
	int len = strlen (needle);
	while (*p)
	{
		if (strncasecmp (haystack, needle, len))
			return (char *) p;
		p++;
	}
	return NULL;
}
#endif // RTS2_HAVE_STRCASESTR

#ifndef RTS2_HAVE_GETLINE
ssize_t getline(char **lineptr, size_t *n, FILE *stream)
{
	if (*lineptr == NULL)
		*lineptr = (char *) malloc (*n);
	char *ret = fgets (*lineptr, *n, stream);
	if (ret == NULL)
		return -1;
	return *n;
}
#endif

double getNow ()
{
	struct timeval infot;
	gettimeofday (&infot, NULL);
	return infot.tv_sec + (double) infot.tv_usec / USEC_SEC;
}

const char * multiWCS (const char *name, char multi_wcs)
{
	static char ret[50];
	strcpy (ret, name);
	if (multi_wcs != '\0')
	{
		int l = strlen (ret);
		ret[l] = multi_wcs;
		ret[l+1] = '\0';
	}
	return ret;
}

int db_nan_indicator (double value)
{
	return std::isnan (value) ? -1 : 0;
}

double db_nan_double (double value, int ind)
{
	return ind ? NAN : value;
}

float db_nan_float (float value, int ind)
{
	return ind ? NAN : value;
}

void getNight (time_t curr_time, struct ln_lnlat_posn *observer, double nightHorizon, time_t &nstart, time_t &nstop)
{
	time_t nt = curr_time;

	nstart = nt;
	nstop = nt;

	rts2_status_t call_state;
	rts2_status_t net;

	time_t t_start_t = nt + 1;

	next_event (observer, &t_start_t, &call_state, &net, &nt, nightHorizon, nightHorizon + 5, 1000, 1000);

	bool nsta = true;
	bool nsto = true;

	while (nt < (curr_time + 86400) && (nsta || nsto))
	{
		t_start_t = nt + 1;
		if (call_state == SERVERD_DUSK)
		{
			nstart = nt;
			nsta = false;
		}
		if (call_state == SERVERD_NIGHT)
		{
			nstop = nt;
			nsto = false;
		}
		next_event (observer, &t_start_t, &call_state, &net, &nt, nightHorizon, nightHorizon + 5, 1000, 1000);
	}
}

void normalizeRaDec (double &ra, double &dec)
{
	if (dec < -90)
	{
		dec = -180 - dec;
		ra = ra - 180;
	}
	else if (dec > 90)
	{
		dec = 180 - dec;
		ra = ra - 180;
	}
	ra = ln_range_degrees (ra);
}

float celsiusToFahrenheit (float c)
{
	return c * 1.8 + 32;
}

float fahrenheitToCelsius (float f)
{
	return (f - 32) / 1.8;
}

float kelvinToCelsius (float k)
{
	return k - 273.15;
}

float celsiusToKelvin (float c)
{
	return c + 273.15;
}

float mphToMs (float mph)
{
	return 0.4474 * mph;
}

std::string string_format(const char *fmt, ...)
{
	int size = strlen (fmt) * 2 + 50;   // Use a rubric appropriate for your code
	std::string str;
	va_list ap;
	while (1) {	 // Maximum two passes on a POSIX system...
		str.resize(size);
		va_start(ap, fmt);
		int n = vsnprintf((char *)str.data(), size, fmt, ap);
		va_end(ap);
		if (n > -1 && n < size) {  // Everything worked
			str.resize(n);
			return str;
		}
		if (n > -1)  // Needed size returned
			size = n + 1;   // For null char
		else
			size *= 2;	  // Guess at a larger size (OS specific)
	}
	return str;
}

#define CRC_16     0xA001			 /* crc-16 mask */

uint16_t getMsgBufCRC16 (char *msgBuf, int msgLen)
{
	uint16_t ret = 0xffff;
	for (int l = 0; l < msgLen; l++)
	{
		char ch = msgBuf[l];
		for (int i = 0; i < 8; i++)
		{
			if ((ret ^ ch) & 0x01)
				ret = (ret >> 1) ^ CRC_16;
			else
				ret >>= 1;
			ch >>= 1;
		}
	}
	return ret;
}

int parseVariableName (const char *name, char **device, char **variable)
{
	const char *sep = strchr (name, '.');
	if (sep == NULL)
		return -1;

	size_t len = sep - name + 1;
	*device = new char[len];
	memcpy (*device, name, len - 1);
	(*device)[len - 1] = '\0';

	len = strlen (name) - (sep - name);
	*variable = new char[len];
	memcpy (*variable, sep + 1, len - 1);
	(*variable)[len - 1] = '\0';
	return 0;
}

void parallacticAngle (double ha, double dec, double sin_lat, double cos_lat, double tan_lat, double &pa, double &parate)
{
	double radha = ln_deg_to_rad (ha);
	double raddec = ln_deg_to_rad (dec);
	double cos_dec = cos (raddec);
	double sin_dec = sin (raddec);
	double cos_ha = cos (radha);
	double sin_ha = sin (radha);
	double div = (sin_lat * cos_dec - cos_lat * sin_dec * cos_ha);
	// don't divide by 0
	if (fabs (div) < 10e-5)
		pa = 0;
	else
		pa = ln_rad_to_deg (atan2 (cos_lat * sin_ha, div));
	double par1 = (tan_lat * cos_dec - sin_dec * cos_ha);
	parate = (15 * (tan_lat * cos_dec * cos_ha - sin_dec) / (sin_ha * sin_ha + par1 * par1));
}

void sph2cart (double a, double b, double *xyz)
{
	double cb = cos (b);
	xyz[0] = cos (a) * cb;
	xyz[1] = sin (a) * cb;
	xyz[2] = sin (b);
}

void cart2sph (double *xyz, double &a, double &b)
{
	double r = sqrt (xyz[0] * xyz[0] + xyz[1] * xyz[1] + xyz[2] * xyz[2]);
	a = asin (xyz[2] / r);
	b = atan2 (xyz[1], xyz[0]);
}

double posangle (double *xyz0, double *xyz1)
{
	return atan2 (xyz1[1] * xyz0[0] - xyz1[0] * xyz0[1], xyz1[2] * ( xyz0[0] * xyz0[0] + xyz0[1] * xyz0[1] ) - xyz0[2] * ( xyz1[0] * xyz0[0] + xyz1[1] * xyz1[0] ));
}
