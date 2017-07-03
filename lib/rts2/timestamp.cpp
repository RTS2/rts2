/* 
 * Timestamp manipulation routines.
 * Copyright (C) 2003-2007 Petr Kubanek <petr@kubanek.net>
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

#include "timestamp.h"
#include "rts2format.h"
#include "block.h"

#include <iomanip>
#include <math.h>
#include <time.h>
#include <sys/time.h>

#include <sstream>

std::ostream & operator << (std::ostream & _os, Timestamp _ts)
{
	// convert timestamp to timeval
	struct timeval tv;
	struct tm *tmval;
	if (formatPureNumbers (_os))
	{
		_os.setf (std::ios_base::fixed, std::ios_base::floatfield);
		_os.precision(6);
		_os << _ts.ts;
		return _os;
	}
	if (!finite (_ts.ts))
	{
		_os << std::right << std::setw (22) << (std::isinf (_ts.ts) ? "infinite " : "---- ");
		if (formatLocalTime (_os))
			_os << *tzname;
		else
			_os << "UT";  
		return _os;
	}
	tv.tv_sec = (long) _ts.ts;
	tv.tv_usec = (long) ((_ts.ts - floor (_ts.ts)) * USEC_SEC);
	if (formatLocalTime (_os))
		tmval = localtime (&tv.tv_sec);
	else
		tmval = gmtime (&tv.tv_sec);

	std::ios_base::fmtflags old_settings = _os.flags ();
	_os.setf (std::ios_base::fixed, std::ios_base::floatfield);
	int old_precision = _os.precision (3);
	char old_fill = _os.fill ('0');

	_os << std::setw (4) << (tmval->tm_year + 1900) << "-"
		<< std::setw (2) << (tmval->tm_mon + 1) << "-"
		<< std::setw (2) << tmval->tm_mday << "T"
		<< std::setw (2) << tmval->tm_hour << ":"
		<< std::setw (2) << tmval->tm_min << ":"
		<< std::setw (6) << ((double) tmval->tm_sec +
		(double) tv.tv_usec / USEC_SEC);

	if (formatLocalTime (_os))
		_os << " " << *tzname;
	else
		_os << " UT";  	

	_os.flags (old_settings);
	_os.precision (old_precision);
	_os.fill (old_fill);

	return _os;
}

std::ostream & operator << (std::ostream & _os, TimeDiff _td)
{
	if (formatPureNumbers (_os))
	{
		_os.setf (std::ios_base::fixed, std::ios_base::floatfield);
		_os.precision(6);
		_os << _td.time_2 - _td.time_1;
		return _os;
	}
	if (!finite (_td.time_1) || !finite (_td.time_2))
	{
		_os << ((std::isinf (_td.time_1) || std::isinf (_td.time_2)) ? "infinite" : "na (nan)");
	}
	else
	{
		std::ostringstream _oss;
		int diff = (int) (_td.time_2 - _td.time_1);
		if (diff < 0)
		{
			_oss << "-";
			diff *= -1;
		}
		long usec_diff = (long) ((fabs (_td.time_2 - _td.time_1) - diff) * USEC_SEC);

		int msec = usec_diff / (USEC_SEC / 1000);

		// microseconds will show as zero
		// int usec = usec_diff / (USEC_SEC / 1000000);

		bool print_all = false;
		bool mprinted = false;
		if (diff / 86400 >= 1)
		{
			_oss << (diff / 86400) << " days ";
			diff %= 86400;
			print_all = true;
		}
		if (print_all)
			_oss.fill ('0');

		if (diff / 3600 >= 1 || print_all)
		{
			_oss << std::setw (2) << (diff / 3600) << ":";
			diff %= 3600;
			print_all = true;
			_oss.fill ('0');
		}
		if (diff / 60 >= 1 || print_all)
		{
			_oss << std::setw (2) << (diff / 60);
			if (!print_all)
			{
				_oss << "m";
				mprinted = true;
			}	
			else
				_oss << ":";
			diff %= 60;
		}

		if (diff > 0 || print_all)
		{
			if (mprinted)
				_oss << " ";
			_oss << std::setw (2) << diff;
			if (_td.print_milisec || mprinted == false)
				if (msec > 0 || print_all)
					 _oss << "." << std::setfill('0') << std::setw (3) << msec;
			if (!print_all)
				_oss << "s";
			mprinted = true;
		}

		if (diff == 0 && msec > 0)
		{
			if (msec > 99)
			{
				if ((msec > 0 && _td.print_milisec) || mprinted == false)
					_oss << "0." << msec;
						
				_oss << "s";
				mprinted = true;
			}
			else
			{	
				if ((msec > 0 && _td.print_milisec) || mprinted == false)
					_oss  << msec;
						
				_oss << "ms";
				mprinted = true;
			}	
		}

		if (!mprinted)
			_oss << "0";


		_os << _oss.str ();
	}
	return _os;
}

std::ostream & operator << (std::ostream & _os, TimeJDDiff _tjd)
{
	TimeDiff td = TimeDiff (_tjd.time_diff, _tjd.getTs ());
	_os << ((TimeJD) _tjd) << " (" << td << ")";
	return _os;
}

std::ostream & operator << (std::ostream & _os, Percentage _per)
{
	if (!finite (_per.per) || !finite (_per.total) || _per.total == 0)
	{
		_os << std::setw (6) << std::right  << ((std::isinf (_per.per) || std::isinf (_per.total)) ? "infinite" : "nan");
	}
	else
	{
		std::ios_base::fmtflags old_settings = _os.flags ();
		_os.setf (std::ios_base::fixed, std::ios_base::floatfield);
		int old_precision = _os.precision (2);
		_os << std::setw (6) << ((double) (_per.per * 100.0) / _per.total);
		_os.precision (old_precision);
		_os.setf (old_settings);
	}
	return _os;
}
