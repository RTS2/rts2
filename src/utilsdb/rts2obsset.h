/* 
 * Observation class set.
 * Copyright (C) 2005-2008 Petr Kubanek <petr@kubanek.net>
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

#include "rts2obs.h"

#include <string>
#include <vector>

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

class Rts2ObsSet:public std::vector <Rts2Obs >
{
	private:
		int images;
		int counts;
		bool collocated;
		int successNum;
		int failedNum;
		void initObsSet ()
		{
			images = 0;
			counts = 0;
			collocated = false;
			successNum = 0;
			failedNum = 0;

			allNum = 0;
			goodNum = 0;

			firstNum = 0;

			errFirstRa = 0;
			errFirstDec = 0;
			errFirstRad = 0;

			errAvgRa = 0;
			errAvgDec = 0;
			errAvgRad = 0;
		}
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
	public:
		Rts2ObsSet (void);
		Rts2ObsSet (int in_tar_id, const time_t * start_t, const time_t * end_t);
		Rts2ObsSet (const time_t * start_t, const time_t * end_t);
		Rts2ObsSet (int in_tar_id);
		Rts2ObsSet (char type_id, int state_mask, bool inv = false);
		Rts2ObsSet (struct ln_equ_posn *position, double radius);
		virtual ~ Rts2ObsSet (void);

		void  printImages (int in_images)
		{
			images = in_images;
		}

		int getPrintImages ()
		{
			return images;
		}

		int getPrintCounts ()
		{
			return counts;
		}

		void printCounts (int in_counts)
		{
			counts = in_counts;
		}

		/**
		 * Collocate observations by target ids.
		 */
		void collocate ()
		{
			collocated = true;
		}

		bool isCollocated ()
		{
			return collocated;
		}

		int getSuccess ()
		{
			return successNum;
		}
		int getFailed ()
		{
			return failedNum;
		}
		int computeStatistics ();

		int getNumberOfImages ()
		{
			return allNum;
		}

		int getNumberOfGoodImages ()
		{
			return goodNum;
		}

		void printStatistics (std::ostream & _os);

		/**
		 * Return JD of start of the first observation in the set.
		 *
		 * @return JD of start of the first observation.
		 */
		double getJDStart ()
		{
			if (size () == 0)
				return nan("f");
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
				return nan("f");
			return (*(begin ())).getObsJDEnd ();
		}

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

		friend std::ostream & operator << (std::ostream & _os, Rts2ObsSet & obs_set);
};

std::ostream & operator << (std::ostream & _os, Rts2ObsSet & obs_set);
#endif							 /* !__RTS2_OBS_SET__ */
