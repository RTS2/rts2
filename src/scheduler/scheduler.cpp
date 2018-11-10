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

#include "rts2db/appdb.h"
#include "rts2db/sqlerror.h"
#include "configuration.h"

#include "rts2scheduler/schedbag.h"

#define OPT_START_DATE		OPT_LOCAL + 210
#define OPT_END_DATE		OPT_LOCAL + 211

/**
 * Class of the scheduler application.  Prepares schedule, and run
 * optimalization few times to get them out..
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Rts2ScheduleApp: public rts2db::AppDb
{
	public:
		Rts2ScheduleApp (int argc, char ** argv);
		virtual ~Rts2ScheduleApp (void);

		virtual int doProcessing ();

	protected:
		virtual void usage ();
		virtual void help ();
		virtual int processOption (int _opt);
		virtual int init ();

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

		// if the programme will print detail schedule informations
		bool printSchedules;

		// if true, prints out statistics of merits of population with rank 0
		bool printMeritsStat;

		// observation night - will generate schedule for same night
		struct ln_date *obsNight;

		// start and end JD
		double startDate;
		double endDate;

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
		 * Prints schedule. This method prints all observation entries
		 * of given schedule.
		 *
		 * @param sched Schedule which will be printed.
		 */
		void printSchedule (Rts2Schedule *sched);

		/**
		 * Print objective functions for SGA.
		 */
		void printSGAMerits ();

		/**
		 * Prints objective functions of NSGAII.
		 */
		void printNSGAMerits ();

		/**
		 * Prints all merits of each member of schedule set.
		 */
		void printMerits ();

		/**
		 * Prints statistics of merits of the best population.
		 */
		void printMeritsStatistics ();
};

Rts2ScheduleApp::Rts2ScheduleApp (int argc, char ** argv): rts2db::AppDb (argc, argv)
{
	schedBag = NULL;
	verbose = 0;
	generations = 1500;
	popSize = 100;
	algorithm = NSGAII;

	printSchedules = false;

	printMeritsStat = false;

	obsNight = NULL;

	startDate = NAN;
	endDate = NAN;

	addOption ('v', NULL, 0, "verbosity level");
	addOption ('g', NULL, 1, "number of generations");
	addOption ('p', NULL, 1, "population size");
	addOption ('a', NULL, 1, "algorithm (SGA or NSGAII are currently supported, NSGAII is default)");
	addOption ('s', NULL, 0, "print schedule entries");
	addOption ('m', NULL, 0, "print merits statistics of the best population group");
	addOption ('o', NULL, 1, "do scheduling for observation set from this date");

	addOption (OPT_START_DATE, "start", 1, "produce schedule from this date");
	addOption (OPT_END_DATE, "end", 1, "produce schedule till this date");
}

Rts2ScheduleApp::~Rts2ScheduleApp (void)
{
	delete obsNight;
	delete schedBag;
}

int Rts2ScheduleApp::doProcessing ()
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

			std::cout << std::right << std::setw (5) << i << SEP
				<< std::setw (4) << schedBag->size () << SEP
				<< std::setw (10) << _min << SEP
				<< std::setw (10) << _avg << SEP
				<< std::setw (10) << _max << SEP
				<< std::setw (4) << schedBag->constraintViolation (CONSTR_VISIBILITY) << SEP
				<< std::setw (4) << schedBag->constraintViolation (CONSTR_SCHEDULE_TIME) << SEP
				<< std::setw (4) << schedBag->constraintViolation (CONSTR_UNOBSERVED_TICKETS) << SEP
				<< std::setw (4) << schedBag->constraintViolation (CONSTR_OBS_NUM);
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
						std::cout << SEP << std::right << std::setw (3) << rankSize;
						rank++;
					}

					break;
			}
				
			std::cout << std::endl;
		}
	}

	if (verbose)	
		printMerits ();
	if (printMeritsStat)
	  	printMeritsStatistics ();

	return 0;
}

void Rts2ScheduleApp::usage ()
{
	std::cout << "\t" << getAppName () << std::endl
		<< " To get schedule from 17th January 2006 01:17:18 UT to 18th January 2006 01:17:18 UT" << std::endl
		<< "\t" << getAppName () << " --start 2006-01-17T01:17:18 --end 2006-01-18T02:03:04" << std::endl;
}

