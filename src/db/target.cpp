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

#include "rts2script/script.h"
#include "rts2script/elementtarget.h"
#include "rts2db/constraints.h"
#include "rts2db/observation.h"
#include "rts2db/appdb.h"
#include "rts2db/sqlerror.h"
#include "rts2db/target.h"
#include "rts2db/targetset.h"
#include "askchoice.h"
#include "configuration.h"
#include "rts2format.h"

#include <iostream>

#define OP_NONE                   0x000000
#define OP_ENABLE                 0x000003
#define OP_DISABLE                0x000001
#define OP_MASK_EN                0x000003
#define OP_PRIORITY               0x000004
#define OP_BONUS                  0x000008
#define OP_BONUS_TIME             0x000010
#define OP_NEXT_TIME              0x000020
#define OP_SCRIPT                 0x000040
#define OP_NEXT_OBSER             0x000080

#define OP_OBS_START              0x000100
#define OP_OBS_SLEW               0x000200
#define OP_OBS_END                0x000400
#define OP_TEMPDISABLE            0x000800
#define OP_CONSTRAINTS            0x001000

#define OP_PI_NAME                0x002000
#define OP_PROGRAM_NAME           0x004000
#define OP_DELETE                 0x008000

#define OP_CONS_RESET             0x010000
#define OP_SET_PROPER_MOTION      0x020000

#define OPT_OBSERVE_START         OPT_LOCAL + 831
#define OPT_OBSERVE_SLEW          OPT_LOCAL + 832
#define OPT_OBSERVE_END           OPT_LOCAL + 833
#define OPT_TEMPDISABLE           OPT_LOCAL + 834
#define OPT_AIRMASS               OPT_LOCAL + 835
#define OPT_LUNAR_DISTANCE        OPT_LOCAL + 836
#define OPT_LUNAR_ALTITUDE        OPT_LOCAL + 837
#define OPT_MAXREPEATS            OPT_LOCAL + 838
#define OPT_PI_NAME               OPT_LOCAL + 839
#define OPT_PROGRAM_NAME          OPT_LOCAL + 840
#define OPT_SET_PROPER_MOTION     OPT_LOCAL + 841
#define OPT_DELETE                OPT_LOCAL + 842

#define OPT_ID_ONLY               OPT_LOCAL + 843
#define OPT_NAME_ONLY             OPT_LOCAL + 844

#define OPT_RESETC                OPT_LOCAL + 845

#define OPT_FORCE                 OPT_LOCAL + 846

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

