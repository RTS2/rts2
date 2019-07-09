/*
 * Application to test horizon files.
 * Copyright (C) 2005-2007 Petr Kubanek <petr@kubanek.net>
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

#include "cliapp.h"
#include "configuration.h"

#include <iostream>

#define OP_DUMP             0x01

/**
 * Class which will plot horizon from horizon file, possibly with
 * commands for GNUPlot.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class HorizonApp:public rts2core::CliApp
{
	private:
		char *configFile;
		char *horizonFile;

		int op;

	protected:
		virtual int processOption (int in_arg);

		virtual int init ();
	public:
		HorizonApp (int in_argc, char **in_argv);

		virtual int doProcessing ();
};

HorizonApp::HorizonApp (int in_argc, char **in_argv):rts2core::CliApp (in_argc, in_argv)
{
	op = 0;
	configFile = NULL;
	horizonFile = NULL;

	addOption (OPT_CONFIG, "config", 1, "configuration file");
	addOption ('f', NULL, 1,"horizon file; overwrites file specified in configuration file");
	addOption ('d', NULL, 0, "dump horizon file in AZ-ALT format");
	addOption ('A', NULL, 1, "check given alt-az pair (separated by :)");
	addOption ('R', NULL, 1, "check given ra-dec pair (separated by :)");
}

int HorizonApp::processOption (int in_opt)
{
	switch (in_opt)
	{
		case OPT_CONFIG:
			configFile = optarg;
			break;
		case 'f':
			horizonFile = optarg;
			break;
		case 'd':
			op = OP_DUMP;
			break;
		default:
			return rts2core::CliApp::processOption (in_opt);
	}
	return 0;
}

int HorizonApp::init ()
{
	int ret;

	ret = rts2core::CliApp::init ();
	if (ret)
		return ret;

	ret = rts2core::Configuration::instance ()->loadFile (configFile);
	if (ret)
		return ret;

	return 0;
}

int HorizonApp::doProcessing ()
{
	struct ln_hrz_posn hrz;
	ObjectCheck *checker;

	hrz.alt = 0;

	if (!horizonFile)
	{
		checker = rts2core::Configuration::instance ()->getObjectChecker ();
	}
	else
	{
		checker = new ObjectCheck ();
		if (checker->loadHorizon(horizonFile))
		{
			std::cerr << "Cannot load horizon file " << horizonFile << std::endl;
			return -1;
		}
	}

	if (op & OP_DUMP)
	{
		std::cout << "AZ-ALT" << std::endl;

		for (horizon_t::iterator iter = checker->begin ();
			iter != checker->end (); iter++)
		{
			std::cout << (*iter).hrz.az << " " << (*iter).hrz.alt << std::endl;
		}
	}
	else
	{
		std::cout
			<< "set terminal x11 persist" << std::endl
			<< " set xrange [0:360]" << std::endl;

		std::
			cout << "plot \"-\" u 1:2 smooth csplines lt 2 lw 2 t \"Horizon\"" <<
			std::endl;

		for (hrz.az = 0; hrz.az <= 360; hrz.az += 0.1)
		{
			std::cout << hrz.az << " " << checker->getHorizonHeight (&hrz,
				0) << std::
				endl;
		}
		std::cout << "e" << std::endl;
	}
	return 0;
}

int main (int argc, char **argv)
{
	HorizonApp app = HorizonApp (argc, argv);
	return app.run ();
}
