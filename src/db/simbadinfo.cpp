/* 
 * SOAP name resolver.
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
#include "../utilsdb/target.h"
#include "../utilsdb/rts2obsset.h"
#include "../utils/rts2config.h"
#include "../utils/libnova_cpp.h"
#include "../utils/rts2askchoice.h"
#include "simbad/rts2simbadtarget.h"

#include "rts2targetapp.h"

#include <iostream>
#include <iomanip>
#include <list>
#include <stdlib.h>
#include <sstream>

class Rts2SimbadInfo:public Rts2TargetApp
{
	private:
		char *name;
		bool prettyPrint;
		bool visibilityPrint;
	protected:
		virtual int processOption (int in_opt);

	public:
		Rts2SimbadInfo (int in_argc, char **in_argv);
		virtual ~ Rts2SimbadInfo (void);

		virtual int doProcessing ();
};

Rts2SimbadInfo::Rts2SimbadInfo (int in_argc, char **in_argv):
Rts2TargetApp (in_argc, in_argv)
{
	name = NULL;
	prettyPrint = false;
	visibilityPrint = false;

	addOption ('p', NULL, 1, "prints target coordinates and exit");
	addOption ('P', NULL, 1, "pretty print extented target coordinates and exit");
	addOption ('v', NULL, 1, "pretty print target visibility and exit");
}


Rts2SimbadInfo::~Rts2SimbadInfo ()
{
}


int
Rts2SimbadInfo::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'P':
			prettyPrint = true;
		case 'p':
			name = optarg;
			break;
		case 'v':
			visibilityPrint = true;
			name = optarg;
			break;
		default:
			return Rts2AppDb::processOption (in_opt);
	}
	return 0;
}


int
Rts2SimbadInfo::doProcessing ()
{
	int ret;
	if (name)
	{
		target = new Rts2SimbadTarget (name);
		ret = target->load ();
		if (ret)
		{
			std::cerr << "Cannot resolve name " << name << std::endl;
			return -1;
		}
		struct ln_equ_posn pos;
		target->getPosition (&pos);
		if (visibilityPrint)
		{
			target->printAltTable (std::cout, ln_get_julian_from_sys ());	
		}
		if (prettyPrint)
		{
			struct ln_hrz_posn hrz;
			target->getAltAz (&hrz);
			std::cout << "EQU " << LibnovaRaDec (&pos) << std::endl
				<< "HRZ " << LibnovaHrz (&hrz) << std::endl;
		}
		else
		{
			std::cout.precision (15);
			std::cout << pos.ra << "\t" << pos.dec << std::endl;
		}
		return 0;
	}
	static double radius = 10.0 / 60.0;
	// ask for target ID..
	std::cout << "Default values are written at [].." << std::endl;
	ret = askForObject ("Target name, RA&DEC or anything else");
	if (ret)
		return ret;

	std::cout << target;

	Rts2AskChoice selection = Rts2AskChoice (this);
	selection.addChoice ('s', "Save");
	selection.addChoice ('q', "Quit");
	selection.addChoice ('o', "List observations around position");
	selection.addChoice ('t', "List targets around position");
	selection.addChoice ('i', "List images around position");

	while (1)
	{
		char sel_ret;
		sel_ret = selection.query (std::cout);
		switch (sel_ret)
		{
			case 's':
			case 'q':
				return 0;
			case 'o':
				askForDegrees ("Radius", radius);
				target->printObservations (radius, std::cout);
				break;
			case 't':
				askForDegrees ("Radius", radius);
				target->printTargets (radius, std::cout);
				break;
			case 'i':
				target->printImages (std::cout);
				break;
		}
	}
}


int
main (int argc, char **argv)
{
	Rts2SimbadInfo app = Rts2SimbadInfo (argc, argv);
	return app.run ();
}
