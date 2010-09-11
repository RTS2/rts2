#include "../utilsdb/rts2appdb.h"
#include "../utilsdb/plan.h"
#include "../utilsdb/planset.h"
#include "../utils/libnova_cpp.h"
#include "../utils/rts2config.h"

#include <signal.h>
#include <iostream>

#include <list>

class Rts2PlanApp:public Rts2AppDb
{
	public:
		Rts2PlanApp (int in_argc, char **in_argv);
		virtual ~ Rts2PlanApp (void);

		virtual int doProcessing ();
	protected:
		virtual int processOption (int in_opt);
	private:
		enum { NO_OP, OP_DUMP, OP_LOAD, OP_GENERATE, OP_COPY } operation;
		int dumpPlan ();
		int loadPlan ();
		int generatePlan ();
		int copyPlan ();
		double JD;
};

Rts2PlanApp::Rts2PlanApp (int in_argc, char **in_argv):Rts2AppDb (in_argc, in_argv)
{
	Rts2Config *config;
	config = Rts2Config::instance ();

	JD = ln_get_julian_from_sys ();

	operation = NO_OP;

	addOption ('n', "night", 1, "work with this night");
	addOption ('d', "dump_plan", 0, "dump plan to standart output");
	addOption ('l', "load_plan", 0, "load plan from standart input");
	addOption ('g', "generate", 0, "generate plan based on targets");
	addOption ('c', "copy_plan", 1, "copy plan to given night (from night given by -n)");
}

Rts2PlanApp::~Rts2PlanApp (void)
{
}

int Rts2PlanApp::dumpPlan ()
{
	rts2db::PlanSet *plan_set;
	Rts2Night night = Rts2Night (JD, Rts2Config::instance ()->getObserver ());
	plan_set = new rts2db::PlanSet (night.getFrom (), night.getTo ());
	std::cout << "Night " << night << std::endl;
	std::cout << (*plan_set) << std::endl;
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
	return -1;
}

int Rts2PlanApp::processOption (int in_opt)
{
	int ret;
	switch (in_opt)
	{
		case 'n':
			ret = parseDate (optarg, JD);
			if (ret)
				return ret;
			break;
		case 'd':
			if (operation != NO_OP)
				return -1;
			operation = OP_DUMP;
			break;
		case 'l':
			if (operation != NO_OP)
				return -1;
			operation = OP_LOAD;
			break;
		case 'g':
			if (operation != NO_OP)
				return -1;
			operation = OP_GENERATE;
			break;
		case 'c':
			if (operation != NO_OP)
				return -1;
			operation = OP_COPY;
			break;
		default:
			return Rts2AppDb::processOption (in_opt);
	}
	return 0;
}

int Rts2PlanApp::doProcessing ()
{
	switch (operation)
	{
		case NO_OP:
			std::cout << "Please select mode of operation.." << std::endl;
			// interactive..
			return 0;
		case OP_DUMP:
			return dumpPlan ();
		case OP_LOAD:
			return loadPlan ();
		case OP_GENERATE:
			return generatePlan ();
		case OP_COPY:
			return copyPlan ();
	}
	return -1;
}

int main (int argc, char **argv)
{
	Rts2PlanApp app = Rts2PlanApp (argc, argv);
	return app.run ();
}
