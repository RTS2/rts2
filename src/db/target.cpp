/* 
 * Target editing application.
 * Copyright (C) 2006-2008 Petr Kubanek <petr@kubanek.net>
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

#include "../plan/script.h"
#include "../utilsdb/observation.h"
#include "../utilsdb/rts2appdb.h"
#include "../utilsdb/target.h"
#include "../utilsdb/targetset.h"
#include "../utils/rts2askchoice.h"
#include "../utils/rts2config.h"
#include "../utils/rts2format.h"

#include <iostream>

#define OP_NONE             0x0000
#define OP_ENABLE           0x0003
#define OP_DISABLE          0x0001
#define OP_MASK_EN          0x0003
#define OP_PRIORITY         0x0004
#define OP_BONUS            0x0008
#define OP_BONUS_TIME       0x0010
#define OP_NEXT_TIME        0x0020
#define OP_SCRIPT           0x0040
#define OP_NEXT_OBSER       0x0080

#define OP_OBS_START        0x0100
#define OP_OBS_SLEW         0x0200
#define OP_OBS_END          0x0400

#define OPT_OBSERVE_START   OPT_LOCAL + 831
#define OPT_OBSERVE_SLEW    OPT_LOCAL + 832
#define OPT_OBSERVE_END     OPT_LOCAL + 833

class CamScript
{
	public:
		const char *cameraName;
		const char *script;

		CamScript (const char *in_cameraName, const char *in_script)
		{
			cameraName = in_cameraName;
			script = in_script;
		}
};

class Rts2TargetApp:public Rts2AppDb
{
	public:
		Rts2TargetApp (int argc, char **argv);
		virtual ~ Rts2TargetApp (void);
	protected:
		virtual int processOption (int in_opt);

		virtual int processArgs (const char *arg);
		virtual int init ();

		virtual int doProcessing ();
	private:
		int op;
		std::vector < const char * >tar_names;
		rts2db::TargetSet *target_set;

		float new_priority;
		float new_bonus;
		time_t new_bonus_time;
		time_t new_next_time;

		const char *camera;
		std::list < CamScript > new_scripts;

		int runInteractive ();
};

Rts2TargetApp::Rts2TargetApp (int in_argc, char **in_argv):
Rts2AppDb (in_argc, in_argv)
{
	target_set = NULL;
	op = OP_NONE;
	new_priority = nan ("f");
	new_bonus = nan ("f");

	camera = NULL;

	addOption ('e', NULL, 0, "enable given targets");
	addOption ('d', NULL, 0, "disable given targets (they will not be picked up by selector");
	addOption ('p', NULL, 1, "set target (fixed) priority");
	addOption ('b', NULL, 1, "set target bonus to this value");
	addOption ('t', NULL, 1, "set target bonus time to this value");
	addOption ('n', NULL, 1, "set next observable time this value");
	addOption ('o', NULL, 0, "clear next observable time");
	addOption ('c', NULL, 1, "next script will be set for given camera");
	addOption ('s', NULL, 1, "set script for target and camera");
	addOption ('N', NULL, 0, "do not pretty print");

	addOption (OPT_OBSERVE_SLEW, "slew", 0, "mark telescope slewing to observation. Return observation ID");
	addOption (OPT_OBSERVE_START, "observe", 0, "mark start of observation (after telescope slew in). Requires observation ID");
	addOption (OPT_OBSERVE_END, "end", 0, "mark end of observation. Requires observation ID");
}

Rts2TargetApp::~Rts2TargetApp ()
{
	delete target_set;
}

int Rts2TargetApp::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'e':
			if (op & OP_DISABLE)
				return -1;
			op |= OP_ENABLE;
			break;
		case 'd':
			if (op & OP_ENABLE)
				return -1;
			op |= OP_DISABLE;
			break;
		case 'p':
			new_priority = atof (optarg);
			op |= OP_PRIORITY;
			break;
		case 'b':
			new_bonus = atof (optarg);
			op |= OP_BONUS;
			break;
		case 't':
			op |= OP_BONUS_TIME;
			return parseDate (optarg, &new_bonus_time);
		case 'n':
			op |= OP_NEXT_TIME;
			return parseDate (optarg, &new_next_time);
		case 'o':
			op |= OP_NEXT_OBSER;
			break;
		case 'c':
			if (camera)
				return -1;
			camera = optarg;
			break;
		case 's':
			if (!camera)
			{
				std::cerr << "Please provide camera name (with -c parameter) before specifing script!" << std::endl;
				return -1;
			}
			// try to parse it..
			new_scripts.push_back (CamScript (camera, optarg));
			camera = NULL;
			op |= OP_SCRIPT;
			break;
		case 'N':
			std::cout << pureNumbers;
			break;
		case OPT_OBSERVE_SLEW:
			op |= OP_OBS_SLEW;
			break;
		case OPT_OBSERVE_START:
			op |= OP_OBS_START;
			break;
		case OPT_OBSERVE_END:
			op |= OP_OBS_END;
			break;
		default:
			return Rts2AppDb::processOption (in_opt);
	}
	return 0;
}

int Rts2TargetApp::processArgs (const char *arg)
{
	tar_names.push_back (arg);
	return 0;
}

int Rts2TargetApp::init ()
{
	int ret;

	ret = Rts2AppDb::init ();
	if (ret)
		return ret;

	Rts2Config *config;
	config = Rts2Config::instance ();

	return 0;
}

int Rts2TargetApp::runInteractive ()
{
	Rts2AskChoice selection = Rts2AskChoice (this);
	selection.addChoice ('e', "Enable target(s)");
	selection.addChoice ('d', "Disable target(s)");
	selection.addChoice ('o', "List observations around position");
	selection.addChoice ('t', "List targets around position");
	selection.addChoice ('n', "Choose new target");
	selection.addChoice ('s', "Save");
	selection.addChoice ('q', "Quit");
	while (1)
	{
		char sel_ret;
		sel_ret = selection.query (std::cout);
		switch (sel_ret)
		{
			case 'e':

				break;
			case 'd':

				break;
			case 'o':

				break;
			case 't':

				break;
			case 'n':

				break;
			case 'q':
				return 0;
			case 's':
				return target_set->save (true);
		}
	}
}

int Rts2TargetApp::doProcessing ()
{
	if ((op & OP_OBS_START) || (op & OP_OBS_END))
	{
		if (tar_names.size () != 1)
		{
			std::cerr << "You must specify only a single observation ID." << std::endl;
			return -1;
		}
		char *endp;
		long obs_id = strtol (tar_names[0], &endp, 10);
		if (*endp != '\0')
		{
			std::cerr << "You must specify observation ID - this must be number." << std::endl;
			return -1;
		}
		rts2db::Observation obs (obs_id);
		if (op & OP_OBS_START)
			obs.startObservation ();
		if (op & OP_OBS_END)
		  	obs.endObservation (OBS_BIT_MOVED | OBS_BIT_STARTED | OBS_BIT_PROCESSED);
		return 0;
	}
	if (tar_names.size () == 0)
	{
		std::cerr << "No target specified, exiting." << std::endl;
		return -1;
	}
	target_set = new rts2db::TargetSet ();
	target_set->load (tar_names);
	if ((op & OP_MASK_EN) == OP_ENABLE)
	{
		target_set->setTargetEnabled (true, true);
	}
	if ((op & OP_MASK_EN) == OP_DISABLE)
	{
		target_set->setTargetEnabled (false, true);
	}
	if (op & OP_PRIORITY)
	{
		target_set->setTargetPriority (new_priority);
	}
	if (op & OP_BONUS)
	{
		target_set->setTargetBonus (new_bonus);
	}
	if (op & OP_BONUS_TIME)
	{
		target_set->setTargetBonusTime (&new_bonus_time);
	}
	if (op & OP_NEXT_TIME)
	{
		target_set->setNextObservable (&new_next_time);
	}
	if (op & OP_SCRIPT)
	{
		for (std::list < CamScript >::iterator iter = new_scripts.begin (); iter != new_scripts.end (); iter++)
		{
			rts2script::Script script = rts2script::Script (iter->script);
			struct ln_equ_posn target_pos;
			target_set->begin ()->second->getPosition (&target_pos);
			script.parseScript (((Rts2Target *) target_set->begin ()->second), &target_pos);
			int failedCount = script.getFaultLocation ();
			if (failedCount != -1)
			{
				std::cerr << "PARSING of script '" << iter->script << "' FAILED!!! AT " << failedCount << std::endl
					<< std::string (iter->script).substr (0, failedCount + 1) << std::endl;
				for (; failedCount > 0; failedCount--)
					std::cerr << ' ';
				std::cerr << "^ here" << std::endl;
			}
			try
			{
				target_set->setTargetScript (iter->cameraName, iter->script);
			}
			catch (rts2db::CameraMissingExcetion &ex)
			{
				std::cerr << "Missing camera " << iter->cameraName << ". Is it filled in \"cameras\" database table?" << std::endl;
			}
		}
	}
	if (op & OP_NEXT_OBSER)
	{
		target_set->setNextObservable (NULL);
	}
	if (op & OP_OBS_SLEW)
	{
		if (target_set->size () != 1)
		{
			std::cerr << "You must specify only single target which observation will be started." << std::endl;
			return -1;
		}
		Target *tar = (target_set->begin ())->second;
		struct ln_equ_posn pos;
		tar->getPosition (&pos);
		tar->startSlew (&pos);
		std::cout << tar->getObsId () << std::endl;
		tar->setObsId (-1);
		return 0;
	}
	if (op == OP_NONE)
	{
		return runInteractive ();
	}

	return target_set->save (true);
}

int main (int argc, char **argv)
{
	Rts2TargetApp app = Rts2TargetApp (argc, argv);
	return app.run ();
}
