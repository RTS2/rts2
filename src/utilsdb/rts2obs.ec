/* 
 * Observation entry.
 * Copyright (C) 2005-2007 Petr Kubanek <petr@kubanek.net>
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

#include "rts2obs.h"
#include "target.h"
#include "rts2taruser.h"
#include "../utils/libnova_cpp.h"
#include "../utils/timestamp.h"
#include "../utils/rts2config.h"

#include <iostream>
#include <iomanip>
#include <sstream>

EXEC SQL INCLUDE sqlca;

Rts2Obs::Rts2Obs (int in_obs_id)
{
	obs_id = in_obs_id;
	tar_id = -1;
	tar_name = std::string ("unset");
	tar_type = 0;
	imgset = NULL;
	displayImages = 0;
	displayCounts = 0;
	printHeader = true;
}


Rts2Obs::Rts2Obs (int in_tar_id, const char *in_tar_name, char in_tar_type, int in_obs_id, double in_obs_ra,
double in_obs_dec, double in_obs_alt, double in_obs_az, double in_obs_slew, double in_obs_start,
int in_obs_state, double in_obs_end)
{
	tar_id = in_tar_id;
	tar_name = std::string (in_tar_name);
	tar_type = in_tar_type;
	obs_id = in_obs_id;
	obs_ra = in_obs_ra;
	obs_dec = in_obs_dec;
	obs_alt = in_obs_alt;
	obs_az = in_obs_az;
	obs_slew = in_obs_slew;
	obs_start = in_obs_start;
	obs_state = in_obs_state;
	obs_end = in_obs_end;
	imgset = NULL;
	displayImages = 0;
	displayCounts = 0;
	printHeader = true;
}


Rts2Obs::~Rts2Obs (void)
{
	std::vector <Rts2Image *>::iterator img_iter;
	if (imgset)
	{
		for (img_iter = imgset->begin (); img_iter != imgset->end (); img_iter++)
			delete (*img_iter);
		imgset->clear ();
		delete imgset;
	}
}


int
Rts2Obs::load ()
{
	EXEC SQL BEGIN DECLARE SECTION;
		// cannot use TARGET_NAME_LEN, as it does not work with some ecpg veriosn
		VARCHAR db_tar_name[150];
		int db_tar_id;
		char db_tar_type;
		int db_obs_id = obs_id;
		double db_obs_ra;
		int ind_obs_ra;
		double db_obs_dec;
		int ind_obs_dec;
		double db_obs_alt;
		int ind_obs_alt;
		double db_obs_az;
		int ind_obs_az;
		double db_obs_slew;
		int ind_obs_slew;
		double db_obs_start;
		int ind_obs_start;
		int db_obs_state;
		double db_obs_end;
		int ind_obs_end;
	EXEC SQL END DECLARE SECTION;

	// already loaded
	if (tar_id > 0)
		return 0;

	obs_ra = nan ("f");
	obs_dec = nan ("f");
	obs_alt = nan ("f");
	obs_az = nan ("f");
	obs_slew = nan ("f");
	obs_start = nan ("f");
	obs_state = 0;
	obs_end = nan ("f");

	EXEC SQL
		SELECT
			tar_name,
			observations.tar_id,
			type_id,
			obs_ra,
			obs_dec,
			obs_alt,
			obs_az,
			EXTRACT (EPOCH FROM obs_slew),
			EXTRACT (EPOCH FROM obs_start),
			obs_state,
			EXTRACT (EPOCH FROM obs_end)
		INTO
			:db_tar_name,
			:db_tar_id,
			:db_tar_type,
			:db_obs_ra     :ind_obs_ra,
			:db_obs_dec    :ind_obs_dec,
			:db_obs_alt    :ind_obs_alt,
			:db_obs_az     :ind_obs_az,
			:db_obs_slew   :ind_obs_slew,
			:db_obs_start  :ind_obs_start,
			:db_obs_state,
			:db_obs_end    :ind_obs_end
		FROM
			observations,
			targets
		WHERE
			obs_id = :db_obs_id
		AND observations.tar_id = targets.tar_id;
	if (sqlca.sqlcode)
	{
		logStream (MESSAGE_ERROR) << "Rts2Obs::load DB error: " << sqlca.sqlerrm.sqlerrmc << " (" << sqlca.sqlcode << ")" << sendLog;
		EXEC SQL ROLLBACK;
		return -1;
	}
	EXEC SQL COMMIT;
	tar_id = db_tar_id;
	tar_name = std::string (db_tar_name.arr);
	tar_type = db_tar_type;
	if (ind_obs_ra >= 0)
		obs_ra = db_obs_ra;
	if (ind_obs_dec >= 0)
		obs_dec = db_obs_dec;
	if (ind_obs_alt >= 0)
		obs_alt = db_obs_alt;
	if (ind_obs_az >= 0)
		obs_az = db_obs_az;
	if (ind_obs_slew >= 0)
		obs_slew = db_obs_slew;
	if (ind_obs_start >= 0)
		obs_start = db_obs_start;
	obs_state = db_obs_state;
	if (ind_obs_end >= 0)
		obs_end = db_obs_end;
	return 0;
}


int
Rts2Obs::loadImages ()
{
	int ret;
	if (imgset)
		// already loaded..
		return 0;
	ret = load ();
	if (ret)
		return ret;

	imgset = new Rts2ImgSetObs (this);
	return imgset->load ();
}


int
Rts2Obs::loadCounts ()
{
	EXEC SQL BEGIN DECLARE SECTION;
		int db_obs_id = obs_id;
		long db_count_date;
		int db_count_usec;
		int db_count_value;
		float db_count_exposure;
		char db_count_filter;
	EXEC SQL END DECLARE SECTION;

	EXEC SQL DECLARE cur_counts CURSOR FOR
		SELECT
			EXTRACT (EPOCH FROM count_date),
			count_usec,
			count_value,
			count_exposure,
			count_filter
		FROM
			counts
		WHERE
			obs_id = :db_obs_id;

	EXEC SQL OPEN cur_counts;
	while (1)
	{
		EXEC SQL FETCH next FROM cur_counts INTO
				:db_count_date,
				:db_count_usec,
				:db_count_value,
				:db_count_exposure,
				:db_count_filter;
		if (sqlca.sqlcode)
			break;
		counts.push_back (Rts2Count (obs_id, db_count_date, db_count_usec, db_count_value, db_count_exposure,
			db_count_filter));
	}
	if (sqlca.sqlcode != ECPG_NOT_FOUND)
	{
		logStream (MESSAGE_DEBUG) << "Rts2Obs::loadCounts DB error: " << sqlca.sqlerrm.sqlerrmc << " (" << sqlca.sqlcode << sendLog;
		EXEC SQL CLOSE cur_counts;
		EXEC SQL ROLLBACK;
		return -1;
	}
	EXEC SQL CLOSE cur_counts;
	EXEC SQL COMMIT;
	return 0;
}


void
Rts2Obs::printObsHeader (std::ostream & _os)
{
	if (!printHeader)
		return;
	std::ios_base::fmtflags old_settings = _os.flags ();
	int old_precision = _os.precision ();

	_os.setf (std::ios_base::fixed, std::ios_base::floatfield);
	_os.precision (2);
	_os << std::setw (5) << obs_id << SEP
		<< std::setw(6) << tar_id << SEP
		<< std::left << std::setw (20) << tar_name << std::right << SEP
		<< Timestamp (obs_slew) << SEP
		<< LibnovaRa (obs_ra) << SEP
		<< LibnovaDec (obs_dec) << SEP
		<< LibnovaDeg90 (obs_alt) << SEP
		<< LibnovaDeg (obs_az) << SEP
		<< std::setfill (' ');

	if (obs_start > 0)
		_os << std::setw (12) << TimeDiff (obs_slew, obs_start) << SEP;
	else
		_os << std::setw (12) << "not" << SEP;

	if (!isnan (obs_end))
		_os << std::setw (12) << TimeDiff (obs_start, obs_end) << SEP;
	else
		_os << std::setw (12) << "not" << SEP;

	_os << SEP
		<< std::setw (7) << getSlewSpeed() << SEP
		<< Rts2ObsState (obs_state)
		<< std::endl;

	_os.flags (old_settings);
	_os.precision (old_precision);
}


void
Rts2Obs::printCountsShort (std::ostream &_os)
{
	std::vector <Rts2Count>::iterator count_iter;
	if (counts.empty ())
	{
		return;
	}
	for (count_iter = counts.begin (); count_iter != counts.end (); count_iter++)
	{
		_os
			<< tar_id
			<< SEP << obs_id
			<< SEP << (*count_iter);
	}
}


void
Rts2Obs::printCounts (std::ostream &_os)
{
	std::vector <Rts2Count>::iterator count_iter;
	if (counts.empty ())
	{
		_os << "      " << "--- no counts ---" << std::endl;
		return;
	}
	for (count_iter = counts.begin (); count_iter != counts.end (); count_iter++)
	{
		_os << "      " << (*count_iter);
	}
}


void
Rts2Obs::printCountsSummary (std::ostream &_os)
{
	_os << "       Number of counts:" << counts.size () << std::endl;
}


double
Rts2Obs::altitudeMerit (double _start, double _end)
{
	double minA, maxA;
	struct ln_hrz_posn hrz;
	getTarget ()->getMinMaxAlt (_start, _end, minA, maxA);

	getTarget ()->getAltAz (&hrz, getObsJDMid ());

	if (maxA == minA)
		return 1;

	if ((hrz.alt - minA) / (maxA - minA) > 1)
	{
		std::cout << "hrz.alt: " << hrz.alt
			<< " minA: " << minA
			<< " maxA: " << maxA
			<< " from " << LibnovaDate (_start)
			<< " to " << LibnovaDate (_end)
			<< " obs from " << LibnovaDate (getObsJDStart ())
			<< " to " << LibnovaDate (getObsJDEnd ())
			<< std::endl;
	}

	if (isnan (hrz.alt) || isnan (minA) || isnan (maxA))
	{
		std::cout << "nan hrz.alt: " << hrz.alt
			<< " minA: " << minA
			<< " maxA: " << maxA
			<< " from " << LibnovaDate (_start)
			<< " to " << LibnovaDate (_end)
			<< " obs from " << LibnovaDate (getObsJDStart ())
			<< " to " << LibnovaDate (getObsJDEnd ())
			<< std::endl;
	}

	return (hrz.alt - minA) / (maxA - minA);
}


int
Rts2Obs::getUnprocessedCount ()
{
	EXEC SQL BEGIN DECLARE SECTION;
		int db_obs_id = getObsId ();
		int db_count = 0;
	EXEC SQL END DECLARE SECTION;

	EXEC SQL
		SELECT
		count (*)
		INTO
			:db_count
		FROM
			images
		WHERE
			obs_id = :db_obs_id
		AND ((process_bitfield & 1) = 0);
	EXEC SQL ROLLBACK;

	return db_count;
}


int
Rts2Obs::checkUnprocessedImages (Rts2Block *master)
{
	int ret;
	load ();
	if (isnan (obs_end))
		return -1;
	// obs_end is not null - observation ends sucessfully
	// get unprocessed counts..
	ret = getUnprocessedCount ();
/*	if (ret == 0)
	{
		int count;
		Rts2TarUser tar_user = Rts2TarUser (getTargetId (), getTargetType ());
		std::string mails = tar_user.getUsers (SEND_ASTRO_OK, count);
		if (count == 0)
			return ret;
		std::ostringstream subject;
		subject << "TARGET #" << getTargetId ()
			<< ", OBSERVATION " << getObsId ()
			<< " ALL IMAGES PROCESSED";
		std::ostringstream os;
		os << *this;
		master->sendMailTo (subject.str().c_str(), os.str().c_str(), mails.c_str());
	} */
	return ret;
}


