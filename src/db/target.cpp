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

#include "../utilsdb/rts2appdb.h"
#include "../utilsdb/targetset.h"
#include "../utils/rts2askchoice.h"
#include "../utils/rts2config.h"
#include "../utils/rts2format.h"

#include <iostream>

#define OP_NONE   0x0000
#define OP_ENABLE 0x0003
#define OP_DISABLE  0x0001
#define OP_MASK_EN  0x0003
#define OP_PRIORITY 0x0004
#define OP_BONUS  0x0008
#define OP_BONUS_TIME 0x0010
#define OP_NEXT_TIME    0x0020
#define OP_SCRIPT 0x0040
#define OP_NEXT_OBSER   0x0080

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
	private:
		int op;
		std::list < int >tar_ids;
		rts2db::TargetSet *target_set;

		float new_priority;
		float new_bonus;
		time_t new_bonus_time;
		time_t new_next_time;

		const char *camera;
		std::list < CamScript > new_scripts;

		int runInteractive ();

	protected:
		virtual int processOption (int in_opt);

		virtual int processArgs (const char *arg);
		virtual int init ();

		virtual int doProcessing ();
	public:
		Rts2TargetApp (int argc, char **argv);
		virtual ~ Rts2TargetApp (void);
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
	addOption ('d', NULL, 0,
		"disable given targets (they will not be picked up by selector");
	addOption ('p', NULL, 1, "set target (fixed) priority");
	addOption ('b', NULL, 1, "set target bonus to this value");
	addOption ('t', NULL, 1, "set target bonus time to this value");
	addOption ('n', NULL, 1, "set next observable time this value");
	addOption ('o', NULL, 0, "clear next observable time");
	addOption ('c', NULL, 1, "next script will be set for given camera");
	addOption ('s', NULL, 1, "set script for target and camera");
	addOption ('N', NULL, 0, "do not pretty print");
}


Rts2TargetApp::~Rts2TargetApp ()
{
	delete target_set;
}


int
Rts2TargetApp::processOption (int in_opt)
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
				return -1;
			new_scripts.push_back (CamScript (camera, optarg));
			camera = NULL;
			op |= OP_SCRIPT;
			break;
		case 'N':
			std::cout << pureNumbers;
			break;
		default:
			return Rts2AppDb::processOption (in_opt);
	}
	return 0;
}


int
Rts2TargetApp::processArgs (const char *arg)
{
	int new_tar_id;
	char *endptr;

	new_tar_id = strtol (arg, &endptr, 10);
	if (*endptr != '\0')
		return -1;

	tar_ids.push_back (new_tar_id);
	return 0;
}


int
Rts2TargetApp::init ()
{
	int ret;

	ret = Rts2AppDb::init ();
	if (ret)
		return ret;

	Rts2Config *config;
	config = Rts2Config::instance ();

	return 0;
}


int
Rts2TargetApp::runInteractive ()
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


int
Rts2TargetApp::doProcessing ()
{
	if (tar_ids.size () == 0)
	{
		std::cerr << "No target specified, exiting." << std::endl;
		return -1;
	}
	target_set = new rts2db::TargetSet ();
	target_set->load (tar_ids);
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
		for (std::list < CamScript >::iterator iter = new_scripts.begin ();
			iter != new_scripts.end (); iter++)
		{
			target_set->setTargetScript (iter->cameraName, iter->script);
		}
	}
	if (op & OP_NEXT_OBSER)
	{
		target_set->setNextObservable (NULL);
	}
	if (op == OP_NONE)
	{
		return runInteractive ();
	}

	return target_set->save (true);
}


int
main (int argc, char **argv)
{
	Rts2TargetApp app = Rts2TargetApp (argc, argv);
	return app.run ();
}
