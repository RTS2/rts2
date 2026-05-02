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

#include "imgdisplay.h"

#include "rts2db/observationset.h"
#include "rts2db/sqlerror.h"
#include "rts2db/target.h"
#include "rts2db/devicedb.h"

#include <algorithm>
#include <sstream>

#include "libnova_cpp.h"
#include "configuration.h"

using namespace rts2db;

ObservationSet::ObservationSet (void)
{
	images = 0;
	counts = 0;
	collocated = false;
	successNum = 0;
	failedNum = 0;

	imageFormat = NULL;

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

ObservationSet::~ObservationSet (void)
{
	clear ();
}

void ObservationSet::loadTarget (int _tar_id, const time_t * start_t, const time_t * end_t)
{
	std::ostringstream os;
	os << "observations.tar_id = " << _tar_id;
	if (start_t)
	{
		os << " AND observations.obs_slew >= to_timestamp (" << *start_t<< ")";
	}
	if (end_t)
	{
	 	os << " AND ((observations.obs_slew <= to_timestamp ("
		<< *end_t
		<< ") AND (observations.obs_end is NULL OR observations.obs_end < to_timestamp ("
		<< *end_t
		<< "))";
	}
	load (os.str ());
}

void ObservationSet::loadTime (const time_t * start_t, const time_t * end_t)
{
	std::ostringstream os;
	if (start_t)
	{
		os << "observations.obs_slew >= to_timestamp (" << *start_t << ")";
	}
	if (end_t)
	{
		if (start_t)
			os << " AND ";
		os << "((observations.obs_slew <= to_timestamp ("
			<< *end_t
			<< ") AND observations.obs_end is NULL) OR observations.obs_end < to_timestamp ("
			<< *end_t
			<< "))";
	}
	load (os.str ());
}

void ObservationSet::loadType (char type_id, int state_mask, bool inv)
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
	load (os.str ());
}

void ObservationSet::loadRadius (struct ln_equ_posn * position, double radius)
{
	std::ostringstream os;
	os << "ln_angular_separation (observations.obs_ra, observations.obs_dec, "
		<< position->ra << ", "
		<< position->dec << ") < "
		<< radius;
	load (os.str ());
}

void ObservationSet::loadLabel (int label_id)
{
	std::ostringstream os;
	os << "exists (select * from target_labels where observations.tar_id = target_labels.tar_id and label_id = " << label_id << ")";
	load (os.str ());
}

void ObservationSet::load (std::string in_where)
{
	if (checkDbConnection ())
		throw SqlError ();

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
	int db_plan_id;

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
	int db_plan_id_ind;
	EXEC SQL END DECLARE SECTION;

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
		"EXTRACT (EPOCH FROM obs_end),"
		"plan_id"
		" FROM "
		"observations, targets"
		" WHERE "
		"observations.tar_id = targets.tar_id "
		"AND " << in_where <<
		" ORDER BY obs_id ASC;";
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
				:db_obs_end :db_obs_end_ind,
				:db_plan_id :db_plan_id_ind;
		if (sqlca.sqlcode)
			break;
		if (db_tar_ind < 0)
			db_tar_name.arr[0] = '\0';
		if (db_obs_ra_ind < 0)
			db_obs_ra = NAN;
		if (db_obs_dec_ind < 0)
			db_obs_dec = NAN;
		if (db_obs_alt_ind < 0)
			db_obs_alt = NAN;
		if (db_obs_az_ind < 0)
			db_obs_az = NAN;
		if (db_obs_slew_ind < 0)
			db_obs_slew = NAN;
		if (db_obs_start_ind < 0)
			db_obs_start = NAN;
		if (db_obs_state_ind < 0)
			db_obs_state = 0;
		if (db_obs_end_ind < 0)
			db_obs_end = NAN;
		if (db_plan_id_ind < 0)
			db_plan_id = -1;

		// add new observations to vector
		Observation obs = Observation (db_tar_id, db_tar_name.arr, db_tar_type, db_obs_id, db_obs_ra, db_obs_dec, db_obs_alt,
			db_obs_az, db_obs_slew, db_obs_start, db_obs_state, db_obs_end, db_plan_id);
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
		EXEC SQL CLOSE obs_cur_timestamps;
		EXEC SQL ROLLBACK;
		throw SqlError ();
	}
	EXEC SQL CLOSE obs_cur_timestamps;
	EXEC SQL ROLLBACK;
}