int
Rts2Obs::getNumberOfImages ()
{
	loadImages ();
	if (imgset)
		return imgset->size ();
	return 0;
}


int
Rts2Obs::getNumberOfGoodImages ()
{
	loadImages ();
	if (!imgset)
		return 0;
	std::vector <Rts2Image *>::iterator img_iter;
	int ret = 0;
	for (img_iter = imgset->begin (); img_iter != imgset->end (); img_iter++)
	{
		if ((*img_iter)->haveOKAstrometry ())
			ret++;
	}
	return ret;
}


int
Rts2Obs::getFirstErrors (double &eRa, double &eDec, double &eRad)
{
	loadImages ();
	if (!imgset)
		return -1;

	std::vector <Rts2Image *>::iterator img_iter;
	for (img_iter = imgset->begin (); img_iter != imgset->end (); img_iter++)
	{
		if (((*img_iter)->getError (eRa, eDec, eRad)) == 0)
		{
			return 0;
		}
	}
	return -1;
}


int
Rts2Obs::getAverageErrors (double &eRa, double &eDec, double &eRad)
{
	loadImages ();
	if (!imgset)
		return -1;
	return imgset->getAverageErrors (eRa, eDec, eRad);
}


int
Rts2Obs::getPrevPosition (struct ln_equ_posn &prevEqu, struct ln_hrz_posn &prevHrz)
{
	int ret;
	Rts2Obs prevObs = Rts2Obs (getObsId () - 1);
	ret = prevObs.load();
	if (ret)
		return ret;
	prevObs.getEqu (prevEqu);
	prevObs.getHrz (prevHrz);
	return 0;
}