void Rts2ScheduleApp::help ()
{
	std::cout << "Create schedule for given night. The programme uses genetic algorithm scheduling, described at \
http://rts2.org/scheduling.pdf." << std::endl
		<< "You are free to experiment with various settings to create optimal observing schedule" << std::endl;
	rts2db::AppDb::help ();
}

int Rts2ScheduleApp::processOption (int _opt)
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
		case 's':
			printSchedules = true;
			break;
		case 'm':
			printMeritsStat = true;
			break;
		case 'o':
			obsNight = new struct ln_date;
			return parseDate (optarg, obsNight);
		case OPT_START_DATE:
			return parseDate (optarg, startDate);
		case OPT_END_DATE:
			return parseDate (optarg, endDate);
		default:
			return rts2db::AppDb::processOption (_opt);
	}
	return 0;
}

int Rts2ScheduleApp::init ()
{
	int ret;
	ret = rts2db::AppDb::init ();
	if (ret)
		return ret;

	srandom (time (NULL));

	// initialize schedules..
	if (std::isnan (startDate))
		startDate = ln_get_julian_from_sys ();
	if (std::isnan (endDate))
	  	endDate = startDate + 0.5;
	if (startDate >= endDate)
	{
		  std::cerr << "Scheduling interval end date is behind scheduling start date, start is "
		  	<< LibnovaDate (startDate) << ", end is "
			<< LibnovaDate (endDate)
			<< std::endl;
	}

	// create list of schedules..
	if (obsNight)
	{
		std::cout << "Generating schedule for night " << LibnovaDate (obsNight) << std::endl;

		schedBag = new Rts2SchedBag (NAN, NAN);
		ret = schedBag->constructSchedulesFromObsSet (popSize, obsNight);
		if (ret)
			return ret;
	}
	else
	{
		std::cout << "Generating schedule from " << LibnovaDate (startDate) << " to " << LibnovaDate (endDate) << std::endl;

		schedBag = new Rts2SchedBag (startDate, endDate);

		ret = schedBag->constructSchedules (popSize);
		if (ret)
			return ret;
	}

	return 0;
}

void Rts2ScheduleApp::printMerits (objFunc _type, const char *name)
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
		<< min << SEP
		<< avg << SEP
		<< max << std::endl;
}

void Rts2ScheduleApp::printSchedule (Rts2Schedule *sched)
{
	int i = 1;
	for (Rts2Schedule::iterator iter_s = sched->begin (); iter_s != sched->end (); iter_s++)
	{
		std::cout << "S  " << std::setw (3) << std::right << i++ << " "
			<< *(*iter_s) << " "
			<< (*iter_s)->altitudeMerit (sched->getJDStart (), sched->getJDEnd ()) << std::endl;
	}
}

void Rts2ScheduleApp::printSGAMerits ()
{
	Rts2SchedBag::iterator iter;
	schedBag->calculateNSGARanks ();
	// print header
	std::cout << std::left
		<< std::setw (11) << "ALTITUDE" SEP
		<< std::setw (11) << "ACCOUNT" SEP
		<< std::setw (11) << "DISTANCE" SEP
		<< std::setw (11) << "VISIBILITY" SEP
		<< std::setw (4) << "DIVT" SEP
		<< std::setw (4) << "DIVO" SEP
		<< std::setw (4) << "SCH" SEP
		<< std::setw (4) << "UNT" SEP
		<< std::setw (4) << "OBN" SEP
		<< std::setw (11) << "SINGLE"
		<< std::endl;
	for (iter = schedBag->begin (); iter < schedBag->end (); iter++)
	{
		std::cout << std::left 
			<< std::setw (11) << (*iter)->getObjectiveFunction (ALTITUDE) << SEP
			<< std::setw (11) << (*iter)->getObjectiveFunction (ACCOUNT) << SEP
			<< std::setw (11) << (*iter)->getObjectiveFunction (DISTANCE) << SEP
			<< std::setw (11) << (*iter)->getObjectiveFunction (VISIBILITY) << SEP
			<< std::setw (4) << (int) ((*iter)->getObjectiveFunction (DIVERSITY_TARGET)) << SEP
			<< std::setw (4) << (int) ((*iter)->getObjectiveFunction (DIVERSITY_OBSERVATIONS)) << SEP
			<< std::setw (4) << (*iter)->getConstraintFunction (CONSTR_SCHEDULE_TIME) << SEP
			<< std::setw (4) << (*iter)->getConstraintFunction (CONSTR_UNOBSERVED_TICKETS) << SEP
			<< std::setw (4) << (*iter)->getConstraintFunction (CONSTR_OBS_NUM) << SEP
			<< std::setw (11) << (*iter)->getObjectiveFunction (SINGLE) 
			<< std::endl;
		if (printSchedules)
			printSchedule (*iter);
	}
}

