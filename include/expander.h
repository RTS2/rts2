/* 
 * Expanding mechanism.
 * Copyright (C) 2007-2010 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_EXPANDER__
#define __RTS2_EXPANDER__

#include <string>
#include <time.h>
#include <sys/time.h>

namespace rts2core
{

/**
 * This class is common ancestor to expending mechanism.
 * Short one-letter variables are prefixed with %, two letters and longer
 * variables are prefixed with $. Double % or $ works as escape characters.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Expander
{
	public:
		Expander ();
		Expander (const struct timeval *tv);
		Expander (Expander * in_expander);
		virtual ~ Expander (void);

		/**
		 * Expand variables encoded in string. Iterates through string,
		 * do some actions if it encounters % or @ expansion characters.
		 *
		 * @param expression      string to expand. % and @ starts expression characters
		 * @param onlyAlphaNum    filter result of the expansion, so any non-alphaNumerical character will be replaced with _
		 *
		 * @return expanded string
		 */
		virtual std::string expand (std::string expression, bool onlyAlphaNum = false);

		/**
		 * Expand file path.
		 *
		 * @param pathEx      Path expansion string.
		 * @param onlyAlNum   If true, replace non-path character in expansion
		 *
		 * @return Expanded path.
		 */
		virtual std::string expandPath (std::string pathEx, bool onlyAlNum = true);

		/**
		 * Sets expanding date to current sysdate.
		 */
		void setExpandDate ();

		/**
		 * Sets expanding date. This date is used in construction of
		 * the filename (for %y%d.. expansions).
		 *
		 * @param tv Timeval holding date to set.
		 */
		void setExpandDate (const struct timeval *tv) { setExpandDate (tv, false); }

		void setExpandDate (const struct timeval *tv, bool localdate);

		void useLocalDate () { expandDate = &localDate; }
		void useUtDate () { expandDate = &utDate; }
	
		double getExpandDateCtime ();
		const struct timeval *getExpandDate ();

		// date related functions
		std::string getYearString () { return getYearString (getYear ()); }
		std::string getYearString (int year);
		std::string getShortYearString () { return getShortYearString (getYear ()); }
		std::string getShortYearString (int year);
		std::string getMonthString () { return getMonthString (getMonth ()); }
		std::string getMonthString (int month);
		std::string getDayString () { return getDayString (getDay ()); }
		std::string getDayString (int day);
		std::string getYDayString ();

		std::string getHourString ();
		std::string getMinString ();
		std::string getSecString ();
		std::string getMSecString ();

		std::string getNightString ();

		long getCtimeSec () { return expandTv.tv_sec; }

		long getCtimeUsec () { return expandTv.tv_usec; }

	protected:
		/**
		 * ID of current epoch.
		 */
		int epochId;
		
		// (unique) number - position, formatting;
		int num_pos;
		int num_pos_end;
		int num_lenght;
		char num_fill;

		virtual std::string expandVariable (char var, size_t beg, bool &replaceNonAlpha);
		virtual std::string expandVariable (std::string expression);

	private:
		// formating parameters - length, fill
		int length;
		char fill;
		struct tm localDate;
		struct tm utDate;
		struct tm nightDate;
		struct tm *expandDate;
		struct timeval expandTv;

		std::string getEpochString ();

		int getYear () { return expandDate->tm_year + 1900; }

		int getMonth () { return expandDate->tm_mon + 1; }

		int getDay () { return expandDate->tm_mday; }

		int getYDay () { return expandDate->tm_yday; }

		int getHour () { return expandDate->tm_hour; }

		int getMin () { return expandDate->tm_min; }

		int getSec () { return expandDate->tm_sec; }

		int getNightYear () { return nightDate.tm_year + 1900; }

		int getNightMonth () { return nightDate.tm_mon + 1; }

		int getNightDay () { return nightDate.tm_mday; }

		/**
		 * Retrieves formating parameters.
		 */
		void getFormating (const std::string &expression, std::string::iterator &iter, std::ostringstream &ret);
};

};

#endif							 /* !__RTS2_EXPANDER__ */
