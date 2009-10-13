/* 
 * Application which display various lists of targets.
 * Copyright (C) 2006-2009 Petr Kubanek <petr@kubanek.net>
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
#include "../utils/rts2config.h"
#include "../utils/rts2format.h"

#include <iostream>

#define LIST_ALL  0x00
#define LIST_GRB  0x01
#define LIST_SELECTABLE 0x02

class Rts2TargetList:public Rts2AppDb
{
	private:
		// which target to list
		int list;
		char *targetType;

	protected:
		virtual int processOption (int in_opt);

		virtual int processArgs (const char *arg);
		virtual int init ();

		virtual int doProcessing ();
	public:
		Rts2TargetList (int argc, char **argv);
		virtual ~ Rts2TargetList (void);
};

Rts2TargetList::Rts2TargetList (int in_argc, char **in_argv):
Rts2AppDb (in_argc, in_argv)
{
	list = LIST_ALL;
	targetType = NULL;

	addOption ('g', "grb", 0, "list onlu GRBs");
	addOption ('s', "selectable", 0,
		"list only targets considered by selector");
	addOption ('t', "target_type", 1, "print given target types");
	addOption ('N', NULL, 0, "do not pretty print");
}


Rts2TargetList::~Rts2TargetList ()
{
}


int
Rts2TargetList::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'g':
			list |= LIST_GRB;
			break;
		case 's':
			list |= LIST_SELECTABLE;
			break;
		case 't':
			targetType = optarg;
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
Rts2TargetList::processArgs (const char *arg)
{
	return Rts2AppDb::processArgs (arg);
}


int
Rts2TargetList::init ()
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
Rts2TargetList::doProcessing ()
{
	rts2db::TargetSetGrb *tar_set_grb;
	rts2db::TargetSet *tar_set;
	switch (list)
	{
		case LIST_GRB:
			tar_set_grb = new rts2db::TargetSetGrb ();
			tar_set_grb->load ();
			tar_set_grb->printGrbList (std::cout);
			delete tar_set_grb;
			break;
		case LIST_SELECTABLE:
			tar_set = new rts2db::TargetSetSelectable (targetType);
			tar_set->load ();
			tar_set->printBonusList (std::cout, ln_get_julian_from_sys ());
			delete tar_set;
			break;
		default:
			tar_set = new rts2db::TargetSet (targetType);
			tar_set->load ();
			std::cout << (*tar_set);
			delete tar_set;
			break;
	}
	return 0;
}


int
main (int argc, char **argv)
{
	Rts2TargetList app = Rts2TargetList (argc, argv);
	return app.run ();
}
