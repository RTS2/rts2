/* 
 * Performs selection and print out next target scheduled for observation.
 * Copyright (C) 2003-2007,2010 Petr Kubanek <petr@kubanek.net>
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
#include "rts2selector.h"

#define OPT_FILTERS      OPT_LOCAL + 630

namespace rts2plan
{

/**
 * Selector test application class.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class SelectorApp:public PrintTarget
{
	public:
		SelectorApp (int argc, char **argv);
		virtual ~ SelectorApp (void);

	protected:
		virtual int processOption (int opt);
		virtual int doProcessing ();
	private:
		int verbosity;

		std::map <std::string, std::vector < std::string > > availableFilters;
};

}

using namespace rts2plan;

SelectorApp::SelectorApp (int in_argc, char **in_argv):PrintTarget (in_argc, in_argv)
{
	verbosity = 0;
	addOption ('v', NULL, 0, "increase verbosity");

	addOption (OPT_FILTERS, "available-filters", 1, "available filters for given camera. Camera name is separated with space, filters with :");
}

SelectorApp::~SelectorApp (void)
{
}

int SelectorApp::processOption (int opt)
{
	std::vector <std::string> cam_filters;
	std::vector <std::string> filters;
	switch (opt)
	{
		case 'v':
			verbosity++;
			break;
		case OPT_FILTERS:
			cam_filters = SplitStr (std::string (optarg), std::string (" "));
			if (cam_filters.size () != 2)
			{
			  	std::cerr << "camera must be separated by space from filter names" << std::endl;
				return -1;
			}
			availableFilters[cam_filters[0]] = SplitStr (cam_filters[1], std::string (":"));
			break;
		default:
			return PrintTarget::processOption (opt);
	}
	return 0;
}

int SelectorApp::doProcessing ()
{
	int next_tar;

	Rts2Config *config;
	struct ln_lnlat_posn *observer;

	rts2plan::Selector *sel;

	rts2db::Target *tar;

	config = Rts2Config::instance ();
	observer = config->getObserver ();

	sel = new rts2plan::Selector (observer);

	for (std::map < std::string, std::vector < std::string > >::iterator iter = availableFilters.begin (); iter != availableFilters.end (); iter++)
	{
		sel->addFilters (iter->first.c_str (), iter->second);
	}

	next_tar = sel->selectNextNight (0, verbosity);

	tar = createTarget (next_tar, observer);
	if (tar)
	{
		printTarget (tar);
	}
	else
	{
		std::cout << "cannot create target" << std::endl;
	}

	delete tar;

	delete sel;
	return 0;
}

int main (int argc, char **argv)
{
	SelectorApp selApp = SelectorApp (argc, argv);
	return selApp.run ();
}
