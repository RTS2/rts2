/*
 * Plan management application.
 * Copyright (C) 2005-2010 Petr Kubanek <petr@kubanek.net>
 * Copyright (C) 2011 Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
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
#include "../utilsdb/plan.h"
#include "../utilsdb/planset.h"
#include "../utils/libnova_cpp.h"
#include "../utils/rts2config.h"

#include <signal.h>
#include <iostream>

#include <list>

#define OPT_DUMP          OPT_LOCAL + 701
#define OPT_LOAD          OPT_LOCAL + 702
#define OPT_GENERATE      OPT_LOCAL + 703
#define OPT_COPY          OPT_LOCAL + 704
#define OPT_DELETE        OPT_LOCAL + 705
#define OPT_ADD           OPT_LOCAL + 706
#define OPT_DUMP_TARGET   OPT_LOCAL + 707

class Rts2PlanApp:public Rts2AppDb
{
	public:
		Rts2PlanApp (int in_argc, char **in_argv);
		virtual ~ Rts2PlanApp (void);

		virtual int doProcessing ();
	protected:
		virtual int processOption (int in_opt);
		virtual int processArgs (const char *arg);

		virtual void usage ();
	private:
		enum { NO_OP, OP_ADD, OP_DUMP, OP_LOAD, OP_GENERATE, OP_COPY, OP_DELETE, OP_DUMP_TARGET } operation;
		int addPlan ();
		void doAddPlan (rts2db::Plan *addedplan);
		int dumpPlan ();
		int loadPlan ();
		int generatePlan ();
		int copyPlan ();
		int deletePlan ();
		int dumpTargetPlan ();

		double parsePlanDate (const char *arg, double base);

		double JD;
		std::vector <const char *> args;
};

Rts2PlanApp::Rts2PlanApp (int in_argc, char **in_argv):Rts2AppDb (in_argc, in_argv)
{
	JD = rts2_nan ("f");

	operation = NO_OP;

	addOption (OPT_ADD, "add", 0, "add plan entry for target (specified as an argument)");
	addOption ('n', NULL, 1, "work with this night");
	addOption (OPT_DUMP, "dump", 0, "dump plan to standart output");
	addOption (OPT_LOAD, "load", 0, "load plan from standart input");
	addOption (OPT_GENERATE, "generate", 0, "generate plan based on targets");
	addOption (OPT_COPY, "copy", 0, "copy plan to given night (from night given by -n)");


	addOption (OPT_DELETE, "delete", 1, "delete plan with plan ID given as parameter");
	addOption (OPT_DUMP_TARGET, "target", 0, "dump plan for given target");
}

Rts2PlanApp::~Rts2PlanApp (void)
{
}

int Rts2PlanApp::addPlan ()
{
	rts2db::Plan *addedplan = NULL;
	for (std::vector <const char *>::iterator iter = args.begin (); iter != args.end (); iter++)
	{
		// check if new plan was fully specifed and can be saved
		if (addedplan && !isnan (addedplan->getPlanStart ()) && !isnan (addedplan->getPlanEnd ()))
		{
			doAddPlan (addedplan);  
			delete addedplan;
			addedplan = NULL;
		}
		
		// create new target if none existed..	
		if (!addedplan)
		{
			try
			{
				rts2db::TargetSet tar_set;
				tar_set.load (*iter);
				if (tar_set.empty ())
				{
					std::cerr << "cannot load target with name/ID" << *iter << std::endl;
					return -1;
				}
				if (tar_set.size () != 1)
				{
					std::cerr << "multiple targets matched " << *iter << ", please be more restrictive" << std::endl;
					return -1;
				}
				addedplan = new rts2db::Plan ();
				addedplan->setTargetId (tar_set.begin ()->second->getTargetID ());
			}
			catch (rts2core::Error er)
			{
				std::cerr << er << std::endl;
				return -1;
			}
		}
		else if (isnan (addedplan->getPlanStart ()))
		{
		  	addedplan->setPlanStart (parsePlanDate (*iter, getNow ()));
		}
		else
		{
			addedplan->setPlanEnd (parsePlanDate (*iter, addedplan->getPlanStart ()));
		}
	}

	if (addedplan && !isnan (addedplan->getPlanStart ()))
	{
		doAddPlan (addedplan);
		delete addedplan;
		return 0;
	}
	delete addedplan;
	return -1;
}

void Rts2PlanApp::doAddPlan (rts2db::Plan *addedplan)
{
	int ret = addedplan->save ();
	if (ret)
	{
		std::ostringstream os;
		os << "cannot save plan " << addedplan->getTarget ()->getTargetName () << " (#" << addedplan->getTargetId () << ")";
		throw rts2core::Error (os.str ());
	}
	std::cout << addedplan << std::endl;
}


int Rts2PlanApp::dumpPlan ()
{
	time_t from;
	time_t to;

	if (isnan (JD))
	{
		from = Rts2Config::instance ()->getNight ();
		to = from + 86400;
	}
	else
	{
		Rts2Night night = Rts2Night (JD, Rts2Config::instance ()->getObserver ());
		from = *(night.getFrom ());
		to = *(night.getTo ());
	}
	rts2db::PlanSet *plan_set = new rts2db::PlanSet (&from, &to);
	plan_set->load ();
	std::cout << "Plan entries from " << Timestamp (from) << " to " << Timestamp (to) << std::endl << (*plan_set) << std::endl;
	delete plan_set;
	return 0;
}

int Rts2PlanApp::loadPlan ()
{
	while (1)
	{
		rts2db::Plan plan;
		int ret;
		std::cin >> plan;
		if (std::cin.fail ())
			break;
		ret = plan.save ();
		if (ret)
			std::cerr << "Error loading plan" << std::endl;
	}
	return 0;
}

int Rts2PlanApp::generatePlan ()
{
	return -1;
}

int Rts2PlanApp::copyPlan ()
{
	if (isnan (JD))
	{
	  	std::cerr << "you must specify source night with -n" << std::endl;
		return -1;
	}

	if (args.empty ())
	{
		std::cerr << "you must provide at least one argument for target night" << std::endl;
		return -1;
	}

	// load source plans..
	Rts2Night night = Rts2Night (JD, Rts2Config::instance ()->getObserver ());
	time_t from = *(night.getFrom ());
	time_t to = *(night.getTo ());

	rts2db::PlanSet plan_set (&from, &to);
	plan_set.load ();

	if (plan_set.empty ())
	{
		std::cerr << "Empy set for dates from " << Timestamp (from) << " to " << Timestamp (to) << std::endl;
		return -1;
	}

	for (std::vector <const char *>::iterator iter = args.begin (); iter != args.end (); iter++)
	{
		 double jd2;
		 int ret = parseDate (*iter, jd2);
		 if (ret)
		 {
		  	 std::cerr << "cannot parse date " << *iter << std::endl;
			 return -1;
		 }
		 double d = (jd2 - JD) * 86400;
		 for (rts2db::PlanSet::iterator pi = plan_set.begin (); pi != plan_set.end (); pi++)
		 {
			rts2db::Plan np;
			np.setTargetId (pi->getTargetId ());
			np.setPlanStart (pi->getPlanStart () + d);
			if (!isnan (pi->getPlanEnd ()))
				np.setPlanEnd (pi->getPlanEnd () + d);
			ret = np.save ();
			if (ret)
			{
				std::cerr << "error saving " << std::endl << (&np) << std::endl << ", exiting" << std::endl;
				return -1;
			}
		 }
		 std::cout << "copied " << plan_set.size () << " plan entries to " << *iter << std::endl;
	}
	return 0;
}

int Rts2PlanApp::deletePlan ()
{
	for (std::vector <const char *>::iterator iter = args.begin (); iter != args.end (); iter++)
	{
		int id = atoi (*iter);
		rts2db::Plan p (id);
		if (p.del ())
		{
			logStream (MESSAGE_ERROR) << "cannot delete plan with ID " << (*iter) << sendLog;
			return -1;	
		}
	}
	return 0;
}

int Rts2PlanApp::dumpTargetPlan ()
{
	for (std::vector <const char *>::iterator iter = args.begin (); iter != args.end (); iter++)
	{
		rts2db::TargetSet ts;
		ts.load (*iter);
		for (rts2db::TargetSet::iterator tsi = ts.begin (); tsi != ts.end (); tsi++)
		{
			rts2db::PlanSetTarget ps (tsi->second->getTargetID ());
			ps.load ();
			tsi->second->printShortInfo (std::cout);
			std::cout << std::endl << ps << std::endl;
		}
	}
	return 0;
}

double Rts2PlanApp::parsePlanDate (const char *arg, double base)
{
	if (arg[0] == '+')
		return base + atof (arg);
	double d;
	parseDate (arg, d);
	time_t t;
	ln_get_timet_from_julian (d, &t);
	return t;
}

int Rts2PlanApp::processOption (int in_opt)
{
	int ret;
	switch (in_opt)
	{
		case OPT_ADD:
			if (operation != NO_OP)
				return -1;
			operation = OP_ADD;
			break;
		case 'n':
			ret = parseDate (optarg, JD);
			if (ret)
				return ret;
			break;
		case OPT_DUMP:
			if (operation != NO_OP)
				return -1;
			operation = OP_DUMP;
			break;
		case OPT_LOAD:
			if (operation != NO_OP)
				return -1;
			operation = OP_LOAD;
			break;
		case OPT_GENERATE:
			if (operation != NO_OP)
				return -1;
			operation = OP_GENERATE;
			break;
		case OPT_COPY:
			if (operation != NO_OP)
				return -1;
			operation = OP_COPY;
			break;
		case OPT_DELETE:
			if (operation != NO_OP)
				return -1;
			operation = OP_DELETE;
			break;
		case OPT_DUMP_TARGET:
			if (operation != NO_OP)
				return -1;
			operation = OP_DUMP_TARGET;
			break;
		default:
			return Rts2AppDb::processOption (in_opt);
	}
	return 0;
}

int Rts2PlanApp::processArgs (const char *arg)
{
	if (operation != OP_ADD && operation != OP_DUMP_TARGET && operation != OP_COPY && operation != OP_DELETE)
		return Rts2AppDb::processArgs (arg);
	args.push_back (arg);
	return 0;
}

void Rts2PlanApp::usage ()
{
	std::cout << "To add new entry for target 1001 to plan schedule, with observations starting on 2011-01-24 20:00;00 UT, and running for 5 minutes" << std::endl
		<< "  " << getAppName () << " --add 1001 2011-01-24T20:00:00 +300" << std::endl
		<< std::endl;
}

int Rts2PlanApp::doProcessing ()
{
	switch (operation)
	{
		case NO_OP:
			std::cout << "Please select mode of operation.." << std::endl;
			// interactive..
			return 0;
		case OP_ADD:
			return addPlan ();
		case OP_DUMP:
			return dumpPlan ();
		case OP_LOAD:
			return loadPlan ();
		case OP_GENERATE:
			return generatePlan ();
		case OP_COPY:
			return copyPlan ();
		case OP_DELETE:
			return deletePlan ();
		case OP_DUMP_TARGET:
			return dumpTargetPlan ();
	}
	return -1;
}

int main (int argc, char **argv)
{
	Rts2PlanApp app = Rts2PlanApp (argc, argv);
	return app.run ();
}
