/* 
 * Create new observation target.
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
#include "../utils/rts2config.h"
#include "../utils/libnova_cpp.h"
#include "../utils/rts2askchoice.h"

#include "rts2targetapp.h"

#include <iostream>
#include <iomanip>
#include <list>
#include <stdlib.h>
#include <sstream>

class Rts2NewTarget:public Rts2TargetApp
{
	private:
		int n_tar_id;
		const char *n_tar_name;
		const char *n_tar_ra_dec;
		double radius;

		int saveTarget ();
	protected:
		virtual void help ();

		virtual int processOption (int in_opt);
		virtual int processArgs (const char *arg);
	public:
		Rts2NewTarget (int in_argc, char **in_argv);
		virtual ~ Rts2NewTarget (void);

		virtual int doProcessing ();
};

Rts2NewTarget::Rts2NewTarget (int in_argc, char **in_argv):
Rts2TargetApp (in_argc, in_argv)
{
	n_tar_id = INT_MIN;
	n_tar_name = NULL;
	n_tar_ra_dec = NULL;
	radius = nan ("f");
	addOption ('r', "radius", 2, "radius for target checks");
}


Rts2NewTarget::~Rts2NewTarget (void)
{
}


void
Rts2NewTarget::help ()
{
	Rts2TargetApp::help ();
	std::cout
		<<
		"You can specify target on command line. Arguments must be in following order:"
		<< std::endl << "  <target_id> <target_name> <target ra + dec>" << std::
		endl <<
		"If you specify them, you will be quired only if there exists target within 10' from target which you specified"
		<< std::endl;
}


int
Rts2NewTarget::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'r':
			if (optarg)
				radius = atof (optarg);
			else
				radius = 1.0 / 60.0;
			break;
		default:
			return Rts2TargetApp::processOption (in_opt);
	}
	return 0;
}


int
Rts2NewTarget::processArgs (const char *arg)
{
	if (n_tar_id == INT_MIN)
		n_tar_id = atoi (arg);
	else if (n_tar_name == NULL)
		n_tar_name = arg;
	else if (n_tar_ra_dec == NULL)
		n_tar_ra_dec = arg;
	else
		return -1;
	return 0;
}


int
Rts2NewTarget::saveTarget ()
{
	std::string target_name;
	int ret;

	if (n_tar_id == INT_MIN)
		askForInt ("Target ID", n_tar_id);
	// create target if we don't create it..
	if (n_tar_name == NULL)
	{
		target_name = target->getTargetName ();
		askForString ("Target NAME", target_name);
	}
	else
	{
		target_name = std::string (n_tar_name);
	}
	target->setTargetName (target_name.c_str ());

	if (!isnan (radius))
	{
		rts2db::TargetSet tarset = target->getTargets (radius);
		tarset.load ();
		if (tarset.size () == 0)
		{
			std::
				cout << "No targets were found within " << LibnovaDegDist (radius)
				<< " from entered target." << std::cout;
		}
		else
		{
			std::
				cout << "Following targets were found within " <<
				LibnovaDegDist (radius) << " from entered target:" << std::
				endl << tarset << std::endl;
			if (askForBoolean ("Would you like to enter target anyway?", false)
				== false)
			{
				std::cout << "No target created, exiting." << std::endl;
				return -1;
			}
		}
	}

	if (n_tar_id != INT_MIN)
		ret = target->save (false, n_tar_id);
	else
		ret = target->save (false);

	if (ret)
	{
		if (askForBoolean
			("Target with given ID already exists. Do you want to overwrite it?",
			false))
		{
			if (n_tar_id != INT_MIN)
				ret = target->save (true, n_tar_id);
			else
				ret = target->save (true);
		}
		else
		{
			std::cout << "No target created, exiting." << std::endl;
			return -1;
		}
	}

	std::cout << *target;

	if (ret)
	{
		std::cerr << "Error when saving target." << std::endl;
	}
	return ret;
}


int
Rts2NewTarget::doProcessing ()
{
	double t_radius = 10.0 / 60.0;
	if (!isnan (radius))
		t_radius = radius;
	int ret;
	// ask for target name..
	if (n_tar_ra_dec == NULL)
	{
		if (n_tar_name == NULL)
		{
			std::cout << "Default values are written at [].." << std::endl;
			ret = askForObject ("Target name, RA&DEC or anything else");
		}
		else
		{
			ret =
				askForObject ("Target name, RA&DEC or anything else",
				std::string (n_tar_name));
		}
	}
	else
	{
		ret =
			askForObject ("Target, RA&DEC or anything else",
			std::string (n_tar_ra_dec));
	}
	if (ret)
		return ret;

	if (n_tar_id != INT_MIN)
		return saveTarget ();

	Rts2AskChoice selection = Rts2AskChoice (this);
	selection.addChoice ('s', "Save");
	selection.addChoice ('q', "Quit");
	selection.addChoice ('o', "List observations around position");
	selection.addChoice ('t', "List targets around position");

	while (1)
	{
		char sel_ret;
		sel_ret = selection.query (std::cout);
		switch (sel_ret)
		{
			case 's':
				return saveTarget ();
			case 'q':
				return 0;
			case 'o':
				askForDegrees ("Radius", t_radius);
				target->printObservations (t_radius, std::cout);
				break;
			case 't':
				askForDegrees ("Radius", t_radius);
				target->printTargets (t_radius, std::cout);
				break;
			default:
				std::cerr << "Unknow key pressed: " << sel_ret << std::endl;
				return -1;
		}
	}
}


int
main (int argc, char **argv)
{
	Rts2NewTarget app = Rts2NewTarget (argc, argv);
	return app.run ();
}
