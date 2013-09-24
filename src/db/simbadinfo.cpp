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

#include "rts2db/appdb.h"
#include "rts2db/target.h"
#include "rts2db/simbadtarget.h"

#include "configuration.h"
#include "libnova_cpp.h"
#include "askchoice.h"

#include "rts2targetapp.h"

#include <iostream>
#include <iomanip>
#include <list>
#include <stdlib.h>
#include <sstream>

namespace rts2db
{


/**
 * Prints informations about object. Object names are
 * resolved using Simbad resolver.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class SimbadInfo:public Rts2TargetApp
{
	public:
		SimbadInfo (int argc, char **argv);
		virtual ~ SimbadInfo (void);

		virtual int doProcessing ();

	protected:
		virtual void usage ();

		virtual int processOption (int _opt);
		virtual int processArgs (const char *_arg);

	private:
		std::list <const char *> names;
		bool prettyPrint;
		bool visibilityPrint;
};

};

using namespace rts2db;

SimbadInfo::SimbadInfo (int argc, char **argv):Rts2TargetApp (argc, argv)
{
	prettyPrint = false;
	visibilityPrint = false;

	addOption ('p', NULL, 0, "prints target coordinates and exit");
	addOption ('P', NULL, 0, "pretty print extented target coordinates and exit");
	addOption ('v', NULL, 0, "pretty print target visibility and exit");
}

SimbadInfo::~SimbadInfo ()
{
}

void SimbadInfo::usage ()
{
	std::cout << "\t" << getAppName () << " 'M 31' 'NGC 321' 'IGR J05346-5759' 'TW Pic'" << std::endl
		<< "\t" << getAppName () << " -v 'M 31'" << std::endl
		<< getAppName () << " support communication through SIMBAD server using HTTP PROXY, specified in http_proxy environment variable." << std::endl;
}

int SimbadInfo::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'P':
			prettyPrint = true;
			break;
		case 'p':
			break;
		case 'v':
			visibilityPrint = true;
			break;
		default:
			return AppDb::processOption (in_opt);
	}
	return 0;
}

int SimbadInfo::processArgs (const char *arg)
{
	names.push_back (arg);
	return 0;
}

int SimbadInfo::doProcessing ()
{
	int ret;
	for (std::list <const char *>::iterator iter = names.begin (); iter != names.end (); iter++)
	{
	  	try
		{
			getObject (*iter);
		}
		catch (rts2core::Error err)
		{
			std::cerr << "Cannot resolve " << *iter << ":" << err << std::endl;
			continue;
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
	}
	if (names.size () != 0)
		return 0;

	static double radius = 10.0 / 60.0;
	// ask for target ID..
	std::cout << "Default values are written at [].." << std::endl;
	ret = askForObject ("Target name, RA&DEC or anything else");
	if (ret)
		return ret;

	std::cout << target;

	rts2core::AskChoice selection = rts2core::AskChoice (this);
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

int main (int argc, char **argv)
{
	SimbadInfo app = SimbadInfo (argc, argv);
	return app.run ();
}
