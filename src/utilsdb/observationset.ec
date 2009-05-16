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

#include "imgdisplay.h"
#include "observationset.h"
#include "target.h"

#include <algorithm>
#include <sstream>

#include "../utils/libnova_cpp.h"
#include "../utils/rts2config.h"

using namespace rts2db;

void
ObservationSet::load (std::string in_where)
{
	EXEC SQL BEGIN DECLARE SECTION;
		char *stmp_c;

		// cannot use TARGET_NAME_LEN, as it does not work with some ecpg veriosn
		VARCHAR db_tar_name[150];
		int db_tar_id;
		int db_obs_id;
		double db_obs_ra;
		double db_obs_dec;
		double db_obs_alt;
		double db_obs_az;
		double db_obs_slew;
		double db_obs_start;
		int db_obs_state;
		double db_obs_end;

		int db_tar_ind;
		char db_tar_type;
		int db_obs_ra_ind;
		int db_obs_dec_ind;
		int db_obs_alt_ind;
		int db_obs_az_ind;
		int db_obs_slew_ind;
		int db_obs_start_ind;
		int db_obs_state_ind;
		int db_obs_end_ind;
	EXEC SQL END DECLARE SECTION;

	initObservationSet ();

	std::ostringstream _os;
	_os << 
		"SELECT "
		"tar_name,"
		"observations.tar_id,"
		"type_id,"
		"obs_id,"
		"obs_ra,"
		"obs_dec,"
		"obs_alt,"
		"obs_az,"
		"EXTRACT (EPOCH FROM obs_slew),"
		"EXTRACT (EPOCH FROM obs_start),"
		"obs_state,"
		"EXTRACT (EPOCH FROM obs_end)"
		" FROM "
		"observations, targets"
		" WHERE "
		"observations.tar_id = targets.tar_id "
		"AND " << in_where << 
		" ORDER BY obs_id DESC;";
	stmp_c  = new char[_os.str ().length () + 1];
	strcpy (stmp_c, _os.str ().c_str ());

	EXEC SQL PREPARE obs_stmp FROM :stmp_c;

	delete[] stmp_c;

	EXEC SQL DECLARE obs_cur_timestamps CURSOR FOR obs_stmp;

	EXEC SQL OPEN obs_cur_timestamps;
	while (1)
	{
		EXEC SQL FETCH next FROM obs_cur_timestamps INTO
				:db_tar_name :db_tar_ind,
				:db_tar_id,
				:db_tar_type,
				:db_obs_id,
				:db_obs_ra :db_obs_ra_ind,
				:db_obs_dec :db_obs_dec_ind,
				:db_obs_alt :db_obs_alt_ind,
				:db_obs_az :db_obs_az_ind,
				:db_obs_slew :db_obs_slew_ind,
				:db_obs_start :db_obs_start_ind,
				:db_obs_state :db_obs_state_ind,
				:db_obs_end :db_obs_end_ind;
		if (sqlca.sqlcode)
			break;
		if (db_tar_ind < 0)
			db_tar_name.arr[0] = '\0';
		if (db_obs_ra_ind < 0)
			db_obs_ra = nan("f");
		if (db_obs_dec_ind < 0)
			db_obs_dec = nan("f");
		if (db_obs_alt_ind < 0)
			db_obs_alt = nan("f");
		if (db_obs_az_ind < 0)
			db_obs_az = nan("f");
		if (db_obs_slew_ind < 0)
			db_obs_slew = nan("f");
		if (db_obs_start_ind < 0)
			db_obs_start = nan("f");
		if (db_obs_state_ind < 0)
			db_obs_state = 0;
		if (db_obs_end_ind < 0)
			db_obs_end = nan("f");

		// add new observations to vector
		Rts2Obs obs = Rts2Obs (db_tar_id, db_tar_name.arr, db_tar_type, db_obs_id, db_obs_ra, db_obs_dec, db_obs_alt,
			db_obs_az, db_obs_slew, db_obs_start, db_obs_state, db_obs_end);
		push_back (obs);
		if (db_obs_state & OBS_BIT_STARTED)
		{
			if (db_obs_state & OBS_BIT_ACQUSITION_FAI)
				failedNum++;
			else
				successNum++;
		}
	}
	if (sqlca.sqlcode != ECPG_NOT_FOUND)
	{
		logStream (MESSAGE_ERROR) << "ObservationSet::ObservationSet cannot load observation set:"
			<< sqlca.sqlerrm.sqlerrmc << sendLog;
	}
	EXEC SQL CLOSE obs_cur_timestamps;
	EXEC SQL ROLLBACK;
}


