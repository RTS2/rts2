#include "rts2targetapp.h"
#include "simbad/rts2simbadtarget.h"
#include "../utils/rts2config.h"
#include "../utils/libnova_cpp.h"

#include <sstream>
#include <iostream>

Rts2TargetApp::Rts2TargetApp (int in_argc, char **in_argv):
Rts2AppDb (in_argc, in_argv)
{
	target = NULL;
	obs = NULL;
}


Rts2TargetApp::~Rts2TargetApp (void)
{
	delete target;
}


int
Rts2TargetApp::askForDegrees (const char *desc, double &val)
{
	LibnovaDegDist degDist = LibnovaDegDist (val);
	while (1)
	{
		std::cout << desc << " [" << degDist << "]: ";
		std::cin >> degDist;
		if (!std::cin.fail ())
			break;
		std::cout << "Invalid string!" << std::endl;
		std::cin.clear ();
		std::cin.ignore (2000, '\n');
	}
	val = degDist.getDeg ();
	std::cout << desc << ": " << degDist << std::endl;
	return 0;
}


int
Rts2TargetApp::askForObject (const char *desc, std::string obj_text)
{
	int ret;
	if (obj_text.length () == 0)
	{
		ret = askForString (desc, obj_text);
		if (ret)
			return ret;
	}
	std::istringstream is (obj_text);
	// now try to parse it to ra dec..
	double ra = 0;
	double dec = 0;
	int dec_sign = 1;
	int step = 1;
	enum
	{ NOT_GET, RA, DEC }
	state;
	bool ra_six = false;

	state = NOT_GET;
	while (1)
	{
		std::string next;
		is >> next;
		if (is.fail ())
			break;
		std::istringstream next_num (next);
		int next_c = next_num.peek ();
		if (next_c == '+' || next_c == '-')
		{
			if (state != RA)
				return -1;
			state = DEC;
			step = 1;
			if (next_c == '-')
			{
				dec_sign = -1;
				next_num.get ();
			}
		}
		double val;
		next_num >> val;
		if (next_num.fail ())
			break;
		switch (state)
		{
			case NOT_GET:
				state = RA;
			case RA:
				ra += (val / step);
				if (step > 1)
					ra_six = true;
				step *= 60;
				break;
			case DEC:
				dec += (val / step);
				step *= 60;
				break;
		}
	}
	if (state == DEC)
	{
		// convert ra from hours to degs
		// if ra is one number, then it's already in degrees
		double obj_ra = ra;
		if (ra_six)
			obj_ra *= 15.0;
		double obj_dec = dec_sign * dec;
		std::string new_prefix;

		// default prefix for new RTS2 sources
		new_prefix = std::string ("RTS2");

		// set name..
		ConstTarget *constTarget = new ConstTarget ();
		constTarget->setPosition (obj_ra, obj_dec);
		std::ostringstream os;

		Rts2Config::instance ()->getString ("newtarget", "prefix", new_prefix);
		os << new_prefix << LibnovaRaComp (obj_ra) <<
			LibnovaDeg90Comp (obj_dec);
		constTarget->setTargetName (os.str ().c_str ());
		target = constTarget;
		return 0;
	}
	// try to get target from SIMBAD
	//  target = new Rts2SimbadTarget (obj_text.c_str ());
	target = new Rts2SimbadTarget (obj_text.c_str ());
	ret = target->load ();
	if (ret)
	{
		delete target;
		target = NULL;
		return -1;
	}
	return 0;
}


int
Rts2TargetApp::init ()
{
	int ret;

	ret = Rts2AppDb::init ();
	if (ret)
		return ret;

	Rts2Config *config;
	config = Rts2Config::instance ();

	if (!obs)
	{
		obs = config->getObserver ();
	}

	return 0;
}
