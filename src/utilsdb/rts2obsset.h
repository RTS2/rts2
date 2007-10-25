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
 * @author petr
 */

class Rts2ObsSet:public
std::vector <
Rts2Obs >
{
	private:
		int
			images;
		int
			counts;
		int
			successNum;
		int
			failedNum;
		void
			initObsSet ()
		{
			images = 0;
			counts = 0;
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
		void
			load (std::string in_where);

		// numbers
		int
			allNum;
		int
			goodNum;
		int
			firstNum;

		// errors..
		double
			errFirstRa;
		double
			errFirstDec;
		double
			errFirstRad;

		double
			errAvgRa;
		double
			errAvgDec;
		double
			errAvgRad;
	public:
		Rts2ObsSet (void);
		Rts2ObsSet (int in_tar_id, const time_t * start_t, const time_t * end_t);
		Rts2ObsSet (const time_t * start_t, const time_t * end_t);
		Rts2ObsSet (int in_tar_id);
		Rts2ObsSet (char type_id, int state_mask, bool inv = false);
		Rts2ObsSet (struct ln_equ_posn *position, double radius);
		virtual ~ Rts2ObsSet (void);

		void
			printImages (int in_images)
		{
			images = in_images;
		}

		int
			getPrintImages ()
		{
			return images;
		}

		int
			getPrintCounts ()
		{
			return counts;
		}

		void
			printCounts (int in_counts)
		{
			counts = in_counts;
		}

		int
			getSuccess ()
		{
			return successNum;
		}
		int
			getFailed ()
		{
			return failedNum;
		}
		int
			computeStatistics ();

		int
			getNumberOfImages ()
		{
			return allNum;
		}

		int
			getNumberOfGoodImages ()
		{
			return goodNum;
		}

		void
			printStatistics (std::ostream & _os);

		friend
			std::ostream & operator << (std::ostream & _os, Rts2ObsSet & obs_set);
};

std::ostream & operator << (std::ostream & _os, Rts2ObsSet & obs_set);
#endif							 /* !__RTS2_OBS_SET__ */
