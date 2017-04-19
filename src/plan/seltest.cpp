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

#include "rts2script/printtarget.h"
#include "selector.h"

#define OPT_FILTERS             OPT_LOCAL + 630
#define OPT_FILTER_FILE         OPT_LOCAL + 631
#define OPT_FILTER_ALIAS        OPT_LOCAL + 632
#define OPT_PRINT_POSSIBLE      OPT_LOCAL + 633
#define OPT_PRINT_SATISFIED     OPT_LOCAL + 634
#define OPT_PRINT_DISSATISFIED  OPT_LOCAL + 635
#define OPT_MAXLENGTH           OPT_LOCAL + 636

namespace rts2plan
{

typedef enum {NONE, POSSIBLE, SATISIFED, DISSATISIFIED} printFilter_t;

class PrintFilter
{
	public:
		PrintFilter (printFilter_t _pf, double _JD) { pf = _pf; JD = _JD; }

		bool operator () (rts2db::Target * t)
		{
			switch (pf)
			{
				case NONE:
					return false;  
				case SATISIFED:
					return t->checkConstraints (JD);
				case DISSATISIFIED:
					return !t->checkConstraints (JD);
				default:
					return true;	
			}
		}

	private:
		printFilter_t pf;
		double JD;
};

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
		bool interactive;
		printFilter_t printFilter;

		rts2plan::Selector sel;

		int runInteractive ();
		void disableTargets ();
		double maxLength;
};

}

using namespace rts2plan;

SelectorApp::SelectorApp (int in_argc, char **in_argv):PrintTarget (in_argc, in_argv), sel(&cameras)
{
	addOption (OPT_FILTERS, "available-filters", 1, "available filters for given camera. Camera name is separated with space, filters with :");
	addOption (OPT_FILTER_FILE, "filter-file", 1, "available filter for camera and file separated with :");
	addOption (OPT_FILTER_ALIAS, "filter-aliases", 1, "filter aliases file");

	verbosity = 0;
	addOption ('v', NULL, 0, "increase verbosity");

	addOption (OPT_MAXLENGTH, "max-length", 1, "maximal allowed length");
	maxLength = NAN;

	printFilter = NONE;
	addOption (OPT_PRINT_SATISFIED, "print-satisfied", 0, "print all available targets satisfiing observing conditions, ordered by priority");
	addOption (OPT_PRINT_DISSATISFIED, "print-dissatisfied", 0, "print all available targets not satisfiing observing conditions, ordered by priority");
	addOption (OPT_PRINT_POSSIBLE, "print-possible", 0, "print all available targets, ordered by priority");

	interactive = false;
	addOption ('i', NULL, 0, "interactive mode (allows modifing priorities,..");
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
		case OPT_PRINT_POSSIBLE:
			if (printFilter != NONE)
				return -1;  
			printFilter = POSSIBLE;
			break;
		case OPT_PRINT_SATISFIED:
			if (printFilter != NONE)
				return -1;
			printFilter = SATISIFED;
			break;
		case OPT_PRINT_DISSATISFIED:
			if (printFilter != NONE)
				return -1;  
			printFilter = DISSATISIFIED;
			break;
		case OPT_MAXLENGTH:
			maxLength = atof (optarg);
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
		case 'i':
			interactive = true;
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

	Configuration *config;
	struct ln_lnlat_posn *observer;

	config = Configuration::instance ();
	observer = config->getObserver ();
	obs_altitude = config->getObservatoryAltitude ();

	sel.setObserver (observer, obs_altitude);
	sel.init ();

	next_tar = sel.selectNextNight (0, verbosity, maxLength);

	if (next_tar < 0)
	{
		if (!std::isnan (maxLength))
			std::cout << "cannot find any possible targets with maximal length " << TimeDiff (maxLength) << std::endl;
		else
			std::cout << "cannot find any possible targets" << std::endl;
		return -1;	
	}

	if (printFilter != NONE)
	  	sel.printPossible (std::cout, PrintFilter (printFilter, ln_get_julian_from_sys ()));

	if (interactive)
		return runInteractive ();  

	rts2db::Target *tar = createTarget (next_tar, observer, obs_altitude);
	
	if (tar)
	{
		printTarget (tar);
	}
	else
	{
		std::cout << "cannot create target with ID " << next_tar << std::endl;
	}

	delete tar;

	return 0;
}

int SelectorApp::runInteractive ()
{
	rts2core::AskChoice main (this);
	main.addChoice ('p', "Print targets");
	main.addChoice ('d', "Disable targets");
	main.addChoice ('s', "Save targets");
	main.addChoice ('q', "Quit");

	while (true)
	{
		char ret;
		ret = main.query (std::cout);
		switch (ret)
		{
			case 'p':
				sel.printPossible (std::cout, PrintFilter (POSSIBLE, 0));
				break;
			case 'd':
				disableTargets ();
				break;	
			case 's':
				sel.saveTargets ();
				break;	
			case 'q':
				return 0;
			default:
				std::cerr << "Unknow key pressed: " << ret << std::endl;
				break;
		}
	}
}

void SelectorApp::disableTargets ()
{
	int n = 1;
	askForInt ("Enter target index from the printout", n);
	if (n <= 0)
	{
		std::cerr << "Invalid index " << n << std::endl;
		return;
	}
	sel.disableTarget (n);
}

int main (int argc, char **argv)
{
	SelectorApp selApp = SelectorApp (argc, argv);
	return selApp.run ();
}
