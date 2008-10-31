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

		// number of populations
		int generations;

		// population size
		int popSize;

		// used algorithm
		enum {SGA, NSGAII} algorithm;

		/**
		 * Print merit of given type.
		 *
		 * @param _type Merit type.
		 * @param _name Name of the merit function (for printing).
		 *
		 * @see objFunc
		 */
		void printMerits (objFunc _type, const char *name);

		/**
		 * Prints objective functions of NSGAII.
		 */
		void printNSGAMerits ();

		/**
		 * Prints all merits statistics of schedule set.
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
Rts2ScheduleApp::printMerits (objFunc _type, const char *name)
{
	Rts2SchedBag::iterator iter;
	std::cout << name << ": ";

	for (iter = schedBag->begin (); iter != schedBag->end (); iter++)
	{
		std::cout << std::left << std::setw (8) << (*iter)->getObjectiveFunction (_type) << " ";
	}

	double min, avg, max;
	schedBag->getStatistics (min, avg, max, _type);

	std::cout << std::endl << name << " statistics: "
		<< min << " "
		<< avg << " "
		<< max << std::endl;
}

void
Rts2ScheduleApp::printNSGAMerits ()
{
	Rts2SchedBag::iterator iter;
	for (iter = schedBag->begin (); iter < schedBag->end (); iter++)
	{
		std::cout << std::setw (3) << (*iter)->getNSGARank () <<
			" " << std::left << std::setw (8) << (*iter)->getObjectiveFunction (ALTITUDE) <<
			" " << std::left << std::setw (8) << (*iter)->getObjectiveFunction (ACCOUNT) <<
			" " << std::left << std::setw (8) << (*iter)->getObjectiveFunction (DISTANCE) <<
			" " << std::left << std::setw (8) << (*iter)->getObjectiveFunction (VISIBILITY) 
			<< std::endl; 
	}
}


void
Rts2ScheduleApp::printMerits ()
{
	switch (algorithm)
	{
		case SGA:
			printMerits (VISIBILITY, "visibility");
			printMerits (ALTITUDE, "altitude");
			printMerits (ACCOUNT, "account");
			printMerits (DISTANCE, "distance");
			printMerits (SINGLE, "single (main)");
			break;
		case NSGAII:
			printNSGAMerits ();
			break;
	}
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
http://rts2.org/scheduling.pdf." << std::endl
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
		case 'g':
			generations = atoi (optarg);
			break;
		case 'p':
			popSize = atoi (optarg);
			if (popSize <= 0)
			{
				logStream (MESSAGE_ERROR) << "Population size must be positive number " << optarg << sendLog;
				return -1;
			}
			break;		
		case 'a':
			if (!strcasecmp (optarg, "SGA"))
			{
				algorithm = SGA;
			}
			else if (!strcasecmp (optarg, "NSGAII"))
			{
			  	algorithm = NSGAII;
			}
			else
			{
				logStream (MESSAGE_ERROR) << "Unknow algorithm: " << optarg << sendLog;
			  	return -1;
			}
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

	ret = schedBag->constructSchedules (popSize);
	if (ret)
		return ret;

	return 0;
}


Rts2ScheduleApp::Rts2ScheduleApp (int argc, char ** argv): Rts2AppDb (argc, argv)
{
	schedBag = NULL;
	verbose = 0;
	generations = 1500;
	popSize = 100;
	algorithm = SGA;

	addOption ('v', NULL, 0, "verbosity level");
	addOption ('g', NULL, 1, "number of generations");
	addOption ('p', NULL, 1, "population size");
	addOption ('a', NULL, 1, "algorithm (SGA or NSGAII are currently supported)");
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

	for (int i = 1; i <= generations; i++)
	{
		switch (algorithm)
		{
			case SGA:
				schedBag->doGAStep ();
				break;
			case NSGAII:
				schedBag->doNSGAIIStep ();
				break;
		}


		if (verbose > 1)
		{
			printMerits ();
		}
		else
		{
			// collect and print statistics..
			double _min, _avg, _max;
			schedBag->getStatistics (_min, _avg, _max);

			std::cout << std::right << std::setw (5) << i << " "
				<< std::setw (4) << schedBag->size () << " "
				<< std::left << std::setw (10) << _min << " "
				<< std::left << std::setw (10) << _avg << " "
				<< std::left << std::setw (10) << _max;
			int rankSize = 0;
			int rank = 0;
			// print addtional algoritm specific info
			switch (algorithm)
			{
				case SGA:
					break;
				case NSGAII:
					// print populations size by ranks..
					while (true)
				  	{
						rankSize = schedBag->getNSGARankSize (rank);
						if (rankSize <= 0)
							break;
						std::cout << " " << std::right << std::setw (3) << rankSize;
						rank++;
					}

					break;
			}
				
			std::cout << std::endl;
		}
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
