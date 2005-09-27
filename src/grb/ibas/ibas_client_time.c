
#include <ibas_client.h>


int ibc_utc_year_min = 1972;
int ibc_utc_month_min = 0;
int ibc_utc_year_max = 2020;
int ibc_utc_month_max = 0;
int ibc_month_days[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
int ibc_month_days_leap[12] =
  { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
IBC_LEAP_SEC_ENTRY ibc_tai_leap_sec[IBC_LEAP_SEC_TABLE_SIZE] =
  { {1972, 0, 10},
{1972, 6, 11},
{1973, 0, 12},
{1974, 0, 13},
{1975, 0, 14},
{1976, 0, 15},
{1977, 0, 16},
{1978, 0, 17},
{1979, 0, 18},
{1980, 0, 19},
{1981, 6, 20},
{1982, 6, 21},
{1983, 6, 22},
{1985, 6, 23},
{1988, 0, 24},
{1990, 0, 25},
{1991, 0, 26},
{1992, 6, 27},
{1993, 6, 28},
{1994, 6, 29},
{1996, 0, 30},
{1997, 6, 31},
{1999, 0, 32},
{IBC_LEAP_SEC_END_MARKER, 0, 0}
};


int
ibc_compare_ym_pair (int y1, int m1, int y2, int m2)
{
  if (y1 < y2)
    return (-1);
  if (y1 > y2)
    return (1);
  if (m1 < m2)
    return (-1);
  if (m1 > m2)
    return (1);
  return (0);
}


int
ibc_get_feb_days (int year)
{
  if (year & 3)
    return (28);
  if (0 == (year % 400))
    return (29);
  if (0 == (year % 100))
    return (28);
  return (29);
}


int
ibc_utc2tai2ert (IBC_UTC_TIME * utc, double *tai, IBC_ERT_TIME * ert)
{
  int my_days, nsec, cy, cm, i;
  int *ms;

  if (NULL == utc)
    return (IBC_NUL_PTR);

  if ((utc->month < 0) || (utc->month >= 12))
    return (IBC_BAD_ARG);
  if (ibc_compare_ym_pair
      (utc->year, utc->month, ibc_utc_year_min, ibc_utc_month_min) < 0)
    return (IBC_BAD_ARG);
  if (ibc_compare_ym_pair
      (utc->year, utc->month, ibc_utc_year_max, ibc_utc_month_max) >= 0)
    return (IBC_BAD_ARG);

  ms =
    ((29 ==
      ibc_get_feb_days (utc->
			year)) ? (&(ibc_month_days_leap[0]))
     : (&(ibc_month_days[0])));
  if ((utc->day < 0) || (utc->day >= ms[utc->month]))
    return (IBC_BAD_ARG);

  my_days = IBC_DAYS_1_1_72_FROM_EPOCH;
  for (i = 1972; i < utc->year; i++)
    my_days += 337 + ibc_get_feb_days (i);
  for (i = 0; i < utc->month; i++)
    my_days += ms[i];

  if ((utc->hour < 0) || (utc->hour >= 24))
    return (IBC_BAD_ARG);
  if ((utc->min < 0) || (utc->min >= 60))
    return (IBC_BAD_ARG);
  if ((utc->usec < 0) || (utc->usec >= 1000000))
    return (IBC_BAD_ARG);

  for (i = 0; i < (IBC_LEAP_SEC_TABLE_SIZE - 1); i++)
    {
      if ((ibc_compare_ym_pair
	   (utc->year, utc->month, ibc_tai_leap_sec[i].year,
	    ibc_tai_leap_sec[i].month) >= 0)
	  &&
	  (ibc_compare_ym_pair
	   (utc->year, utc->month, ibc_tai_leap_sec[i + 1].year,
	    ibc_tai_leap_sec[i + 1].month) < 0))
	{
	  nsec = 60;		/* min == 60 sec, but if this is last minute of the month */
	  if (((ms[utc->month] - 1) == utc->day) && (23 == utc->hour)
	      && (59 == utc->min))
	    {
	      cy = ibc_tai_leap_sec[i + 1].year;
	      cm = ibc_tai_leap_sec[i + 1].month - 1;
	      if (cm < 0)
		{
		  cy--;
		  cm = 11;
		}		/* and next month has an entry in leap second table */
	      if ((cy == utc->year) && (cm == utc->month))
		nsec += ibc_tai_leap_sec[i + 1].delta - ibc_tai_leap_sec[i].delta;	/* then this minute != 60 sec !!! */
	    }
	  if ((utc->sec < 0) || (utc->sec >= nsec))
	    return (IBC_BAD_ARG);

	  if (NULL != tai)
	    *tai =
	      (my_days + utc->day) * 86400 + utc->hour * 3600 +
	      utc->min * 60 + utc->sec + ibc_tai_leap_sec[i].delta +
	      utc->usec / 1e6;

	  if (NULL != ert)
	    {
	      ert->day = my_days + utc->day;
	      ert->msec =
		1000 * (utc->hour * 3600 + utc->min * 60 + utc->sec) +
		utc->usec / 1000;
	      ert->usec = utc->usec % 1000;
	    }

	  return (IBC_OK);
	}
      if (IBC_LEAP_SEC_END_MARKER == ibc_tai_leap_sec[i + 1].year)
	break;			/* should not happen, but who knows ... */
    }
  return (IBC_BAD_ARG);
}


int
ibc_ert2utc (IBC_ERT_TIME * ert, IBC_UTC_TIME * utc)
{
  int my_days, ys;
  int *ms;

  if (NULL == utc)
    return (IBC_NUL_PTR);
  if (NULL == ert)
    return (IBC_NUL_PTR);

  if (ert->day < IBC_DAYS_1_1_72_FROM_EPOCH)
    return (IBC_BAD_ARG);

  my_days = IBC_DAYS_1_1_72_FROM_EPOCH;
  utc->year = 1972;
  for (;;)
    {
      if (utc->year > ibc_utc_year_max)
	return (IBC_BAD_ARG);
      ys = 337 + ibc_get_feb_days (utc->year);
      if ((my_days + ys) > ert->day)
	break;
      my_days += ys;
      (utc->year)++;
    }

  ms =
    ((29 ==
      ibc_get_feb_days (utc->
			year)) ? (&(ibc_month_days_leap[0]))
     : (&(ibc_month_days[0])));

  for (utc->month = 0; utc->month < 12;)
    {
      if ((utc->year == ibc_utc_year_max)
	  && (utc->month >= ibc_utc_month_max))
	return (IBC_BAD_ARG);
      if ((my_days + ms[utc->month]) > ert->day)
	break;
      my_days += ms[(utc->month)++];
    }

  utc->day = ert->day - my_days;
  if (ert->msec >= 86400000)
    {
      utc->hour = 23;
      utc->min = 59;
      utc->sec = (ert->msec / 1000) - (86400 - 60);
    }
  else
    {
      utc->hour = ert->msec / 3600000;
      utc->min = (ert->msec / 60000) % 60;
      utc->sec = (ert->msec / 1000) % 60;
    }
  utc->usec = (ert->msec % 1000) * 1000 + ert->usec;

  return (IBC_OK);
}


int
ibc_leap_entry2tai (IBC_LEAP_SEC_ENTRY * lse, double *tai)
{
  IBC_UTC_TIME u;

  if (NULL == lse)
    return (IBC_NUL_PTR);
  if (NULL == tai)
    return (IBC_NUL_PTR);

  u.year = lse->year;
  u.month = lse->month;
  u.day = 0;
  u.hour = 0;
  u.min = 0;
  u.sec = 0;
  u.usec = 0;
  return (ibc_utc2tai2ert (&u, tai, NULL));
}


int
ibc_tai2utc (double tai, IBC_UTC_TIME * utc)
{
  int cdelta, cy, cm, i, r, *ms, month_sec, year_sec;
  double t1, t2;

  if (NULL == utc)
    return (IBC_NUL_PTR);

  for (i = 0; i < (IBC_LEAP_SEC_TABLE_SIZE - 1); i++)
    {
      if (IBC_OK != (r = ibc_leap_entry2tai (&(ibc_tai_leap_sec[i]), &t1)))
	return (r);
      if (IBC_LEAP_SEC_END_MARKER == ibc_tai_leap_sec[i + 1].year)
	{
	  t2 = 1e99;
	}
      else
	{
	  if (IBC_OK !=
	      (r = ibc_leap_entry2tai (&(ibc_tai_leap_sec[i + 1]), &t2)))
	    return (r);
	}
      if (tai < t1)
	return (IBC_BAD_ARG);	/* time before 1.1.72 */
      if ((tai >= t1) && (tai < t2))	/* found TAI region between 2 leap seconds */
	{
	  cdelta = ibc_tai_leap_sec[i + 1].delta - ibc_tai_leap_sec[i].delta;
	  cy = ibc_tai_leap_sec[i + 1].year;	/* compute leap month/year pair */
	  cm = ibc_tai_leap_sec[i + 1].month - 1;
	  if (cm < 0)
	    {
	      cy--;
	      cm = 11;
	    }			/* and next month has an entry in leap second table */

	  utc->year = ibc_tai_leap_sec[i].year;	/* we start from beginning of this TAI region */
	  utc->month = ibc_tai_leap_sec[i].month;

	  ms =
	    ((29 ==
	      ibc_get_feb_days (utc->
				year)) ? (&(ibc_month_days_leap[0]))
	     : (&(ibc_month_days[0])));

	  for (;;)		/* we traverse up to the end of first year */
	    {
	      if (utc->month >= 12)	/* end of this year ? */
		{
		  (utc->year)++;
		  (utc->month) = 0;
		  break;
		}
	      if ((utc->year == ibc_utc_year_max)
		  && (utc->month >= ibc_utc_month_max))
		return (IBC_BAD_ARG);
	      month_sec = ms[utc->month] * 86400;
	      if ((cy == utc->year) && (cm == utc->month))
		month_sec += cdelta;
	      if ((t1 + month_sec) > tai)
		break;		/* we have found month */
	      t1 += month_sec;
	      (utc->month)++;
	    }

	  for (;;)		/* then we (fast) traverse next years */
	    {
	      if (utc->year >= ibc_utc_year_max)
		return (IBC_BAD_ARG);
	      year_sec = (337 + ibc_get_feb_days (utc->year)) * 86400;
	      if (cy == utc->year)
		year_sec += cdelta;
	      if ((t1 + year_sec) > tai)
		break;		/* have have leap second year */
	      t1 += year_sec;
	      (utc->year)++;
	    }
	  /* at this point we _HAVE_ computed year */

	  ms =
	    ((29 ==
	      ibc_get_feb_days (utc->
				year)) ? (&(ibc_month_days_leap[0]))
	     : (&(ibc_month_days[0])));

	  for (;;)		/* and finally we traverse last year */
	    {
	      if (utc->month >= 12)
		return (IBC_BAD_ARG);	/* and of this year ? */
	      if ((utc->year == ibc_utc_year_max)
		  && (utc->month >= ibc_utc_month_max))
		return (IBC_BAD_ARG);
	      month_sec = ms[utc->month] * 86400;
	      if ((cy == utc->year) && (cm == utc->month))
		month_sec += cdelta;
	      if ((t1 + month_sec) > tai)
		break;		/* we have found month */
	      t1 += month_sec;
	      (utc->month)++;
	    }
	  /* at this point we _HAVE_ computed year/month */

	  utc->day = (int) floor ((tai - t1) / 86400);
	  if (utc->day >= ms[utc->month])
	    {
	      utc->day = ms[utc->month] - 1;	/* leap second case */
	      t1 += utc->day * 86400;
	      utc->hour = 23;
	      t1 += utc->hour * 3600;
	      utc->min = 59;
	      t1 += utc->min * 60;
	    }
	  else
	    {
	      t1 += utc->day * 86400;	/* ordinary time */
	      utc->hour = (int) floor ((tai - t1) / 3600);
	      t1 += utc->hour * 3600;
	      utc->min = (int) floor ((tai - t1) / 60);
	      t1 += utc->min * 60;
	    }
	  utc->sec = (int) floor (tai - t1);
	  t1 += utc->sec;

	  utc->usec = (int) floor (0.5 + (tai - t1) * 1e6);
	  if (utc->usec >= 1000000)
	    utc->usec = 999999;

	  return (IBC_OK);
	}

      if (IBC_LEAP_SEC_END_MARKER == ibc_tai_leap_sec[i + 1].year)
	break;			/* should not happen, but who knows ... */
    }

  return (IBC_BAD_ARG);
}


int
ibc_current_utc (IBC_UTC_TIME * utc)	/* days from 01.Jan.1958 plus millisec of day */
{
  struct timeval mytv;
  struct tm tm;
  time_t sec;

  if (NULL == utc)
    return (IBC_NUL_PTR);

  if (gettimeofday (&mytv, NULL))
    return (IBC_BAD_ARG);
  sec = mytv.tv_sec;
#ifdef IBC_ARCH_MACOSX_CC
  if (NULL == (tm2 = gmtime (&sec)))
    return (IBC_BAD_ARG);
  tm = *tm2;
#else
  if (NULL == (gmtime_r (&sec, &tm)))
    return (IBC_BAD_ARG);
#endif
  utc->year = tm.tm_year + 1900;
  utc->month = tm.tm_mon;
  utc->day = tm.tm_mday - 1;
  utc->hour = tm.tm_hour;
  utc->min = tm.tm_min;
  utc->sec = tm.tm_sec;
  utc->usec = mytv.tv_usec;
  return (IBC_OK);
}

		/* convert UTC time to an ISO9601 string with 100 microsec accuracy (24 chars, no EOS) */

int
ibc_utc2str24 (IBC_UTC_TIME utc, char *str_utc)
{
  char buf[99];
  double tai_utc;
  int r;

  if (NULL == str_utc)
    return (IBC_NUL_PTR);
  if (IBC_OK != (r = ibc_utc2tai2ert (&utc, &tai_utc, NULL)))
    return (r);
  tai_utc += 5e-5;		/* for proper rounding to 100 microsec */
  if (IBC_OK == (r = ibc_tai2utc (tai_utc, &utc)))
    {
      sprintf (buf, "%04d-%02d-%02dT%02d:%02d:%02d.%04d", utc.year,
	       utc.month + 1, utc.day + 1, utc.hour, utc.min, utc.sec,
	       utc.usec / 100);
      memcpy (str_utc, buf, 24);
    }
  return (r);
}

		/* convert an ISO9601 string with 100 microsec accuracy (24 chars, no EOS) to UTC time */

int
ibc_str242utc (char *str_utc, IBC_UTC_TIME * utc)
{
  char buf[99], ctmp;
  double tai_utc;
  int nrec;

  if (NULL == str_utc)
    return (IBC_NUL_PTR);
  if (NULL == utc)
    return (IBC_NUL_PTR);
  memcpy (buf, str_utc, 24);
  buf[24] = 0;
  nrec =
    sscanf (buf, "%04d-%02d-%02dT%02d:%02d:%02d.%04d%c", &(utc->year),
	    &(utc->month), &(utc->day), &(utc->hour), &(utc->min),
	    &(utc->sec), &(utc->usec), &ctmp);
  if (7 != nrec)
    return (IBC_BAD_ARG);
  utc->month--;
  utc->day--;
  utc->usec *= 100;
  return (ibc_utc2tai2ert (utc, &tai_utc, NULL));	/* this is just to check ISO9601 string format */
}
