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
#include "configuration.h"
#include "libnova_cpp.h"

#include "rts2db/simbadtarget.h"

#include <sstream>
#include <iostream>

using namespace rts2db;

Rts2TargetApp::Rts2TargetApp (int in_argc, char **in_argv):AppDb (in_argc, in_argv)
{
	target = NULL;
	obs = NULL;
}

Rts2TargetApp::~Rts2TargetApp (void)
{
	delete target;
}

void Rts2TargetApp::getObject (std::string obj_text)
{
	target = createTargetByString (obj_text, getDebug ());
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

	try
	{
		getObject (obj_text);
		return 0;
	}
	catch (rts2core::Error err)
	{
		std::cerr << err;
		return -1;
	}
}

int Rts2TargetApp::init ()
{
	int ret;

	ret = AppDb::init ();
	if (ret)
		return ret;

	rts2core::Configuration *config;
	config = rts2core::Configuration::instance ();

	if (!obs)
	{
		obs = config->getObserver ();
	}

	return 0;
}