ObservationSet::ObservationSet (void)
{
}


ObservationSet::ObservationSet (int in_tar_id, const time_t * start_t, const time_t * end_t)
{
	std::ostringstream os;
	os << "observations.tar_id = "
		<< in_tar_id
		<< " AND observations.obs_slew >= to_timestamp ("
		<< *start_t
		<< ") AND ((observations.obs_slew <= to_timestamp ("
		<< *end_t
		<< ") AND (observations.obs_end is NULL OR observations.obs_end < to_timestamp ("
		<< *end_t
		<< "))";
	load (os.str());
}


ObservationSet::ObservationSet (const time_t * start_t, const time_t * end_t)
{
	std::ostringstream os;
	os << "observations.obs_slew >= to_timestamp ("
		<< *start_t
		<< ") AND ((observations.obs_slew <= to_timestamp ("
		<< *end_t
		<< ") AND observations.obs_end is NULL) OR observations.obs_end < to_timestamp ("
		<< *end_t
		<< "))";
	load (os.str());
}


ObservationSet::ObservationSet (int in_tar_id)
{
	std::ostringstream os;
	os << "observations.tar_id = "
		<< in_tar_id;
	load (os.str());
}


ObservationSet::ObservationSet (int year, int month)
{
	time_t start_t = Rts2Config::instance ()->getNight (year,month,1);
	month++;
	if (month > 12)
	{
		year++;
		month -= 12;
	}
	time_t end_t = Rts2Config::instance ()->getNight (year,month, 1);
	std::ostringstream os;
	os << "observations.obs_slew >= to_timestamp ("
		<< start_t
		<< ") AND ((observations.obs_slew <= to_timestamp ("
		<< end_t
		<< ") AND observations.obs_end is NULL) OR observations.obs_end < to_timestamp ("
		<< end_t
		<< "))";
	load (os.str());
}


ObservationSet::ObservationSet (char type_id, int state_mask, bool inv)
{
	std::ostringstream os;
	os << "(targets.type_id = "
		<< type_id
		<< ") and ((observations.obs_state & "
		<< state_mask
		<< ") ";
	if (inv)
		os << "!";
	os << "= "
		<< state_mask
		<< ")";
	load (os.str());
}


ObservationSet::ObservationSet (struct ln_equ_posn * position, double radius)
{
	std::ostringstream os;
	os << "ln_angular_separation (observations.obs_ra, observations.obs_dec, "
		<< position->ra << ", "
		<< position->dec << ") < "
		<< radius;
	load (os.str());
}


ObservationSet::~ObservationSet (void)
{
	clear ();
}


int
ObservationSet::computeStatistics ()
{
	int anum = 0;

	std::vector <Rts2Obs>::iterator obs_iter;
	allNum = 0;
	goodNum = 0;
	firstNum = 0;

	errFirstRa = 0;
	errFirstDec = 0;
	errFirstRad = 0;

	errAvgRa = 0;
	errAvgDec = 0;
	errAvgRad = 0;

	for (obs_iter = begin (); obs_iter != end (); obs_iter++)
	{
		double efRa, efDec, efRad;
		double eaRa, eaDec, eaRad;
		int ret;
		allNum += (*obs_iter).getNumberOfImages ();
		goodNum += (*obs_iter).getNumberOfGoodImages ();
		ret = (*obs_iter).getFirstErrors (efRa, efDec, efRad);
		if (!ret)
		{
			errFirstRa += efRa;
			errFirstDec += efDec;
			errFirstRad += efRad;
			firstNum++;
		}
		ret = (*obs_iter).getAverageErrors (eaRa, eaDec, eaRad);
		if (ret > 0)
		{
			errAvgRa += eaRa;
			errAvgDec += eaDec;
			errAvgRad += eaRad;
			anum ++;
		}
	}

	errFirstRa /= firstNum;
	errFirstDec /= firstNum;
	errFirstRad /= firstNum;

	errAvgRa /= anum;
	errAvgDec /= anum;
	errAvgRad /= anum;

	return 0;
}


