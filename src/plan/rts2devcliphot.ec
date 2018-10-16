/**
 * Executor client for photometer.
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

#include "rts2devcliphot.h"

EXEC SQL include sqlca;

using namespace rts2script;

DevClientPhotExec::DevClientPhotExec (rts2core::Connection * in_connection):DevClientPhot (in_connection),DevScript (in_connection)
{
	minFlux = 20;
}


DevClientPhotExec::~DevClientPhotExec ()
{
	deleteScript ();
}

void DevClientPhotExec::integrationStart ()
{
	if (currentTarget)
		currentTarget->startObservation ();
	DevClientPhot::integrationStart ();
}

void DevClientPhotExec::integrationEnd ()
{
	nextCommand ();
	DevClientPhot::integrationEnd ();
}

void DevClientPhotExec::integrationFailed (int status)
{
	nextCommand ();
	DevClientPhot::integrationFailed (status);
}

void DevClientPhotExec::addCount (int count, float exp, bool is_ov)
{
	EXEC SQL BEGIN DECLARE SECTION;
	int d_obs_id;
	int d_count_value;
	double d_count_date;
	int d_count_usec;
	float d_count_exposure;
	char d_count_filter;
	double d_count_ra;
	double d_count_dec;
	VARCHAR d_counter_name[8];
	EXEC SQL END DECLARE SECTION;
	if ((connection->getState () & PHOT_MASK_INTEGRATE) != PHOT_INTEGRATE)
	{
		return;
	}

	if (!currentTarget)
		return;

	struct ln_equ_posn actRaDec;
	struct timeval now;

	if (is_ov)
	{
		logStream (MESSAGE_DEBUG) << "DevClientPhotExec::addCount is_ov" << sendLog;
		return;
	}

	gettimeofday (&now, NULL);

	actRaDec.ra = -1000;
	actRaDec.dec = -1000;

	getMaster ()->postEvent (new rts2core::Event (EVENT_GET_RADEC, (void *) &actRaDec));

	d_obs_id = currentTarget->getObsId ();
	d_count_date = now.tv_sec + now.tv_usec / USEC_SEC;
	d_count_usec = now.tv_usec;
	d_count_value = count;
	d_count_exposure = exp;
	d_count_filter = getConnection ()->getValueInteger ("filter") + '0';
	d_count_ra = actRaDec.ra;
	d_count_dec = actRaDec.dec;

	strncpy (d_counter_name.arr, connection->getName (), 8);
	d_counter_name.len = strlen (connection->getName ());
	if (d_counter_name.len > 8)
		d_counter_name.len = 8;

	EXEC SQL INSERT INTO
		counts
	(
		obs_id,
		count_date,
		count_usec,
		count_value,
		count_exposure,
		count_filter,
		count_ra,
		count_dec,
		counter_name
	)
	VALUES
	(
		:d_obs_id,
		to_timestamp (:d_count_date),
		:d_count_usec,
		:d_count_value,
		:d_count_exposure,
		:d_count_filter,
		:d_count_ra,
		:d_count_dec,
		:d_counter_name
	);

	if (sqlca.sqlcode != 0)
	{
		logStream (MESSAGE_ERROR) << "DevClientPhotExec::addCount db error " << sqlca.sqlerrm.sqlerrmc << sendLog;
		EXEC SQL ROLLBACK;
	}
	else
	{
		EXEC SQL COMMIT;
	}
}

int DevClientPhotExec::getNextCommand ()
{
	return getScript()->nextCommand (*this, &nextComd, cmd_device);
}

void DevClientPhotExec::postEvent (rts2core::Event * event)
{
	DevScript::postEvent (new rts2core::Event (event));
	DevClientPhot::postEvent (event);
}

void DevClientPhotExec::nextCommand (rts2core::Command *triggerCommand)
{
	if (scriptKillCommand && triggerCommand == scriptKillCommand)
	{
		if (currentTarget == NULL && killTarget != NULL)
			DevScript::postEvent (new rts2core::Event (scriptKillcallScriptEnds ? EVENT_SET_TARGET : EVENT_SET_TARGET_NOT_CLEAR, killTarget));
		killTarget = NULL;
		scriptKillCommand = NULL;
		scriptKillcallScriptEnds = false;
	}
	int ret = haveNextCommand (this);
	if (!ret)
		return;

	if (currentTarget && !currentTarget->wasMoved ())
		return;

	connection->queCommand (nextComd);
	nextComd = NULL;               // after command execute, it will be deleted
}

void DevClientPhotExec::filterMoveEnd ()
{
	if ((connection->getState () & PHOT_MASK_INTEGRATE) != PHOT_INTEGRATE)
		nextCommand ();
}

void DevClientPhotExec::filterMoveFailed (__attribute__ ((unused)) int status)
{
	deleteScript ();
}