double
Rts2Obs::getPrevSeparation ()
{
	struct ln_equ_posn prevEqu, currEqu;
	struct ln_hrz_posn prevHrz;
	int ret;
	ret = getPrevPosition (prevEqu, prevHrz);
	if (ret)
		return nan("f");

	getEqu (currEqu);

	return ln_get_angular_separation (&prevEqu, &currEqu);
}


double
Rts2Obs::getSlewSpeed ()
{
	double prevSep = getPrevSeparation ();
	if (isnan (prevSep)
		|| isnan(obs_slew)
		|| isnan (obs_start)
		|| (obs_start - obs_slew) <= 0)
		return nan("f");
	return prevSep / (obs_start - obs_slew);
}


double
Rts2Obs::getSlewTime ()
{
	if (isnan (obs_slew) || isnan (obs_start))
		return nan ("f");
	return obs_start - obs_slew;
}


double
Rts2Obs::getObsTime ()
{
	if (isnan (obs_start) || isnan (obs_end))
		return nan ("f");
	return obs_end - obs_slew;
}


void
Rts2Obs::maskState (int newBits)
{
	EXEC SQL BEGIN DECLARE SECTION;
		int db_obs_id = obs_id;
		int db_newBits = newBits;
	EXEC SQL END DECLARE SECTION;

	EXEC SQL UPDATE
			observations
		SET
			obs_state = obs_state | :db_newBits
		WHERE
			obs_id = :db_obs_id;
	if (sqlca.sqlcode)
	{
		logStream (MESSAGE_ERROR) << "Rts2Obs::maskState: " << sqlca.sqlerrm.sqlerrmc << " (" << sqlca.sqlcode << ")" << sendLog;
		EXEC SQL ROLLBACK;
		return;
	}
	EXEC SQL COMMIT;
	obs_state |= newBits;
}


void
Rts2Obs::unmaskState (int newBits)
{
	EXEC SQL BEGIN DECLARE SECTION;
		int db_obs_id = obs_id;
		int db_newBits = ~newBits;
	EXEC SQL END DECLARE SECTION;

	EXEC SQL UPDATE
			observations
		SET
			obs_state = obs_state & :db_newBits
		WHERE
			obs_id = :db_obs_id;
	if (sqlca.sqlcode)
	{
		logStream (MESSAGE_ERROR) << "Rts2Obs::unmaskState: " << sqlca.sqlerrm.sqlerrmc << " (" << sqlca.sqlcode << ")" << sendLog;
		EXEC SQL ROLLBACK;
		return;
	}
	EXEC SQL COMMIT;
	obs_state |= newBits;
}


std::ostream & operator << (std::ostream &_os, Rts2Obs &obs)
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


std::ostream & operator << (std::ostream &_os, Rts2ObsState obs_state)
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
