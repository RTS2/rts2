/* 
 * Prints informations about target.
 * Copyright (C) 2003-2007 Petr Kubanek <petr@kubanek.net>
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

#include "rts2script/printtarget.h"
#include "rts2db/sqlerror.h"
#include "rts2db/targetset.h"
#include "rts2db/target_auger.h"

#include <ecpgerrno.h>

#define OPT_AUGER_ID              OPT_LOCAL + 501
#define OPT_ID_ONLY               OPT_LOCAL + 502
#define OPT_NAME_ONLY             OPT_LOCAL + 503

namespace rts2plan
{

class TargetInfo:public PrintTarget
{
	public:
		TargetInfo (int argc, char **argv);
		virtual ~ TargetInfo (void);

	protected:
		virtual int processOption (int in_opt);
		virtual int processArgs (const char *arg);
		virtual int doProcessing ();

	private:
		std::vector < const char * >targets;
		bool printSelectable;
		bool printAuger;
		bool matchAll;
		bool unique;
		char *targetType;

		rts2db::resolverType resType;
};

}

using namespace rts2plan;

TargetInfo::TargetInfo (int argc, char **argv):PrintTarget (argc, argv)
{
	printSelectable = false;
	printAuger = false;
	matchAll = false;
	unique = false;
	targetType = NULL;
	resType = rts2db::NAME_ID;

	addOption ('a', NULL, 0, "select all matching target (if search by name gives multiple targets)");
	addOption ('s', NULL, 0, "print only selectable targets");
	addOption ('t', NULL, 1, "search for target types, not for targets IDs");
	addOption ('u', NULL, 0, "require unique target - search for name");
	addOption (OPT_AUGER_ID, "auger-id", 0, "specify trigger(s) number for Auger target(s)");
	addOption (OPT_ID_ONLY, "id-only", 0, "expect numeric target(s) names (IDs only)");
	addOption (OPT_NAME_ONLY, "name-only", 0, "resolver target(s) as names (even pure numbers)");
}

TargetInfo::~TargetInfo ()
{
}

int TargetInfo::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'a':
			matchAll = true;
			break;
		case 's':
			printSelectable = true;
			break;
		case 't':
			targetType = optarg;
			break;
		case 'u':
			unique = true;
			break;
		case OPT_AUGER_ID:
			printAuger = true;
			break;
		case OPT_ID_ONLY:
			resType = rts2db::ID_ONLY;
			break;
		case OPT_NAME_ONLY:
			resType = rts2db::NAME_ONLY;
			break;
		default:
			return PrintTarget::processOption (in_opt);
	}
	return 0;
}

int TargetInfo::processArgs (const char *arg)
{
	// try to create that target..
	targets.push_back (arg);
	return 0;
}

rts2db::TargetSet::iterator const uniqueResolver (rts2db::TargetSet *ts)
{
	if (ts->size () != 1)
	{
		std::cerr << "no target found!";
		exit (1);
	}
	return ts->begin ();
}

int TargetInfo::doProcessing ()
{
	if (printSelectable)
	{
		if (targetType)
		{
			rts2db::TargetSetSelectable selSet = rts2db::TargetSetSelectable (targetType);
			selSet.load ();
			return printTargets (selSet);
		}
		else
		{
			rts2db::TargetSetSelectable selSet = rts2db::TargetSetSelectable ();
			selSet.load ();
			return printTargets (selSet);
		}
	}
	if (targetType)
	{
		rts2db::TargetSet typeSet = rts2db::TargetSet (targetType);
		typeSet.load ();
		return printTargets (typeSet);
	}

	rts2db::TargetSet tar_set = rts2db::TargetSet ();
	if (printAuger)
	{
		for (std::vector <const char*>::iterator iter = targets.begin (); iter != targets.end (); iter++)
		{
			rts2db::TargetAuger *ta = new rts2db::TargetAuger (-1, obs, obs_altitude, 10);
			int tid = atoi (*iter);
			try
			{
				ta->load (tid);
				tar_set[tid] = ta;
			}
			catch (rts2db::SqlError err)
			{
				delete ta;
				if (err.getSqlCode () == ECPG_NOT_FOUND)
				{
					std::cerr << "target with given ID not found" << std::endl;
				}
				else
				{
					throw err;
				}
			}
		}
	}
	else
	{
		try
		{
			if (unique)
			{
				tar_set.load (targets, uniqueResolver, false, resType);
				if (tar_set.empty ())
				{
					std::cerr << "target not found" << std::endl;
					exit (1);
				}
			}	
			else
			{
				// normal target set load
				tar_set.load (targets, (matchAll ? rts2db::resolveAll : rts2db::consoleResolver), true, resType);
			}
		}
		catch (rts2core::Error er)
		{
			std::cerr << er << std::endl;
			exit (1);
		}
	}
	return printTargets (tar_set);
}

int main (int argc, char **argv)
{
	TargetInfo app = TargetInfo (argc, argv);
	return app.run ();
}