/**
 * Class for target application functions.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class TargetApp:public rts2db::AppDb
{
	public:
		TargetApp (int argc, char **argv);
		virtual ~ TargetApp (void);
	protected:
		virtual void usage ();

		virtual int processOption (int in_opt);

		virtual int processArgs (const char *arg);
		virtual int init ();

		virtual int doProcessing ();
	private:
		int32_t op;
		std::vector < const char * >tar_names;
		rts2db::TargetSet target_set;

		float new_priority;
		float new_bonus;
		time_t new_bonus_time;
		time_t new_next_time;

		const char *camera;
		std::list < CamScript > new_scripts;

		int runInteractive ();

		const char *tempdis;

		void setTempdisable ();

		bool matchAll;

		void parseConstraint (const char *name, const char *arg) { constraints.parse (name, arg); }

		// constraints
		rts2db::Constraints constraints;

		char *defaultCamera;

		Configuration *config;

		const char *pi;
		const char *program;

		struct ln_equ_posn pm;

		rts2db::resolverType resType;

		bool confirmOp;
};

TargetApp::TargetApp (int in_argc, char **in_argv):rts2db::AppDb (in_argc, in_argv)
{
	op = OP_NONE;
	new_priority = NAN;
	new_bonus = NAN;

	camera = NULL;

	tempdis = NULL;

	matchAll = false;

	defaultCamera = NULL;
	config = NULL;

	pm.ra = pm.dec = NAN;

	resType = rts2db::NAME_ID;

	confirmOp = true;

	addOption (OPT_FORCE, "force", 0, "do not ask before performing change (deletion, ..)");

	addOption ('a', NULL, 0, "select all matching target (if search by name gives multiple targets)");
	addOption ('e', NULL, 0, "enable given target(s)");
	addOption ('d', NULL, 0, "disable given target(s) (they will not be picked up by selector)");
	addOption ('p', NULL, 1, "set target(s) (fixed) priority");
	addOption ('b', NULL, 1, "set target(s) bonus to this value");
	addOption ('t', NULL, 1, "set target(s) bonus time to this value");
	addOption ('n', NULL, 1, "set target(s) next observable time this value");
	addOption ('o', NULL, 0, "clear target(s) next observable time");
	addOption ('c', NULL, 1, "next script will be set for the given camera");
	addOption ('s', NULL, 1, "set script for target(s) and camera, specified with -c");

	addOption (OPT_PI_NAME, "pi", 1, "set PI name");
	addOption (OPT_PROGRAM_NAME, "program", 1, "set program name");
	addOption (OPT_SET_PROPER_MOTION, "proper-motion", 1, "set target proper motion");

	addOption ('N', NULL, 0, "do not pretty print");

	addOption (OPT_OBSERVE_SLEW, "slew", 0, "mark telescope slewing to observation. Return observation ID");
	addOption (OPT_OBSERVE_START, "observe", 0, "mark start of observation (after telescope slew in). Requires observation ID");
	addOption (OPT_OBSERVE_END, "end", 0, "mark end of observation. Requires observation ID");

	addOption (OPT_TEMPDISABLE, "tempdisable", 1, "change time for which target will be disabled after script execution");

	addOption (OPT_AIRMASS, "airmass", 1, "set airmass constraint for the target");
	addOption (OPT_LUNAR_DISTANCE, "lunarDistance", 1, "set lunar distance constraint for the target");
	addOption (OPT_LUNAR_ALTITUDE, "lunarAltitude", 1, "set lunar altitude (height above horizon) constraint for the target");
	addOption (OPT_MAXREPEATS, "maxRepeats", 1, "set maximal number of target repeats");

	addOption (OPT_DELETE, "delete-targets", 0, "delete targets and associated entries (observations, images) from the database");

	addOption (OPT_ID_ONLY, "id-only", 0, "expect numeric target(s) names (IDs only)");
	addOption (OPT_NAME_ONLY, "name-only", 0, "resolver target(s) as names (even pure numbers)");
	addOption (OPT_RESETC, "reset-constraints", 0, "reset target constraints (do not add new constraints)");
}

TargetApp::~TargetApp ()
{
	delete[] defaultCamera;
}

void TargetApp::usage ()
{
	std::cout << "Unless you use option to process arguments only as target IDs, arguments are first resolved as target names. \"%\" character can be used as wildcard - please see bellow for examples" << std::endl
		<< "If you would like to remove constraints, just pass empy or invalid numbers as arguments - e.g. for intervals, specify only :, for maxRepeats, pass -1 as argument to --maxRepeats" << std::endl

		<< "Set next observable time for target 192 to 1 hour (3600 seconds) from now" << std::endl
		<< "  " << getAppName () << " -n +3600 192         .. " << std::endl
		<< "Delete targets with name starting with GRB" << std::endl
		<< "  " << getAppName () << " --delete-targets GRB%" << std::endl
		<< "Delete targets with name starting with NGC, do not ask for single target confirmation" << std::endl
		<< "  " << getAppName () << " --delete-targets --force NGC%" << std::endl
		<< "Disable target 192 for 1 hour after it is executed:" << std::endl
		<< "  " << getAppName () << " --tempdisable 3600 192 .. " << std::endl
		<< "Disable target 196 for 2 days and 3 hours after it is executed:" << std::endl
		<< "  " << getAppName () << " --tempdisable 2d3h 196" << std::endl
		<< "Disable all targets with name starting with M" << std::endl
		<< "  " << getAppName () << " -d M%" << std::endl
		<< "Set M31 airmass limit to < 1.5 and lunarDistance to > 40:" << std::endl
		<< "  " << getAppName () << " --lunarDistance 40: --airmass :1.5 M31" << std::endl
		<< "Remove airmass limit for target M31" << std::endl
		<< "  " << getAppName () << " --airmass : M31" << std::endl;
}

int TargetApp::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'a':
			matchAll = true;
			break;
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
			if (!camera && !(camera = defaultCamera))
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
		case OPT_TEMPDISABLE:
			tempdis = optarg;
			op |= OP_TEMPDISABLE;
			break;
		case OPT_AIRMASS:
			parseConstraint (CONSTRAINT_AIRMASS, optarg);
			op |= OP_CONSTRAINTS;
			break;
		case OPT_LUNAR_DISTANCE:
			parseConstraint (CONSTRAINT_LDISTANCE, optarg);
			op |= OP_CONSTRAINTS;
			break;
		case OPT_LUNAR_ALTITUDE:
			parseConstraint (CONSTRAINT_LALTITUDE, optarg);
			op |= OP_CONSTRAINTS;
			break;
		case OPT_MAXREPEATS:
			parseConstraint (CONSTRAINT_MAXREPEATS, optarg);
			op |= OP_CONSTRAINTS;
			break;
		case OPT_PI_NAME:
			pi = optarg;
			op |= OP_PI_NAME;
			break;
		case OPT_PROGRAM_NAME:
			program = optarg;
			op |= OP_PROGRAM_NAME;
			break;
		case OPT_SET_PROPER_MOTION:
			{
				LibnovaRaDec raDec;
				if (raDec.parseString (optarg))
				{
					std::cerr << "cannot parse " << optarg << " as RA DEC string" << std::endl;
					return -1;
				}
				raDec.getPos (&pm);
				op |= OP_SET_PROPER_MOTION;
			}
			break;
		case OPT_DELETE:
			op |= OP_DELETE;
			break;
		case OPT_ID_ONLY:
			resType = rts2db::ID_ONLY;
			break;
		case OPT_NAME_ONLY:
			resType = rts2db::NAME_ONLY;
			break;
		case OPT_RESETC:
			op |= OP_CONS_RESET;
			break;
		case OPT_FORCE:
			confirmOp = false;
			break;
		default:
			return AppDb::processOption (in_opt);
	}
	return 0;
}

int TargetApp::processArgs (const char *arg)
{
	tar_names.push_back (arg);
	return 0;
}

int TargetApp::init ()
{
	int ret;

	ret = AppDb::init ();
	if (ret)
		return ret;

	config = Configuration::instance ();

	std::string cam;
	if (config->getString ("observatory", "default_camera", cam) == 0)
	{
		defaultCamera = new char[cam.length () + 1];
		strcpy (defaultCamera, cam.c_str ());
	}

	return 0;
}

int TargetApp::runInteractive ()
{
	rts2core::AskChoice selection = rts2core::AskChoice (this);
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
				return target_set.save (true);
		}
	}
}

void TargetApp::setTempdisable ()
{
	if (camera == NULL && !(camera = defaultCamera))
	{
		std::cerr << "Missing camera name" << std::endl;
		return;
	}
	for (rts2db::TargetSet::iterator iter = target_set.begin (); iter != target_set.end (); iter++)
	{
		std::string cs;
		rts2db::Target *tar = iter->second;
		tar->getScript (camera, cs);
		rts2script::Script script = rts2script::Script (cs.c_str ());
		script.parseScript (tar);
		int failedCount = script.getFaultLocation ();
		if (failedCount != -1)
		{
			std::cerr << "PARSING of script '" << cs << "' FAILED!!! AT " << failedCount << std::endl
				<< cs.substr (0, failedCount + 1) << std::endl;
			for (; failedCount > 0; failedCount--)
				std::cerr << ' ';
			std::cerr << "^ here" << std::endl;
		}
		try
		{
			rts2script::Script::iterator se = script.findElement (COMMAND_TAR_TEMP_DISAB, script.begin ());
			if (tempdis != NULL)
			{
				if (se == script.end ())
				{
					script.push_front (new rts2script::ElementTempDisable (&script, tar, tempdis));
				}
				else
				{
					((rts2script::ElementTempDisable *) *se)->setTempDisable (tempdis);
				}
			}
			else if (se != script.end ())
			{
				script.erase (se);
			}
			
			std::ostringstream os;
			script.prettyPrint (os, rts2script::PRINT_SCRIPT);
			tar->setScript (camera, os.str ().c_str ());
		}
		catch (rts2db::CameraMissingExcetion &ex)
		{
			std::cerr << "Missing camera " << camera << ". Is it filled in \"cameras\" database table?" << std::endl;
		}
	}
}

int TargetApp::doProcessing ()
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

	try
	{
		target_set.load (tar_names, matchAll ? rts2db::resolveAll : rts2db::consoleResolver, true, resType);
		if (op & OP_DELETE)
		{
			for (rts2db::TargetSet::iterator iter = target_set.begin (); iter != target_set.end (); iter++)
			{
				if (confirmOp)
				{
					std::ostringstream os;
					os << "Delete " << iter->second->getTargetName () << " (yes/no/all)?";
					char ch;
					askForChr (os.str ().c_str (), ch);
					if (! (ch == 'y' || ch == 'Y' || ch == 'a' || ch == 'A') )
						continue;
					if (ch == 'a' || ch == 'A')
						confirmOp = false;
				}
				std::cout << "Deleting " << iter->second->getTargetName ();
				iter->second->deleteTarget ();
				std::cout << "." << std::endl;
			}
			return 0;
		}
		if ((op & OP_MASK_EN) == OP_ENABLE)
		{
			target_set.setTargetEnabled (true, true);
		}
		if ((op & OP_MASK_EN) == OP_DISABLE)
		{
			target_set.setTargetEnabled (false, true);
		}
		if (op & OP_PRIORITY)
		{
			target_set.setTargetPriority (new_priority);
		}
		if (op & OP_TEMPDISABLE)
		{
			setTempdisable ();
		}
		if (op & OP_BONUS)
		{
			target_set.setTargetBonus (new_bonus);
		}
		if (op & OP_BONUS_TIME)
		{
			target_set.setTargetBonusTime (&new_bonus_time);
		}
		if (op & OP_NEXT_TIME)
		{
			target_set.setNextObservable (&new_next_time);
		}
		if (op & OP_SCRIPT)
		{
			for (std::list < CamScript >::iterator iter = new_scripts.begin (); iter != new_scripts.end (); iter++)
			{
				rts2script::Script script = rts2script::Script (iter->script);
				script.parseScript (((Rts2Target *) target_set.begin ()->second));
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
					target_set.setTargetScript (iter->cameraName, iter->script);
				}
				catch (rts2db::CameraMissingExcetion &ex)
				{
					std::cerr << "Missing camera " << iter->cameraName << ". Is it filled in \"cameras\" database table?" << std::endl;
				}
			}
		}
		if (op & OP_CONSTRAINTS)
		{
			try
			{
				if (op & OP_CONS_RESET)
				{
					target_set.setConstraints (constraints);
					std::cout << "Set constraints for:" << std::endl << target_set << std::endl;
				}
				else
				{
					target_set.appendConstraints (constraints);
					std::cout << "Changed constraints for:" << std::endl << target_set << std::endl;
				}
			}
			catch (rts2core::Error f)
			{
				std::cerr << "Cannot write target constraint file: " << f << std::endl;
			}
		}
		if (op & OP_NEXT_OBSER)
		{
			target_set.setNextObservable (NULL);
		}
		if (op & OP_OBS_SLEW)
		{
			if (target_set.size () != 1)
			{
				std::cerr << "You must specify only single target which observation will be started." << std::endl;
				return -1;
			}
			rts2db::Target *tar = (target_set.begin ())->second;
			struct ln_equ_posn pos;
			tar->startSlew (&pos, true);
			std::cout << tar->getObsId () << std::endl;
			tar->setObsId (-1);
			return 0;
		}
		if (op & OP_PI_NAME)
		{
			target_set.setTargetPIName (pi);
		}
		if (op & OP_PROGRAM_NAME)
		{
			target_set.setTargetProgramName (program);
		}
		if (op & OP_SET_PROPER_MOTION)
		{
			target_set.setTargetProperMotion (&pm);
		}
		if (op == OP_NONE)
		{
			return runInteractive ();
		}

		return target_set.save (true);
	}
	catch (rts2db::UnresolvedTarget ut)
	{
		std::cerr << "error: " << ut << std::endl;
	}
	catch (rts2db::SqlError e)
	{
		std::cerr << e << std::endl;
	}
	return -1;
}

int main (int argc, char **argv)
{
	TargetApp app (argc, argv);
	return app.run ();
}