void Rts2ScheduleApp::printNSGAMerits ()
{
	Rts2SchedBag::iterator iter;
	schedBag->calculateNSGARanks ();
	// print header
	std::cout << std::left << "RNK " SEP
		<< std::setw (11) << "ALTITUDE" SEP
		<< std::setw (11) << "ACCOUNT" SEP
		<< std::setw (11) << "DISTANCE" SEP
		<< std::setw (11) << "VISIBILITY" SEP
		<< std::setw (4) << "DIVT" SEP
		<< std::setw (4) << "DIVO" SEP
		<< std::setw (4) << "SCH" SEP
		<< std::setw (4) << "UNT" SEP
		<< std::setw (4) << "OBN"
		<< std::endl;
	for (iter = schedBag->begin (); iter < schedBag->end (); iter++)
	{
		std::cout << std::left << std::setw (4) << (*iter)->getNSGARank () << SEP
			<< std::setw (11) << (*iter)->getObjectiveFunction (ALTITUDE) << SEP
			<< std::setw (11) << (*iter)->getObjectiveFunction (ACCOUNT) << SEP
			<< std::setw (11) << (*iter)->getObjectiveFunction (DISTANCE) << SEP
			<< std::setw (11) << (*iter)->getObjectiveFunction (VISIBILITY) << SEP
			<< std::setw (4) << (int) ((*iter)->getObjectiveFunction (DIVERSITY_TARGET)) << SEP
			<< std::setw (4) << (int) ((*iter)->getObjectiveFunction (DIVERSITY_OBSERVATIONS)) << SEP
			<< std::setw (4) << (*iter)->getConstraintFunction (CONSTR_SCHEDULE_TIME) << SEP
			<< std::setw (4) << (*iter)->getConstraintFunction (CONSTR_UNOBSERVED_TICKETS) << SEP
			<< std::setw (4) << (*iter)->getConstraintFunction (CONSTR_OBS_NUM)
			<< std::endl; 
		if (printSchedules)
			printSchedule (*iter);
	}
}

void Rts2ScheduleApp::printMerits ()
{
	switch (algorithm)
	{
		case SGA:
			printSGAMerits ();
			break;
		case NSGAII:
			printNSGAMerits ();
			break;
	}
}

void Rts2ScheduleApp::printMeritsStatistics ()
{
	double _min, _avg, _max;
	switch (algorithm)
	{
	  	case SGA:
			schedBag->getStatistics (_min, _avg, _max);
			std::cout << "SINGLE " << _min << SEP << _avg << SEP << _max << std::endl;	
			break;
		case NSGAII:
			std::list <objFunc> obj = schedBag->getObjectives ();
			for (std::list <objFunc>::iterator objIter = obj.begin (); objIter != obj.end (); objIter++)
			{
				schedBag->getNSGAIIBestStatistics (_min, _avg, _max, *objIter);
				std::cout << getObjectiveName (*objIter) << SEP << _min << SEP << _avg << SEP << _max << std::endl;
			}
			schedBag->getNSGAIIAverageDistance (_min, _avg, _max);
			std::cout << "AVERAGE_DISTANCE" SEP << _min << SEP << _avg << SEP << _max << std::endl;
			break;
	}
}

int main (int argc, char ** argv)
{
	try
	{
		Rts2ScheduleApp app (argc, argv);
		return app.run ();
	}
	catch (rts2db::SqlError &err)
	{
		std::cerr << err << std::endl;
	}
}