int ObservationSet::computeStatistics ()
{
	int anum = 0;

	std::vector <Observation>::iterator obs_iter;
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

void ObservationSet::printStatistics (std::ostream & _os)
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
			<< "(" << (size () > 0 ? 100 * firstNum / size () : NAN) << "%)" << std::endl
			<< "First images errors: ra " << LibnovaDegArcMin (errFirstRa)
			<< " dec: " << LibnovaDegArcMin (errFirstDec)
			<< " radius: " << LibnovaDegArcMin (errFirstRad) << std::endl
			<< "Average error ra: " << LibnovaDegArcMin (errAvgRa)
			<< " dec: " << LibnovaDegArcMin (errAvgDec)
			<< " radius: " << LibnovaDegArcMin (errAvgRad) << std::endl;
	}
	_os.precision (prec);
}

std::map <int, int> ObservationSet::getTargetObservations ()
{
	std::map <int, int> ret;
	for (ObservationSet::iterator iter = begin (); iter != end (); iter++)
	{
		ret[(*iter).getTargetId ()]++;
	}
	return ret;
}

double ObservationSet::altitudeMerit ()
{
	double altMerit = 0;
	int s = 0;
	for (ObservationSet::iterator iter = begin (); iter != end (); iter++)
	{
		double am = (*iter).altitudeMerit (getJDStart (), getJDEnd ());
		if (std::isnan (am))
			continue;
		altMerit += am;
		s++;
	}
	altMerit /= s;
	return altMerit;
}

double ObservationSet::distanceMerit ()
{
	double distMerit = 0;

	unsigned int s = 0;

	ObservationSet::iterator iter1 = begin ();
	ObservationSet::iterator iter2 = begin () + 1;

	for (; iter2 != end (); iter1++, iter2++)
	{
		struct ln_equ_posn pos1, pos2;
		if ((*iter1).getEndPosition (pos1) == 0 && (*iter2).getStartPosition (pos2) == 0)
		{
			distMerit += ln_get_angular_separation (&pos1, &pos2);
			s++;
		}
	}
	if (distMerit == 0)
	{
		distMerit = 1;
	}
	else
	{
		distMerit = distMerit / s;
	}

	return distMerit;
}

