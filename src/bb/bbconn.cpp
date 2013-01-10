/*
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

#include "bb.h"
#include "bbconn.h"
#include "bbdb.h"
#include "rts2db/target.h"
#include "rts2db/sqlerror.h"
#include "configuration.h"

using namespace rts2bb;

ConnBBQueue::ConnBBQueue (rts2core::Block * _master, const char *_exec, ObservatorySchedule *_obs_sched):rts2script::ConnExe (_master, _exec, false)
{
	obs_sched = _obs_sched;
}

void ConnBBQueue::processCommand (char *cmd)
{
	// create mapping
	if (!strcasecmp (cmd, "mapping"))
	{
		int observatory_id;
		int tar_id;
		int obs_tar_id;

		if (paramNextInteger (&observatory_id) || paramNextInteger (&tar_id) || paramNextInteger (&obs_tar_id))
			return;
		createMapping (observatory_id, tar_id, obs_tar_id);
	}
	else if (!strcasecmp (cmd, "targetinfo"))
	{
		int tar_id;

		if (paramNextInteger (&tar_id))
			return;
	
		rts2db::Target *tar = createTarget (tar_id, rts2core::Configuration::instance ()->getObserver ());

		std::ostringstream os;

		ln_equ_posn pos;
		tar->getPosition (&pos);

		os << '"' << tar->getTargetName () << "\" " << pos.ra << " " << pos.dec;

		writeToProcess (os.str ().c_str ());
	}
	else if (!strcasecmp (cmd, "obsapiurl"))
	{
		int observatory_id;

		if (paramNextInteger (&observatory_id))
			return;


		Observatory obs (observatory_id);
		obs.load ();

		writeToProcess (obs.getURL ());
	}
	else if (!strcasecmp (cmd, "schedule_from"))
	{
		double from_time;
		if (paramNextDouble (&from_time))
		{
			from_time = NAN;
			return;
		}
		if (obs_sched)
			obs_sched->updateState (BB_SCHEDULE_REPLIED, from_time, NAN);
	}
	else
	{
		rts2script::ConnExe::processCommand (cmd);
	}
}

ConnBBQueue *rts2bb::scheduleTarget (int tar_id, int observatory_id, ObservatorySchedule *obs_sched)
{
	std::ostringstream p_os;
	p_os << rts2core::Configuration::instance ()->getStringDefault ("bb", "script_dir", RTS2_SHARE_PREFIX "/rts2/bb") << "/schedule_target.py";

	ConnBBQueue *bbqueue = new ConnBBQueue (((BB * ) getMasterApp ()), p_os.str ().c_str (), obs_sched);

	try
	{
		int obs_tar_id = findObservatoryMapping (observatory_id, tar_id);
		bbqueue->addArg ("--obs-tar-id");
		bbqueue->addArg (obs_tar_id);
	}
	catch (rts2db::SqlError er)
	{
		bbqueue->addArg ("--create");
		bbqueue->addArg (tar_id);
	}

	bbqueue->addArg (observatory_id);

	int ret = bbqueue->init ();
	if (ret)
		throw JSONException ("cannot execute schedule script");

	if (((BB *) getMasterApp ())->getDebugConn ())
		bbqueue->setConnectionDebug (true);

	((BB *) getMasterApp ())->addConnection (bbqueue);
	return bbqueue;
}
