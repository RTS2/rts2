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

#include "printtarget.h"
#include "../utilsdb/target_auger.h"

#define OPT_AUGER_ID   OPT_LOCAL + 201

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
		std::list < int >targets;
		bool printSelectable;
		bool printAuger;
		char *targetType;
};

}

using namespace rts2plan;

TargetInfo::TargetInfo (int argc, char **argv):PrintTarget (argc, argv)
{
	printSelectable = false;
	printAuger = false;
	targetType = NULL;

	addOption ('s', NULL, 0, "print only selectable targets");
	addOption ('t', NULL, 1, "search for target types, not for targets IDs");
	addOption (OPT_AUGER_ID, "auger-id", 0, "specify trigger(s) number for Auger target(s)");
}

TargetInfo::~TargetInfo ()
{
}

int TargetInfo::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 's':
			printSelectable = true;
			break;
		case 't':
			targetType = optarg;
			break;
		case OPT_AUGER_ID:
			printAuger = true;
			break;
		default:
			return PrintTarget::processOption (in_opt);
	}
	return 0;
}

int TargetInfo::processArgs (const char *arg)
{
	// try to create that target..
	int tar_id;
	char *test;
	tar_id = strtol (arg, &test, 10);
	if (*test)
	{
		std::cerr << "Invalid target id: " << arg << std::endl;
		return -1;
	}
	targets.push_back (tar_id);
	return 0;
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
		for (std::list <int>::iterator iter = targets.begin (); iter != targets.end (); iter++)
		{
			TargetAuger *ta = new TargetAuger (-1, obs, 10);
			if (ta->load (*iter))
			{
				delete ta;
			}
			else
			{
				tar_set[*iter] = ta;
			}
		}
	}
	else
	{
		// normal target set load
		tar_set.load (targets);
	}
	return printTargets (tar_set);
}

int main (int argc, char **argv)
{
	TargetInfo app = TargetInfo (argc, argv);
	return app.run ();
}
