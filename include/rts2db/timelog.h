/*
 * Abstract class for timelog messages. Used to construct log
 * from multiple sources.
 * Copyright (C) 2010 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_TIMELOG__
#define __RTS2_TIMELOG__

#include <ostream>

namespace rts2db
{

/**
 * Abstract class for printing messages comming from multiple sources.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class TimeLog
{
	public:
		TimeLog () {}
		/**
		 * Rewind log to beginning.
		 */
		virtual void rewind () = 0;
		/**
		 * Get time of next event.
		 */
		virtual double getNextCtime () = 0;
		/**
		 * Print messages until given time.
		 *
		 * @param time timestamp - end of log entries printout
		 * @param os   stream where log entries will be printed
		 */
		virtual void printUntil (double time, std::ostream &os) = 0;
};

}

#endif // ! __RTS2_TIMELOG__
