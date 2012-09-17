/* 
 * Observation entry.
 * Copyright (C) 2005-2007 Petr Kubanek <petr@kubanek.net>
 * Copyright (C) 2007 Stanislav Vitek <standa@iaa.es>
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

#ifndef __RTS2_OBS_DB__
#define __RTS2_OBS_DB__

#include <ostream>
#include <time.h>
#include <vector>
#include <stdexcept>

#include "imgdisplay.h"

#include "timestamp.h"

#include "rts2db/rts2count.h"
#include "rts2db/imageset.h"
#include "rts2db/target.h"

namespace rts2db
{

/**
 * Observation class.
 *
 * This class contains observation. They are (optionaly) linked to images.
 * It can calculate (and display) various statistic on images it contains.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Observation
{
	public:
		/**
		 * Create class from DB
		 */
		Observation (int in_obs_id = -1);
		/**
		 * Create class from informations provided.
		 *
		 * Handy when creating class from cursor.
		 */
		Observation (int in_tar_id, const char *in_tar_name, char in_tar_type,
			int in_obs_id, double in_obs_ra, double in_obs_dec,
			double in_obs_alt, double in_obs_az, double in_obs_slew,
			double in_obs_start, int in_obs_state, double in_obs_end);
		virtual ~ Observation (void);
		int load ();
		int loadLastObservation ();
		int loadImages ();
		int loadCounts ();

		void startSlew (struct ln_equ_posn * coords, struct ln_hrz_posn * hrz);
		void startObservation ();
		void endObservation (int _obs_state);

		void printObsHeader (std::ostream & _os);

		void printCountsShort (std::ostream & _os);
		void printCounts (std::ostream & _os);
		void printCountsSummary (std::ostream & _os);

		void setPrintImages (int in_printImages) { displayImages = in_printImages; }

		void setPrintCounts (int in_printCounts)
		{
			if (in_printCounts == DISPLAY_SHORT)
				printHeader = false;
			displayCounts = in_printCounts;
		}

		void setPrintHeader (bool in_printHeader) { printHeader = in_printHeader; }

		/**
		 * Return target structure.
		 *
		 * @return Target structure in case of success, NULL in case of error.
		 */
		Target *getTarget ()
		{
                 	rts2db::TargetSet::iterator iter = rts2db::TargetSetSingleton::instance ()->find (getTargetId ());
			if (iter == rts2db::TargetSetSingleton::instance ()->end ())
    			{
                         	return NULL;
			}
                        return (*iter).second;
		}

		/**
		 * Return real target ID.
		 *
		 * @return Target id.
		 */
		int getTargetId () { return tar_id; }

		char getTargetType () { return tar_type; }

		/**
		 * Return name of observation target.
		 *
		 * @return Target name.
		 */
		const std::string getTargetName () { return tar_name; }

		int getObsId () { return obs_id; }

		/**
		 * Return time when telescope started slewing to observation location.
		 */
		double getObsSlew () { return obs_slew; }

		double getObsStart () { return obs_start; }

		double getObsJDStart ()
		{
			if (isnan (getObsStart ()))
				return getObsStart ();
			time_t s = (time_t) getObsStart ();
			return ln_get_julian_from_timet (&s);
		}

		/**
		 * Return middle time of observation.
		 */
		double getObsJDMid ()
		{
			if (isnan (getObsStart ()) || isnan (getObsStart ()))
				return NAN;
			time_t mid = (time_t) ((getObsStart () + getObsEnd ()) / 2.0);
			return ln_get_julian_from_timet (&mid);
		}

		double getObsEnd () { return obs_end; }

		double getObsJDEnd ()
		{
			if (isnan (getObsEnd ()))
				return getObsEnd ();
			time_t e = (time_t) getObsEnd ();
			return ln_get_julian_from_timet (&e);
		}

		/**
		 * Calculate observation altitude merit withing given interval.
		 *
		 * @param _start JD of interval start.
		 * @param _end   JD of interval end.
		 *
		 * @return Merit (=number between 0 and 1, higher means better).
		 */
		double altitudeMerit (double _start, double _end);

		/**
		 * Get equatiorial position of the target at the beginning of the observation.
		 *
		 * @param _pos Returned position.
		 *
		 * @return -1 on error, 0 on success.
		 */
		int getStartPosition (struct ln_equ_posn &_pos)
		{
			if (isnan (getObsJDStart ()))
				return -1;
			getTarget ()->getPosition (&_pos, getObsJDStart ());
			return 0;
		}

		/**
		 * Get equatiorial position of the target at the end of the observation.
		 *
		 * @param _pos Returned position.
		 *
		 * @return -1 on error, 0 on success.
		 */
		int getEndPosition (struct ln_equ_posn &_pos)
		{
			if (isnan (getObsJDEnd ()))
				return -1;
			getTarget ()->getPosition (&_pos, getObsJDEnd ());
			return 0;
		}

		double getObsRa () { return obs_ra; }

		double getObsDec () { return obs_dec; }

		ImageSet *getImageSet () { return imgset; }

		int getUnprocessedCount ();

		/**
		 * Called to check if we have any unprocessed images
		 * if we don't have any unprocessed images and observation was ended, then
		 * call appopriate mails.
		 *
		 * @return 0 when we don't have more images to process and observation
		 * was already finished, -1 when observation is still ongoing, > 0 when there are
		 * return images to process, but observation was ended.
		 */
		int checkUnprocessedImages (rts2core::Block * master);

		int getNumberOfImages ();

		int getNumberOfGoodImages ();

		int getFirstErrors (double &eRa, double &eDec, double &eRad);
		int getAverageErrors (double &eRa, double &eDec, double &eRad);

		/**
		 * Return -1 when previous position cannot
		 */
		int getPrevPosition (struct ln_equ_posn &prevPoz, struct ln_hrz_posn &hrzPoz);

		void getEqu (struct ln_equ_posn &equ)
		{
			equ.ra = obs_ra;
			equ.dec = obs_dec;
		}

		void getHrz (struct ln_hrz_posn &hrz)
		{
			hrz.az = obs_az;
			hrz.alt = obs_alt;
		}

		/**
		 * Return average slew speed in arc deg/sec. Return nan when slew speed cannot be determined.
		 */
		double getPrevSeparation ();

		/**
		 * Return slew speed in degrees / second.
		 *
		 * @return Slew speed in degrees per seconds.
		 */
		double getSlewSpeed ();

		/**
		 * Return time spend slewing telescope on target.
		 *
		 * @return Slew time in seconds.
		 */
		double getSlewTime ();

		/**
		 * Return observation time. Return time spend with telescope on target.
		 *
		 * @return Observation time in seconds.
		 */
		double getObsTime ();

		/**
		 * Return time spend on sky.
		 */
		double getTimeOnSky ();

		void maskState (int newBits);
		void unmaskState (int newBits);

		friend std::ostream & operator << (std::ostream & _os, Observation & obs)
		{
			obs.printObsHeader (_os);
			if (obs.displayImages)
			{
				obs.loadImages ();
				_os << std::endl;
				obs.imgset->print (_os, obs.displayImages);
			}
			if (obs.displayCounts)
			{
				obs.loadCounts ();
				if (obs.displayCounts != DISPLAY_SHORT)
					_os << std::endl;
				if (obs.displayCounts & DISPLAY_SHORT)
					obs.printCountsShort (_os);
				if (obs.displayCounts & DISPLAY_ALL)
					obs.printCounts (_os);
				if (obs.displayCounts & DISPLAY_SUMMARY)
					obs.printCountsSummary (_os);
			}
			return _os;
		}

	private:
		//! list of images for that observation
		ImageSet * imgset;
		std::vector < Rts2Count > counts;

		int displayImages;
		int displayCounts;

		bool printHeader;

		int tar_id;
		std::string tar_name;
		char tar_type;
		int obs_id;
		double obs_ra;
		double obs_dec;
		double obs_alt;
		double obs_az;
		double obs_slew;
		double obs_start;
		int obs_state;
		// nan when observations wasn't ended
		double obs_end;
};

class ObservationState
{
	public:
		ObservationState (int in_state)
		{
			state = in_state;
		}

		friend std::ostream & operator << (std::ostream & _os, ObservationState obs_state)
		{
			if (obs_state.state & OBS_BIT_STARTED)
				_os << 'S';
			else if (obs_state.state & OBS_BIT_MOVED)
				_os << 'M';
			else
				_os << ' ';
		
			if (obs_state.state & OBS_BIT_ACQUSITION_FAI)
				_os << 'F';
			else if (obs_state.state & OBS_BIT_ACQUSITION)
				_os << 'A';
			else
				_os << ' ';
		
			if (obs_state.state & OBS_BIT_INTERUPED)
				_os << 'I';
			else
				_os << ' ';
		
			return _os;
		}
	private:
		int state;
};

}

#endif							 /* !__RTS2_OBS_DB__ */
