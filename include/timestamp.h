/* 
 * Timestamp manipulation routines.
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

#ifndef __TIMESTAMP_CPP__
#define __TIMESTAMP_CPP__

#include "rts2format.h"
#include "utilsfunc.h"
#include <status.h>

#include <ostream>
#include <sys/time.h>
#include <libnova/libnova.h>
#include <time.h>
#include <math.h>

/**
 * Provides support timestamps obtained from DB.
 *
 * It was primary designed to enable correct streaminig of timestamps (operator <<), but
 * it might be extendet to carry other operations with timestamp.
 *
 * @author petr
 */
class Timestamp
{
	private:
		double ts;
	public:
		Timestamp ()
		{
			struct timeval tv;
			gettimeofday (&tv, NULL);
			ts = tv.tv_sec + tv.tv_usec / USEC_SEC;
		}
		Timestamp (double _ts)
		{
			ts = _ts;
		}
		Timestamp (time_t _ts)
		{
			ts = _ts;
		}
		Timestamp (struct timeval *tv)
		{
			ts = tv->tv_sec + (double) tv->tv_usec / USEC_SEC;
		}
		void setTs (double _ts)
		{
			ts = _ts;
		}
		double getTs ()
		{
			return ts;
		}
		friend std::ostream & operator << (std::ostream & _os, Timestamp _ts);
};

std::ostream & operator << (std::ostream & _os, Timestamp _ts);

class TimeJD:public Timestamp
{
	public:
		TimeJD ():Timestamp ()
		{
		}
		/**
		 * Construct Timestamp from JD.
		 *
		 * We cannot put it to Timestamp, as we already have in Timestamp
		 * constructor which takes one double.
		 *
		 * @param JD Julian Day as double variable
		 */
		TimeJD (double JD):Timestamp ()
		{
			if (isnan (JD))
			{
				setTs (NAN);
			}
			else
			{
				time_t _ts;
				ln_get_timet_from_julian (JD, &_ts);
				setTs (_ts);
			}
		}
};

class TimeDiff
{
	public:
		TimeDiff () {}

		TimeDiff (double in_time, bool _print_milisec = true)
		{
			time_1 = 0;
			time_2 = in_time;
			print_milisec = _print_milisec;
		}

		/**
		 * Construct time diff from two doubles.
		 *
		 */
		TimeDiff (double in_time_1, double in_time_2, bool _print_milisec = true)
		{
			time_1 = in_time_1;
			time_2 = in_time_2;
			print_milisec = _print_milisec;
		}

		TimeDiff (double in_time_1, time_t in_time_2, bool _print_milisec = true)
		{
			time_1 = in_time_1;
			time_2 = in_time_2;
			print_milisec = _print_milisec;
		}

		double getTimeDiff ()
		{
			return time_2 - time_1;
		}

		friend std::ostream & operator << (std::ostream & _os, TimeDiff _td);

	private:
		double time_1, time_2;
		bool print_milisec;
};

std::ostream & operator << (std::ostream & _os, TimeDiff _td);

class TimeJDDiff:public TimeJD
{
	private:
		time_t time_diff;
	public:
		TimeJDDiff ():TimeJD ()
		{
		}

		TimeJDDiff (double in_time_jd, time_t in_time_diff):TimeJD (in_time_jd)
		{
			time_diff = in_time_diff;
		}

		friend std::ostream & operator << (std::ostream & _os, TimeJDDiff _tjd);
};

std::ostream & operator << (std::ostream & _os, TimeJDDiff _tjd);

/**
 * Holds and prints percentage.
 */
class Percentage
{
	private:
		double per;
		double total;
	public:
		Percentage (double in_per, double in_total)
		{
			per = in_per;
			total = in_total;
		}

		friend std::ostream & operator << (std::ostream & _os, Percentage _per);
};

std::ostream & operator << (std::ostream & _os, Percentage _per);
#endif							 /* !__TIMESTAMP_CPP__ */
