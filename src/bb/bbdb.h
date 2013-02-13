/**
 * RTS2 BB Database API
 * Copyright (C) 2012 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_BBDB__
#define __RTS2_BBDB__

#include "utilsfunc.h"

#include <libnova/libnova.h>
#include <string>
#include <list>

#include <libsoup/soup.h>

namespace rts2bb
{

class Observatory
{
	public:
		Observatory (int id);
		void load ();

		struct ln_lnlat_posn * getPosition () { return &position; }
		double getAltitude () { return altitude; }
		const char * getURL () { return url.c_str (); }
		int getId () { return observatory_id; }

		/**
		 * Authorize soup request (fill in username and password).
		 */
		void auth (SoupAuth *auth);
	private:
		int observatory_id;
		struct ln_lnlat_posn position;
		double altitude;
		std::string url;
};

class Observatories:public std::list <Observatory>
{
	public:
		Observatories ():std::list <Observatory> () {}

		void load ();
};

class ObservatorySchedule
{
	public:
		ObservatorySchedule (int _schedule_id, int _observatory_id)
		{
			schedule_id = _schedule_id;
			observatory_id = _observatory_id;
			state = -1;
			created = NAN;
			last_update = NAN;
			from = NAN;
			to = NAN;
		}

		ObservatorySchedule (int _schedule_id, int _observatory_id, int _state, double _created, double _last_update, double _from, double _to)
		{
			schedule_id = _schedule_id;
			observatory_id = _observatory_id;
			state = _state;
			created = _created;
			last_update = _last_update;
			from = _from;
			to = _to;
		}

		void load ();

		void updateState (int state, double from=NAN, double to=NAN);

		void toJSON (std::ostream &os);

		int getState () { return state; }
		int getScheduleId () { return schedule_id; }
		int getObservatoryId () { return observatory_id; }

		double getFrom () { return from; }
		double getTo () { return to; }

	private:
		int schedule_id;
		int observatory_id;
		int state;
		double created;
		double last_update;
		double from;
		double to;
};

class BBSchedules:public std::list <ObservatorySchedule>
{
	public:
		BBSchedules (int _schedule_id) { schedule_id = _schedule_id; }
		void load ();

		int getTargetId () { return tar_id; }

	private:
		int schedule_id;
		int tar_id;
};

/***
 * Register new target mapping into BB database.
 *
 * @param observatory_id  ID of observatory requesting the change
 * @param tar_id          ID of target in central database
 * @param obs_tar_id      ID of target in observatory database
 */
void createMapping (int observatory_id, int tar_id, int obs_tar_id);

void reportObservation (int observatory_id, int obs_id, int obs_tar_id, double obs_ra, double obs_dec, double obs_slew, double obs_start, double obs_end, double onsky, int good_images, int bad_images);

/**
 * Find mapping for a given target.
 */
int findMapping (int observatory_id, int obs_tar_id);

int findObservatoryMapping (int observatory_id, int tar_id);

/**
 * Creates new schedule.
 */
int createSchedule (int target_id);

//* BB schedule request was created
#define BB_SCHEDULE_CREATED           0
//* Scheduling request for some reason failed
#define BB_SCHEDULE_FAILED            10
//* Observation is possible
#define BB_SCHEDULE_OBSERVABLE        11
//* Observation is possible, but it was (for now) not selected
#define BB_SCHEDULE_BACKUP            12
//* Observation was scheduled on this observatory 
#define BB_SCHEDULE_CONFIRMED         13

}

#endif   // __RTS2_BBDB__
