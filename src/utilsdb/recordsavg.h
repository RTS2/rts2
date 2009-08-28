/* 
 * Utility classes for record values.
 * Copyright (C) 2009 Petr Kubanek <petr@kubanek.net>
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

#include <list>
#include <string>

namespace rts2db
{

/**
 * Class representing an average record (value).
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class RecordAvg
{
	private:
		double rectime;
		double avg;
		double min;
		double max;
		int rcount;
	public:
		RecordAvg (double _rectime, double _avg, double _min, double _max, int _rcount)
		{
			rectime = _rectime;
			avg = _avg;
			min = _min;
			max = _max;
			rcount = _rcount;
		}

		double getRecTime () { return rectime; };
		double getAverage () { return avg; };
		double getMinimum () { return min; };
		double getMaximum () { return max; };
		int getRecCout ()    { return rcount; };
};

typedef enum {DAY, HOUR} cadence_t;

/**
 * Class with value average records.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class RecordAvgSet: public std::list <RecordAvg>
{
	private:
		int recval_id;
		cadence_t cadence;
	public:
		RecordAvgSet (int _recval_id, cadence_t _cadence)
		{
			recval_id = _recval_id;
			cadence = _cadence;
		}

		void load (double t_from, double t_to);
};


}
