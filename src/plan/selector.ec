/*
 * Selector body.
 * Copyright (C) 2003-2008 Petr Kubanek <petr@kubanek.net>
 * Copyright (C) 2011 Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
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

#include "selector.h"
#include "configuration.h"
#include "utilsfunc.h"

#include "rts2script/script.h"
#include "rts2db/sqlerror.h"

#include <libnova/libnova.h>

using namespace rts2plan;

Selector::Selector (rts2db::CamList *cameras)
{
	observer = NULL;
	cameraList = cameras;
}

Selector::~Selector (void)
{
	for (std::vector <TargetEntry *>::iterator iter = possibleTargets.begin (); iter != possibleTargets.end (); iter++)
	{
		delete *iter;
	}
}

void Selector::init ()
{
	Configuration *config;
	std::string doNotObserve;
	double val;

	config = Configuration::instance ();

	config->getString ("selector", "night_do_not_consider", doNotObserve);

	setNightDisabledTypes (doNotObserve.c_str ());

	flat_sun_min = 100;
	flat_sun_max = 100;
	config->getDouble ("observatory", "flat_sun_min", flat_sun_min);
	config->getDouble ("observatory", "flat_sun_max", flat_sun_max);

	if (flat_sun_min > flat_sun_max)
	{
		val = flat_sun_min;
		flat_sun_min = flat_sun_max;
		flat_sun_max = val;
	}
}

int Selector::selectNext (int masterState, double length)
{
	struct ln_equ_posn eq_sun;
	struct ln_hrz_posn sun_hrz;
	double JD;
	int ret;
	// take care of state - select to make darks when we are able to
	// make darks.
	switch (masterState & (SERVERD_STATUS_MASK | SERVERD_STANDBY_MASK))
	{
		case SERVERD_NIGHT:
			return selectNextNight (0, false, length);
			break;
		case SERVERD_DAWN:
		case SERVERD_DUSK:
			// if flats were requested and system is in ON
			if (!(masterState & BAD_WEATHER) && flat_sun_min < flat_sun_max)
			{
				JD = ln_get_julian_from_sys ();
				ln_get_solar_equ_coords (JD, &eq_sun);
				ln_get_hrz_from_equ (&eq_sun, observer, JD, &sun_hrz);
				if (sun_hrz.alt >= flat_sun_min && sun_hrz.alt <= flat_sun_max)
				{
					return selectFlats ();
				}
				else if (sun_hrz.alt < flat_sun_min)
				{
					// special case - select GRBs, which are new (=targets with priority higher then 1500)
					ret = selectNextNight (1500, false, length);
					if (ret != -1)
						return ret;
				}
			}
			// don't break, select darks
		case SERVERD_DAWN | SERVERD_STANDBY:
		case SERVERD_DUSK | SERVERD_STANDBY:
			// otherwise select darks/flats/whatever
			return selectDarks ();
			break;
	}
	return -1;					 // we don't have any target to take observation..
}

struct findTargetById
{
	int ct;

	bool operator () (TargetEntry *tar) const { return ct == tar->target->getTargetID (); }
};

void Selector::considerTarget (int consider_tar_id, double JD)
{
	rts2db::Target *newTar;
	int ret;

	findTargetById ct = { consider_tar_id };

	if (std::find_if (possibleTargets.begin (), possibleTargets.end (), ct) != possibleTargets.end ())
		return;

	// add us..
	newTar = createTarget (consider_tar_id, observer);
	if (!newTar)
		return;
	ret = newTar->considerForObserving (JD);
#ifdef DEBUG_EXTRA
	logStream (MESSAGE_DEBUG) << "considerForObserving tar_id: " << newTar->getTargetID () << " ret: " << ret << sendLog;
#endif
	if (ret)
	{
		delete newTar;
		return;
	}
	// check if all script filters are present
	for (std::map <std::string, std::vector < std::string > >::iterator iter = availableFilters.begin (); iter != availableFilters.end (); iter++)
	{
		std::string scripttext;
		newTar->getScript (iter->first.c_str (), scripttext);
		rts2script::Script script (scripttext.c_str ());
		script.parseScript (NULL);
		for (rts2script::Script::iterator se = script.begin (); se != script.end (); se++)
		{
		  	std::ostringstream os;
			(*se)->printScript (os);
			if (os.str ().find ("filter=") == 0)
			{
				std::string ops = ((rts2script::ElementChangeValue *) (*se))->getOperands ();
				// try alias..
				std::map <std::string, std::string>::iterator alias = filterAliases.find (ops);
				if (alias != filterAliases.end ())
					ops = alias->second;
				if (std::find (iter->second.begin (), iter->second.end (), ops) == iter->second.end ())
				{
					logStream (MESSAGE_WARNING) << "target " << newTar->getTargetName () << " (" << newTar->getTargetID () << ") rejected, as filter " << ops << " is not present among available filters" << sendLog;
					delete newTar;
					return;
				}
			}
		}
	}
	// add to possible targets..
	possibleTargets.push_back (new TargetEntry (newTar));
}

// enable targets which become observable
void Selector::checkTargetObservability ()
{
	EXEC SQL
		UPDATE
			targets
		SET
			tar_next_observable = NULL
		WHERE
			tar_next_observable < now ();
	EXEC SQL COMMIT;
}

// drop old priorities..
void Selector::checkTargetBonus ()
{
	EXEC SQL
		UPDATE
			targets
		SET
			tar_bonus = 0,
			tar_bonus_time = NULL
		WHERE
			tar_bonus_time < now ()
		AND tar_bonus_time is not NULL;
	EXEC SQL COMMIT;
}

void Selector::findNewTargets ()
{
	EXEC SQL BEGIN DECLARE SECTION;
	int consider_tar_id;
	char consider_type_id;
	EXEC SQL END DECLARE SECTION;

	double JD;
	int ret;

	JD = ln_get_julian_from_sys ();

	checkTargetObservability ();
	checkTargetBonus ();

	// drop targets which gets below horizon..
	for (std::vector < TargetEntry * >::iterator target_list = possibleTargets.begin (); target_list != possibleTargets.end ();)
	{
		rts2db::Target *tar = (*target_list)->target;
		ret = tar->considerForObserving (JD);
		if (ret)
		{
			// don't observe us - we are below horizont etc..
			logStream (MESSAGE_DEBUG) << "remove target " << tar->getTargetName () << " # " << tar->getTargetID () << " from possible targets" << sendLog;
			delete *target_list;
			target_list = possibleTargets.erase (target_list);
		}
		else
		{
			target_list++;
		}
	}

	EXEC SQL DECLARE findnewtargets CURSOR WITH HOLD FOR
		SELECT
			tar_id,
			type_id
		FROM
			targets
		WHERE
			(tar_enabled = true)
		AND (tar_priority + tar_bonus >= 0)
		AND ((tar_next_observable is null) OR (tar_next_observable < now ()));

	EXEC SQL OPEN findnewtargets;
	if (sqlca.sqlcode)
	{
		throw rts2db::SqlError ("cannot find new targets");
	}
	while (1)
	{
		EXEC SQL FETCH next
		FROM findnewtargets
		INTO :consider_tar_id, :consider_type_id;
		if (sqlca.sqlcode)
			break;
		// do not consider FLAT and other master targets!
		if (consider_tar_id == TARGET_FLAT)
			continue;
		// do not consider targets listed in nightDisabledTypes
		if (isInNightDisabledTypes (consider_type_id))
			continue;
		// try to find us in considered targets..
		considerTarget (consider_tar_id, JD);
	}
	if (sqlca.sqlcode != ECPG_NOT_FOUND)
	{
		throw rts2db::SqlError ("cannot find any new targets");
	}
	EXEC SQL CLOSE findnewtargets;
};

int Selector::selectNextNight (int in_bonusLimit, bool verbose, double length)
{
	// search for new observation targets..
	findNewTargets ();
	// create structure which will hold bonus for targets..

	std::vector < TargetEntry *>::iterator target_list;

	// sort targets by bonus..first calcaulate them..
	for (target_list = possibleTargets.begin (); target_list != possibleTargets.end (); target_list++)
	{
		(*target_list)->updateBonus ();
	}
	
	// sort them..
	std::sort (possibleTargets.begin (), possibleTargets.end (), bonusSort ());

	// find highest that meets constraints..

	double JD = ln_get_julian_from_sys ();

	std::vector < TargetEntry *>::iterator tar_best = possibleTargets.end ();

	for (target_list = possibleTargets.begin (); target_list != possibleTargets.end (); target_list++)
	{
		rts2db::Target *tar = (*target_list)->target;
		if (cameraList && !isnan (length))
		{
			if (rts2script::getMaximalScriptDuration (tar, *cameraList) > length)
			{
				logStream (MESSAGE_DEBUG) << "script for target " << tar->getTargetName () << " (# " << tar->getTargetID () << ") is longer than " << length << " seconds, ignoring the target" << sendLog;
				continue;
			}

		}
		if (tar->checkConstraints (JD))
		{
			if (!verbose)
			{
				tar_best = target_list;
				break;
			}
			if (tar_best == possibleTargets.end ())
			{
			  	logStream (MESSAGE_DEBUG) << "best target " << tar->getTargetName () << " #" << tar->getTargetID () << " with bonus " << tar->getTargetBonus () << sendLog;
				tar_best = target_list;
			}
			else
			{
				logStream (MESSAGE_DEBUG) << "target " << tar->getTargetName () << " #" << tar->getTargetID () << " bonus " << tar->getTargetBonus () << sendLog;
			}
		}
		else if (verbose)
		{
			logStream (MESSAGE_DEBUG) << "target " << tar->getTargetName () << " #" << tar->getTargetID () << " violates constraints - ignoring it" << sendLog;
		}
	}

	if (tar_best == possibleTargets.end () || (*tar_best)->bonus < in_bonusLimit)
	{
		logStream (MESSAGE_INFO) << "no target selected, letting system to decide what to do" << sendLog;
		return -1;
	}
	return (*tar_best)->target->getTargetID ();
}

int Selector::selectFlats ()
{
	return TARGET_FLAT;
}

int Selector::selectDarks ()
{
	return TARGET_DARK;
}

std::string Selector::getNightDisabledTypes ()
{
	std::string ret;
	for (std::vector <char>::iterator iter = nightDisabledTypes.begin (); iter != nightDisabledTypes.end (); iter++)
	{
		ret += *iter;
		ret += " ";
	}
	return ret;
}

int Selector::setNightDisabledTypes (const char *types)
{
	nightDisabledTypes = Str2CharVector (types);
	return 0;
}

void Selector::readFilters (std::string camera, std::string fn)
{
	std::ifstream ifs (fn.c_str ());
	while (!ifs.fail ())
	{
		std::string fil;
		ifs >> fil;
		if (ifs.fail ())
			break;
		availableFilters[camera].push_back (fil);
	}
	if (availableFilters[camera].size () == 0)
	{
		throw rts2core::Error (std::string ("empty filter file ") + fn + " for camera " + camera);
	}
}

void Selector::readAliasFile (const char *aliasFile)
{
	std::ifstream as;
	as.open (aliasFile);
	std::string f, a;
	while (!as.fail ())
	{
		as >> f;
		if (as.fail ())
			break;
		as >> a;
		if (as.fail ())
			throw rts2core::Error ("invalid filter alias file");
		filterAliases[f] = a;
	}
	as.close ();
}

void Selector::parseFilterOption (const char *in_optarg)
{
	std::vector <std::string> cam_filters;
	std::vector < std::string > filters;
	cam_filters = SplitStr (std::string (in_optarg), std::string (" "));
	if (cam_filters.size () != 2)
	{
	  	throw rts2core::Error ("camera must be separated by space from filter names");
	}
	filters = SplitStr (cam_filters[1], std::string (":"));
	if (filters.size () == 0)
	{
		throw rts2core::Error (std::string ("empty filter names specified for camera ") + cam_filters[0]);
	}
	addFilters (cam_filters[0].c_str (), filters);
}

void Selector::parseFilterFileOption (const char *in_optarg)
{
	std::vector <std::string> cam_filters;

	cam_filters = SplitStr (std::string (in_optarg), std::string (":"));
	if (cam_filters.size () != 2)
	{
		throw rts2core::Error ("camera name and filter file must be separated with :");
	}
	readFilters(cam_filters[0], cam_filters[1]);
}

void Selector::disableTarget (int n)
{
	possibleTargets[n]->target->setTargetEnabled (false);
}

void Selector::saveTargets ()
{
	std::vector <TargetEntry *>::iterator iter = possibleTargets.begin ();  
	for (; iter != possibleTargets.end (); iter++)
	{
		(*iter)->target->save (false);
	}
}

void Selector::revalidateConstraints (int watch_id)
{
	std::vector <TargetEntry *>::iterator iter = possibleTargets.begin ();
	for (; iter != possibleTargets.end (); iter++)
	{
		(*iter)->target->revalidateConstraints (watch_id);
	}
}
