/* 
 * Skeleton for applications which works with targets.
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

#include "rts2targetapp.h"
#include "simbad/simbadtarget.h"
#include "../utils/rts2config.h"
#include "../utils/libnova_cpp.h"
#include "../utilsdb/rts2targetell.h"

#include <sstream>
#include <iostream>

using namespace rts2db;

Rts2TargetApp::Rts2TargetApp (int in_argc, char **in_argv):Rts2AppDb (in_argc, in_argv)
{
	target = NULL;
	obs = NULL;
}

Rts2TargetApp::~Rts2TargetApp (void)
{
	delete target;
}

int Rts2TargetApp::getObject (const char *obj_text)
{
	LibnovaRaDec raDec;

	int ret = raDec.parseString (obj_text);
	if (ret == 0)
	{
		std::string new_prefix;

		// default prefix for new RTS2 sources
		new_prefix = std::string ("RTS2_");

		// set name..
		ConstTarget *constTarget = new ConstTarget ();
		constTarget->setPosition (raDec.getRa (), raDec.getDec ());
		std::ostringstream os;

		Rts2Config::instance ()->getString ("newtarget", "prefix", new_prefix);
		os << new_prefix << LibnovaRaComp (raDec.getRa ()) << LibnovaDeg90Comp (raDec.getDec ());
		constTarget->setTargetName (os.str ().c_str ());
		constTarget->setTargetType (TYPE_OPORTUNITY);
		target = constTarget;
		return 0;
	}
	// if it's MPC ephemeris..
	target = new EllTarget ();
	ret = ((EllTarget *) target)->orbitFromMPC (obj_text);
	if (ret == 0)
	{
		return 0;
	}

	delete target;
	target = NULL;

	// try to get target from SIMBAD
	target = new rts2db::SimbadTarget (obj_text);
	ret = target->load ();
	if (ret == 0)
	{
		target->setTargetType (TYPE_OPORTUNITY);
		return 0;
	}
	delete target;
	target = NULL;
	return -1;
}

int Rts2TargetApp::askForDegrees (const char *desc, double &val)
{
	LibnovaDegDist degDist = LibnovaDegDist (val);
	while (1)
	{
		std::cout << desc << " [" << degDist << "]: ";
		std::cin >> degDist;
		if (!std::cin.fail ())
			break;
		std::cout << "Invalid string!" << std::endl;
		std::cin.clear ();
		std::cin.ignore (2000, '\n');
	}
	val = degDist.getDeg ();
	std::cout << desc << ": " << degDist << std::endl;
	return 0;
}

int Rts2TargetApp::askForObject (const char *desc, std::string obj_text)
{
	int ret;
	if (obj_text.length () == 0)
	{
		ret = askForString (desc, obj_text);
		if (ret)
			return ret;
	}

	return getObject (obj_text.c_str ());
}

int Rts2TargetApp::init ()
{
	int ret;

	ret = Rts2AppDb::init ();
	if (ret)
		return ret;

	Rts2Config *config;
	config = Rts2Config::instance ();

	if (!obs)
	{
		obs = config->getObserver ();
	}

	return 0;
}
