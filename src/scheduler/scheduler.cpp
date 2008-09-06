/*
 * Scheduling body.
 * Copyright (C) 2008 Petr Kubanek <petr@kubanek.net>
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
#include "../utils/rts2config.h"

#include "rts2schedbag.h"

/**
 * Class of the scheduler application.  Prepares schedule, and run
 * optimalization few times to get them out..
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Rts2ScheduleApp: public Rts2AppDb
{
	private:
		Rts2SchedBag *schedBag;

		// verbosity level
		int verbose;

		/**
		 * Prints merits statistics of schedule set.
		 */
		void printMerits ();

	protected:
		virtual void usage ();
		virtual void help ();
		virtual int processOption (int _opt);
		virtual int init ();

	public:
		Rts2ScheduleApp (int argc, char ** argv);
		virtual ~Rts2ScheduleApp (void);

		virtual int doProcessing ();
};

void
Rts2ScheduleApp::printMerits ()
{
	Rts2SchedBag::iterator iter;
	double sum = 0;

	std::cout << "visibility: ";

	for (iter = schedBag->begin (); iter != schedBag->end (); iter++)
	{
		std::cout << std::left << std::setw (8) << (*iter)->visibilityRatio () << " ";
		sum += (*iter)->visibilityRatio ();
	}

	std::cout << std::endl << " avg, visibility: " << (sum / schedBag->size ()) << std::endl
		<< "altitude:   ";

	sum = 0;

	for (iter = schedBag->begin (); iter != schedBag->end (); iter++)
	{
		std::cout << std::left << std::setw (8) << (*iter)->altitudeMerit () << " ";
		sum += (*iter)->altitudeMerit ();
	}

	std::cout << std::endl << "avg. altitude: " << (sum / schedBag->size ()) << std::endl;

	sum = 0;

	for (iter = schedBag->begin (); iter != schedBag->end (); iter++)
	{
		std::cout << std::left << std::setw (8) << (*iter)->accountMerit () << " ";
		sum += (*iter)->accountMerit ();
	}

	std::cout << std::endl << "avg. account: " << (sum / schedBag->size ()) << std::endl;

}


void
Rts2ScheduleApp::usage ()
{
	std::cout << "\t" << getAppName () << std::endl;
}


void
Rts2ScheduleApp::help ()
{
	std::cout << "Create schedule for given night. The programme uses genetic algorithm scheduling, described at \
http://rts-2.sf.net/scheduling.pdf." << std::endl
		<< "You are free to experiment with various settings to create optimal observing schedule" << std::endl;
	Rts2AppDb::help ();
}


int
Rts2ScheduleApp::processOption (int _opt)
{
	switch (_opt)
	{
		case 'v':
			verbose++;
			break;
		default:
			return Rts2AppDb::processOption (_opt);
	}
	return 0;
}


int
Rts2ScheduleApp::init ()
{
	int ret;
	ret = Rts2AppDb::init ();
	if (ret)
		return ret;

	srandom (time (NULL));

	// initialize schedules..
	double jd = ln_get_julian_from_sys ();

	// create list of schedules..
	schedBag = new Rts2SchedBag (jd, jd + 0.5);

	ret = schedBag->constructSchedules (20);
	if (ret)
		return ret;

	return 0;
}


Rts2ScheduleApp::Rts2ScheduleApp (int argc, char ** argv): Rts2AppDb (argc, argv)
{
	schedBag = NULL;
	verbose = 0;

	addOption ('v', NULL, 0, "verbosity level");
}


Rts2ScheduleApp::~Rts2ScheduleApp (void)
{
	delete schedBag;
}


int
Rts2ScheduleApp::doProcessing ()
{
	if (verbose)
	  	printMerits ();

	for (int i = 1; i <= 1500; i++)
	{
		schedBag->doGAStep ();

		double _min, _avg, _max;
		schedBag->getStatistics (_min, _avg, _max);

		std::cout << std::right << std::setw (5) << i << " "
			<< std::setw (4) << schedBag->size () << " "
			<< std::left << std::setw (8) << _min << " "
			<< std::left << std::setw (8) << _avg << " "
			<< std::left << std::setw (8) << _max << std::endl;
	}

	if (verbose)	
		printMerits ();

	return 0;
}


int
main (int argc, char ** argv)
{
	Rts2ScheduleApp app = Rts2ScheduleApp (argc, argv);
	return app.run ();
}