void
ObservationSet::printStatistics (std::ostream & _os)
{
	computeStatistics ();
	int success = getSuccess ();
	int failed = getFailed ();
	int prec = _os.precision (2);
	_os << "Number of observations: " << size () << std::endl
		<< "Succesfully ended:" << success << std::endl
		<< "Failed:" << failed << std::endl
		<< "Number of images:" << allNum
		<< " with astrometry:" << goodNum
		<< " without astrometry:" << (allNum - goodNum);
	if (allNum > 0)
	{
		_os << " (" << (((double)(goodNum * 100)) / allNum) << "%)";
	}
	_os << std::endl;
	if (goodNum > 0)
	{
		_os
			<< "First images : " << firstNum << " from " << size ()
			<< "(" << (size () > 0 ? 100 * firstNum / size () : nan ("f")) << "%)" << std::endl
			<< "First images errors: ra " << LibnovaDegArcMin (errFirstRa)
			<< " dec: " << LibnovaDegArcMin (errFirstDec)
			<< " radius: " << LibnovaDegArcMin (errFirstRad) << std::endl
			<< "Average error ra: " << LibnovaDegArcMin (errAvgRa)
			<< " dec: " << LibnovaDegArcMin (errAvgDec)
			<< " radius: " << LibnovaDegArcMin (errAvgRad) << std::endl;
	}
	_os.precision (prec);
}


std::ostream &
ObservationSet::print (std::ostream &_os)
{
	// list of target ids we already printed
	std::vector <int> processedTargets;

	std::vector <Rts2Obs>::iterator obs_iter;

	if (!isCollocated ())
		_os << "Observations list follow:" << std::endl;

	for (obs_iter = begin (); obs_iter != end (); obs_iter++)
	{
		if (isCollocated ())
		{
			int tar_id = (*obs_iter).getTargetId ();
			if (find (processedTargets.begin (), processedTargets.end (), tar_id) == processedTargets.end ())
			{
				processedTargets.push_back (tar_id);
				int totalImages = 0;
				int goodImages = 0;
				double totalObsTime = 0;
				double totalSlewTime = 0;
				int obsNum = 0;
				// compute target image statitics..
				for (std::vector <Rts2Obs>::iterator obs_iter2 = obs_iter; obs_iter2 != end (); obs_iter2 ++)
				{
					if ((*obs_iter2).getTargetId () == tar_id)
					{
						totalImages += (*obs_iter2).getNumberOfImages ();
						goodImages += (*obs_iter2).getNumberOfGoodImages ();

						totalSlewTime += (*obs_iter2).getSlewTime ();
						totalObsTime += (*obs_iter2).getObsTime ();

						obsNum++;
					}
				}

				_os << "  " << std::setw (6) << (*obs_iter).getTargetId () 
					<< SEP << std::setw (20) << (*obs_iter).getTargetName ()
					<< SEP << std::setw (3) << obsNum
					<< SEP << TimeDiff (totalObsTime + totalSlewTime) 
					<< SEP << TimeDiff (totalObsTime)
					<< SEP << TimeDiff (totalSlewTime)
					<< SEP << std::setw (5) << totalImages
					<< SEP << std::setw (5) << goodImages
					<< std::endl;

				// print record that we found the observation
				// find all observations of target
				for (std::vector <Rts2Obs>::iterator obs_iter2 = obs_iter; obs_iter2 != end (); obs_iter2 ++)
				{
					if ((*obs_iter2).getTargetId () == tar_id)
					{
						if (getPrintImages () == 0 && getPrintCounts () == 0)
						{
							// print time of observation
							_os << Timestamp ((*obs_iter2).getObsStart ()) << SEP;
						}
						else
						{
							(*obs_iter2).setPrintImages (getPrintImages ());
							(*obs_iter2).setPrintCounts (getPrintCounts ());
							(*obs_iter2).setPrintHeader (false);
							_os << (*obs_iter2);
						}
					}
				}
				if (getPrintImages () == 0 && getPrintCounts () == 0)
				{
					_os << std::endl;
				}
			}
		}
		else
		{
			(*obs_iter).setPrintImages (getPrintImages ());
			(*obs_iter).setPrintCounts (getPrintCounts ());
			_os << (*obs_iter);
		}
	}
	return _os;
}
