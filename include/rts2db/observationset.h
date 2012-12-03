/* 
 * Observation class set.
 * Copyright (C) 2005-2010 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_OBS_SET__
#define __RTS2_OBS_SET__

#include "observation.h"
#include "timelog.h"

#include <string>
#include <vector>

namespace rts2db {

/**
 * Observations set class.
 *
 * This class contains list of observations. It's created from DB, based on various criteria (
 * eg. all observation from single night, all observation in same time span, or all observations
 * to give target).
 *
 * It can print various statistic on observations in set.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class ObservationSet:public std::vector <Observation>, public TimeLog
{
	public:
		ObservationSet (void);
		virtual ~ ObservationSet (void);

		void loadTarget (int _tar_id, const time_t * start_t = NULL, const time_t * end_t = NULL);
		void loadTime (const time_t * start_t, const time_t * end_t);

		void loadType (char type_id, int state_mask, bool inv = false);
		void loadRadius (struct ln_equ_posn *position, double radius);

		/**
		 * Load all targets with given label_id.
		 */
		void loadLabel (int label_id);

		void  printImages (int _images) { images = _images; }
		int getPrintImages () { return images; }

		int getPrintCounts () { return counts; }
		void printCounts (int _counts) { counts = _counts; }

		/**
		 * Collocate observations by target ids.
		 */
		void collocate () { collocated = true; }
		bool isCollocated () { return collocated; }

		int getSuccess () { return successNum; }
		int getFailed () { return failedNum; }

		int computeStatistics ();

		int getNumberOfImages () { return allNum; }

		int getNumberOfGoodImages () { return goodNum; }

		void printStatistics (std::ostream & _os);

		/**
		 * Return JD of start of the first observation in the set.
		 *
		 * @return JD of start of the first observation.
		 */
		double getJDStart ()
		{
			if (size () == 0)
				return NAN;
			return (*(end () - 1)).getObsJDStart ();
		}

		/**
		 * Return JD of the end of last observation in the set.
		 *
		 * @return JD of the end of last last observation.
		 */
		double getJDEnd ()
		{
			if (size () == 0)
				return NAN;
			return (*(begin ())).getObsJDEnd ();
		}


		/**
		 * Return map of targetsId and number of their observations.
		 * The returning map has as a key targetId, and as value number of observations
		 * of the target.
		 *
		 * @param Map containing target id as a key and number of target observations as value.
		 */
		std::map <int, int> getTargetObservations ();

		// Merit functions..
		
		/**
		 * Returns averaged altitude merit function.
		 *
		 * @return Averaged altitudu merit function of the observations.
		 */
		double altitudeMerit ();

		/**
		 * Returns merit based on distance between scheduled entries.
		 *
		 * @return 1 / sum of distance distance between schedule entries.
		 */
		double distanceMerit ();

		std::ostream & print (std::ostream &_os);

		friend std::ostream & operator << (std::ostream & _os, ObservationSet & obs_set) { return obs_set.print (_os); }

		virtual void rewind () { timeLogIter = begin (); }
		virtual double getNextCtime ();
		virtual void printUntil (double time, std::ostream &os);
	private:
		int images;
		int counts;
		bool collocated;
		int successNum;
		int failedNum;

		void load (std::string in_where);

		// numbers
		int allNum;
		int goodNum;
		int firstNum;

		// errors..
		double errFirstRa;
		double errFirstDec;
		double errFirstRad;

		double errAvgRa;
		double errAvgDec;
		double errAvgRad;

		std::vector <Observation>::iterator timeLogIter;
};

/**
 * Represents statistic for given time period.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class ObservationStatistics
{
	public:
		ObservationStatistics (int _numObs) { numObs = _numObs; }
	private:
		int numObs;
};

class DateStatistics
{
	public:
		DateStatistics () { c = i = gi = 0; tt = NAN;}
		DateStatistics (int obscount, int images = 0, int good_images = 0, double _tt = NAN)
		{
			c = obscount;
			i = images;
			gi = good_images;
			tt = _tt;
		}

		int c;
		int i;
		int gi;
		double tt;
};

/**
 * Load data from night observation table, grouped by various depth of date (month, year,..).
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class ObservationSetDate:public std::map <int, DateStatistics>
{
	public:
		ObservationSetDate () {}

		/**
		 * Construct set targets for given time period. The parameters
		 * specify which part of the date should be considered. If the
		 * paremeter is -1, the load method select all targets,
		 * regardless of the date value for given part. If it is > 0,
		 * select run on all dates which have the component equal to
		 * this value. This is handy for hierarchical browsing.
		 * Suppose that we would like to select all targets in year
		 * 2009 and 2010. We offer user a choice which months are
		 * available. If he/she click on December 2009, we select all
		 * targets for December 2009, and allow user to select day. For
		 * each entry, it then creates entry in the map, together with
		 * number of observations and some statistics.
		 *
		 * @param year    year of selection
		 * @param month   month of selection
		 * @param day     selection day
		 * @param hour    selection hour
		 * @param minutes selection minutes
		 */
		void load (int year = -1, int month = -1, int day = -1, int hour = 1, int minutes = -1);
};

int lastObservationId ();

}
#endif	 /* !__RTS2_OBS_SET__ */
