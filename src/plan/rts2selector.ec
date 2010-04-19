/*
 * Selector body.
 * Copyright (C) 2003-2008 Petr Kubanek <petr@kubanek.net>
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

#include "rts2selector.h"
#include "../utils/rts2config.h"
#include "../utils/utilsfunc.h"

#include <libnova/libnova.h>

using namespace rts2plan;

Selector::Selector (struct ln_lnlat_posn * in_observer)
{
	int ret;
	Rts2Config *config;
	std::string doNotObserve;
	double val;

	observer = in_observer;

	config = Rts2Config::instance ();

	config->getString ("selector", "night_do_not_consider", doNotObserve);

	setNightDisabledTypes (doNotObserve.c_str ());

	flat_sun_min = 100;
	flat_sun_max = 100;
	ret = config->getDouble ("observatory", "flat_sun_min", flat_sun_min);
	if (ret)
		return;
	ret = config->getDouble ("observatory", "flat_sun_max", flat_sun_max);
	if (ret)
		return;

	if (flat_sun_min > flat_sun_max)
	{
		val = flat_sun_min;
		flat_sun_min = flat_sun_max;
		flat_sun_max = val;
	}
}

Selector::~Selector (void)
{
	for (std::list < Target * >::iterator target_list =
		possibleTargets.begin (); target_list != possibleTargets.end ();
		target_list++)
	{
		Target *tar = *target_list;
		delete tar;
	}
	possibleTargets.clear ();
}

int Selector::selectNext (int masterState)
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
			return selectNextNight ();
			break;
		case SERVERD_DAWN:
		case SERVERD_DUSK:
			// special case - select GRBs, which are new (=targets with priority higher then 1500)
			ret = selectNextNight (1500);
			if (ret != -1)
				return ret;
			// otherwise select darks/flats/whatever
			if (flat_sun_min >= flat_sun_max)
				return selectDarks ();
			JD = ln_get_julian_from_sys ();
			ln_get_solar_equ_coords (JD, &eq_sun);
			ln_get_hrz_from_equ (&eq_sun, observer, JD, &sun_hrz);
			if (sun_hrz.alt >= flat_sun_min && sun_hrz.alt <= flat_sun_max)
				return selectFlats ();
			// don't break..
		case SERVERD_DAWN | SERVERD_STANDBY:
		case SERVERD_DUSK | SERVERD_STANDBY:
			return selectDarks ();
			break;
	}
	return -1;					 // we don't have any target to take observation..
}

void Selector::considerTarget (int consider_tar_id, double JD)
{
	std::list < Target * >::iterator target_list;
	Target *newTar;
	int ret;

	for (target_list = possibleTargets.begin ();
		target_list != possibleTargets.end (); target_list++)
	{
		Target *tar = *target_list;
		if (tar->getTargetID () == consider_tar_id)
		{
			return;
		}
	}
	// add us..
	newTar = createTarget (consider_tar_id, observer);
	if (!newTar)
		return;
	ret = newTar->considerForObserving (JD);
	#ifdef DEBUG_EXTRA
	logStream (MESSAGE_DEBUG) << "considerForObserving tar_id: " << newTar->
		getTargetID () << " ret: " << ret << sendLog;
	#endif
	if (ret)
	{
		delete newTar;
		return;
	}
	// add to possible targets..
	possibleTargets.push_back (newTar);
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

	// drop targets which gets bellow horizont..

	for (std::list < Target * >::iterator target_list =
		possibleTargets.begin (); target_list != possibleTargets.end ();)
	{
		Target *tar = *target_list;
		ret = tar->considerForObserving (JD);
		if (ret)
		{
			// don't observe us - we are bellow horizont etc..
			target_list = possibleTargets.erase (target_list);
			logStream (MESSAGE_DEBUG) << "remove target tar_id " << tar->
				getTargetID () << " from possible targets" << sendLog;
			delete tar;
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
		AND (tar_priority + tar_bonus > 0)
		AND ((tar_next_observable is null) OR (tar_next_observable < now ()));
	if (sqlca.sqlcode)
	{
		logStream (MESSAGE_ERROR) << "findNewTargets: " << sqlca.sqlerrm.
			sqlerrmc << sendLog;
		return;
	}

	EXEC SQL OPEN findnewtargets;
	if (sqlca.sqlcode)
	{
		logStream (MESSAGE_ERROR) << "findNewTargets: " << sqlca.sqlerrm.
			sqlerrmc << sendLog;
		EXEC SQL ROLLBACK;
		return;
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
		// some DB error..strange, let's get out
		logStream (MESSAGE_DEBUG) << "findNewTargets DB error: " << sqlca.
			sqlerrm.sqlerrmc << sendLog;
		exit (1);
	}
	EXEC SQL CLOSE findnewtargets;
};

int Selector::selectNextNight (int in_bonusLimit)
{
	// search for new observation targets..
	int maxId;
	float maxBonus = -1;
	float tar_bonus;
	findNewTargets ();
	// find target with highest bonus and move us to it..
	for (std::list < Target * >::iterator target_list =
		possibleTargets.begin (); target_list != possibleTargets.end ();
		target_list++)
	{
		Target *tar = *target_list;
		tar_bonus = tar->getBonus ();
		#ifdef DEBUG_EXTRA
		logStream (MESSAGE_DEBUG) << "target: " << tar->
			getTargetID () << " bonus: " << tar_bonus << sendLog;
		#endif
		if (tar_bonus > maxBonus)
		{
			maxId = tar->getTargetID ();
			maxBonus = tar_bonus;
		}
	}
	if (maxBonus < in_bonusLimit)
	{
		// we don't get any target..so take some darks..
		if (in_bonusLimit == 0)
			return selectDarks ();
		else
			// indicate error
			return -1;
	}
	return maxId;
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
