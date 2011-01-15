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
#define OPT_FILTER_FILE  OPT_LOCAL + 631
#define OPT_FILTER_ALIAS OPT_LOCAL + 632

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
		virtual void usage ();

		virtual int doProcessing ();
	private:
		int verbosity;

		rts2plan::Selector sel;
};

}

using namespace rts2plan;

SelectorApp::SelectorApp (int in_argc, char **in_argv):PrintTarget (in_argc, in_argv),sel()
{
	verbosity = 0;
	addOption ('v', NULL, 0, "increase verbosity");

	addOption (OPT_FILTERS, "available-filters", 1, "available filters for given camera. Camera name is separated with space, filters with :");
	addOption (OPT_FILTER_FILE, "filter-file", 1, "available filter for camera and file separated with :");
	addOption (OPT_FILTER_ALIAS, "filter-aliases", 1, "filter aliases file");
}

SelectorApp::~SelectorApp (void)
{
}

int SelectorApp::processOption (int opt)
{
	std::vector <std::string> cam_filters;
	switch (opt)
	{
		case 'v':
			verbosity++;
			break;
		case OPT_FILTERS:
			sel.parseFilterOption (optarg);
			break;
		case OPT_FILTER_FILE:
			sel.parseFilterFileOption (optarg);
			break;
		case OPT_FILTER_ALIAS:
			sel.readAliasFile (optarg);
			break;
		default:
			return PrintTarget::processOption (opt);
	}
	return 0;
}

void SelectorApp::usage ()
{
	std::cout
		<< " To simply see what's available:" << std::endl
		<< "  " << getAppName () << std::endl
		<< " To specify that C0 is equipped with C and R filters (and so ignore all observations requiring other filters):" << std::endl
		<< "  " << getAppName () << " --available-filters 'C0 C:R'" << std::endl;
}

int SelectorApp::doProcessing ()
{
	int next_tar;

	Rts2Config *config;
	struct ln_lnlat_posn *observer;

	rts2db::Target *tar;

	config = Rts2Config::instance ();
	observer = config->getObserver ();

	sel.setObserver (observer);
	sel.init ();

	next_tar = sel.selectNextNight (0, verbosity);

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

	return 0;
}

int main (int argc, char **argv)
{
	SelectorApp selApp = SelectorApp (argc, argv);
	return selApp.run ();
}