std::ostream & ObservationSet::print (std::ostream &_os)
{
	// list of target ids we already printed
	std::vector <int> processedTargets;

	std::vector <Observation>::iterator obs_iter;

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
				for (std::vector <Observation>::iterator obs_iter2 = obs_iter; obs_iter2 != end (); obs_iter2 ++)
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
				for (std::vector <Observation>::iterator obs_iter2 = obs_iter; obs_iter2 != end (); obs_iter2 ++)
				{
					if ((*obs_iter2).getTargetId () == tar_id)
					{
						if (getPrintImages () == 0 && getPrintCounts () == 0 && imageFormat == NULL)
						{
							// print time of observation
							_os << Timestamp ((*obs_iter2).getObsStart ()) << SEP;
						}
						else
						{
							(*obs_iter2).setPrintImages (getPrintImages (), imageFormat);
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
			obs_iter->setPrintImages (getPrintImages (), imageFormat);
			obs_iter->setPrintCounts (getPrintCounts ());
			_os << (*obs_iter);
		}
	}
	return _os;
}

double ObservationSet::getNextCtime ()
{
	if (timeLogIter != end ())
		return timeLogIter->getObsSlew ();
	return NAN;
}

void ObservationSet::printUntil (double time, std::ostream &os)
{
	while (timeLogIter != end () && timeLogIter->getObsSlew () <= time)
	{
		timeLogIter->setPrintImages (getPrintImages (), imageFormat);
		timeLogIter->setPrintCounts (getPrintCounts ());
		os << (*timeLogIter);

		timeLogIter++;
	}
}

void ObservationSetDate::load (int year, int month, int day, int hour, int minutes)
{
	if (checkDbConnection ())
		throw SqlError ();

	EXEC SQL BEGIN DECLARE SECTION;
	int d_value1;
	int d_value2;
	int d_value2g;
	int d_c;
	int d_i;
	int d_gi;
	double d_tt;
	char *stmp_c;
	EXEC SQL END DECLARE SECTION;

	const char *group_by;
	std::ostringstream _where;

	double lng = rts2core::Configuration::instance ()->getObserver ()->lng;

	if (year == -1)
	{
		group_by = "year";
	}
	else
	{
		_where << "EXTRACT (year FROM to_night (obs_slew, " << lng << ")) = " << year;
		if (month == -1)
		{
			group_by = "month";
		}
		else
		{
			_where << " AND EXTRACT (month FROM to_night (obs_slew, " << lng << ")) = " << month;
			if (day == -1)
			{
				group_by = "day";
			}
			else
			{
				_where << " AND EXTRACT (day FROM to_night (obs_slew, " << lng << ")) = " << day;
				if (hour == -1)
				{
				 	group_by = "hour";
				}
				else
				{
					_where << " AND EXTRACT (hour FROM obs_slew) = " << hour;
					if (minutes == -1)
					{
					 	group_by = "minute";
					}
					else
					{
						_where << " AND EXTRACT (minutes FROM obs_slew) = " << minutes;
						group_by = "second";
					}
				}
			}
		}
	}

	std::ostringstream _os;
	_os << "SELECT EXTRACT (" << group_by << " FROM to_night (obs_slew, " << lng << ")) as value, count (observations.*) as c FROM observations";

	if (_where.str ().length () > 0)
		_os << " WHERE " << _where.str ();

	_os << " GROUP BY value ORDER BY value;";

	stmp_c = new char[_os.str ().length () + 1];
	memcpy (stmp_c, _os.str().c_str(), _os.str ().length () + 1);
	EXEC SQL PREPARE obsdate_stmp FROM :stmp_c;

	std::cout << _os.str() << std::endl;

	delete[] stmp_c;

	EXEC SQL DECLARE obsdate_cur CURSOR FOR obsdate_stmp;

	EXEC SQL OPEN obsdate_cur;

	std::ostringstream _osi;
	std::ostringstream _osig;
	_osi << "SELECT EXTRACT (" << group_by << " FROM to_night (obs_slew, " << lng << ")) as value, count (im_all.*) as i, sum(im_all.img_exposure) as tt FROM observations, images im_all WHERE observations.obs_id = im_all.obs_id";

	if (_where.str ().length () > 0)
		_osi << " and " << _where.str ();

	_osig << _osi.str ();
	_osig << " AND im_all.process_bitfield & 8 = 8 GROUP BY value ORDER BY value;";

	_osi << " GROUP BY value ORDER BY value;";

	stmp_c = new char[_osi.str ().length () + 1];
	memcpy (stmp_c, _osi.str().c_str(), _osi.str ().length () + 1);
	EXEC SQL PREPARE obsdateimg_stmp FROM :stmp_c;

	delete[] stmp_c;

	EXEC SQL DECLARE obsdateimg_cur CURSOR FOR obsdateimg_stmp;

	EXEC SQL OPEN obsdateimg_cur;

	stmp_c = new char[_osig.str ().length () + 1];
	memcpy (stmp_c, _osig.str().c_str(), _osig.str ().length () + 1);
	EXEC SQL PREPARE obsdateimggood_stmp FROM :stmp_c;

	delete[] stmp_c;

	EXEC SQL DECLARE obsdateimggood_cur CURSOR FOR obsdateimggood_stmp;

	EXEC SQL OPEN obsdateimggood_cur;

	d_value2 = -1;
	d_value2g = -1;

	while (1)
	{
		EXEC SQL FETCH next FROM obsdate_cur INTO
			:d_value1,
			:d_c;
		if (sqlca.sqlcode)
			break;
		if (d_value2 < 0)
		{
			EXEC SQL FETCH next FROM obsdateimg_cur INTO
				:d_value2,
				:d_i,
				:d_tt;
			if (sqlca.sqlcode)
			{
				if (sqlca.sqlcode == ECPG_NOT_FOUND)
				{
					(*this)[d_value1] = DateStatistics (d_c);
				}
				else
				{
				  	break;
				}
			}
		}
		else
		{
			d_i = 0;
			d_tt = NAN;
		}

		if (d_value2g < 0)
		{
			EXEC SQL FETCH next FROM obsdateimggood_cur INTO
				:d_value2g,
				:d_gi;
			if (sqlca.sqlcode)
			{
				if (sqlca.sqlcode == ECPG_NOT_FOUND)
				{
					d_gi = 0;
				}
				else
				{
					break;
				}
			}
		}
		else
		{
			d_gi = 0;
			d_tt = NAN;
		}

		if (d_value1 == d_value2)
		{
			(*this)[d_value1] = DateStatistics (d_c, d_i, d_gi, d_tt);
			d_value2 = -1;
			if (d_value2 == d_value2g)
				d_value2g = -1;
		}
		else
		{
			(*this)[d_value1] = DateStatistics (d_c);
		}
	}
	if (sqlca.sqlcode != ECPG_NOT_FOUND)
	{
		EXEC SQL CLOSE obsdate_cur;
		EXEC SQL CLOSE obsdateimg_cur;
		EXEC SQL CLOSE obsdateimggood_cur;
		EXEC SQL ROLLBACK;

		throw SqlError ();
	}

	EXEC SQL CLOSE obsdate_cur;
	EXEC SQL CLOSE obsdateimg_cur;
	EXEC SQL CLOSE obsdateimggood_cur;
	EXEC SQL ROLLBACK;
}

int rts2db::lastObservationId ()
{
	if (checkDbConnection ())
		return -1;

	EXEC SQL BEGIN DECLARE SECTION;
	int d_obs_id;
	EXEC SQL END DECLARE SECTION;

	EXEC SQL SELECT max(obs_id) INTO :d_obs_id FROM observations WHERE obs_start is not null;

	if (sqlca.sqlcode)
		return -1;
	return d_obs_id;
}
