/* 
 * Prints informations about target.
 * Copyright (C) 2010 Petr Kubanek <petr@kubanek.net>
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
#include "../utils/utilsfunc.h"

#define OPT_FULL_DAY              OPT_LOCAL + 200
#define OPT_NAME                  OPT_LOCAL + 201
#define OPT_PARSE_SCRIPT          OPT_LOCAL + 202
#define OPT_CHECK_CONSTRAINTS     OPT_LOCAL + 203
#define OPT_AIRMASS               OPT_LOCAL + 204

std::ostream & operator << (std::ostream & _os, struct ln_lnlat_posn *_pos)
{
	struct ln_dms dms;
	ln_deg_to_dms (_pos->lng, &dms);
	_os << std::setw (3) << std::setfill ('0') << dms.degrees << ":"
		<< std::setw (2) << std::setfill ('0') << dms.minutes << ":"
		<< std::setprecision (4) << dms.seconds;
	return _os;
}

double get_norm_hour (double JD)
{
	struct ln_date tmp_date;

	ln_get_date (JD, &tmp_date);
	return
		tmp_date.hours + (double) tmp_date.minutes / 60.0 +
		(double) tmp_date.seconds / 3600.0;
}

#define GNUPLOT_TYPE_MASK   0x0f
#define GNUPLOT_TYPE_X11    0x01
#define GNUPLOT_TYPE_PS     0x02
#define GNUPLOT_TYPE_PNG    0x03
#define GNUPLOT_TYPE_EPS    0x04

#define GNUPLOT_BONUS       0x10
#define GNUPLOT_BONUS_ONLY  0x20
#define GNUPLOT_FULL_DAY    0x40

using namespace rts2plan;

PrintTarget::PrintTarget (int in_argc, char **in_argv):Rts2AppDb (in_argc, in_argv)
{
	obs = NULL;
	printExtended = 0;
	printCalTargets = false;
	printObservations = false;
	printImages = 0;
	printCounts = 0;
	printGNUplot = 0;
	printAuger = false;
	printDS9 = false;
	addMoon = true;
	addHorizon = true;
	
	JD = ln_get_julian_from_sys ();

	airmd = rts2_nan ("f");

	constraintFile = NULL;

	addOption ('e', NULL, 0, "print extended informations (visibility prediction,..)");
	addOption ('g', NULL, 2, "print in GNU plot format, optionaly followed by output type (x11 | ps | png)");
	addOption ('b', NULL, 0, "gnuplot bonus of the target");
	addOption ('B', NULL, 0, "gnuplot bonus and altitude of the target");
	addOption ('m', NULL, 0, "do not plot moon");
	addOption (OPT_AIRMASS, "airmass", 1, "specify airmass distance for calibration targets selection (and print calibration targets)");
	addOption ('c', NULL, 0, "print recommended calibration targets");
	addOption ('o', NULL, 2, "print observations (in given time range)");
	addOption ('i', NULL, 2, "print images (in given time range)");
	addOption ('I', NULL, 0, "print images summary row");
	addOption ('l', NULL, 0, "list images filenames");
	addOption ('p', NULL, 2, "print counts (in given format)");
	addOption ('P', NULL, 0, "print counts summary row");
	addOption ('d', NULL, 1, "give informations for this date");
	addOption (OPT_FULL_DAY, "full-day", 0, "prints informations for 24 hours");
	addOption ('9', NULL, 0, "print DS9 .reg file for target");
	addOption ('N', NULL, 0, "do not pretty print");
	addOption (OPT_NAME, "name", 0, "print target(s) name(s)");
	addOption (OPT_PARSE_SCRIPT, "parse", 1, "pretty print parsed script for given camera");
	addOption (OPT_CHECK_CONSTRAINTS, "constraints", 1, "check targets agains constraint file");
}

PrintTarget::~PrintTarget ()
{
	cameras.clear ();
}

int PrintTarget::processOption (int in_opt)
{
	int ret;
	switch (in_opt)
	{
		case 'e':
			printExtended++;
			break;
		case 'g':
			if (optarg)
			{
				if (!strcmp (optarg, "ps"))
					printGNUplot |= GNUPLOT_TYPE_PS;
				if (!strcmp (optarg, "png"))
					printGNUplot |= GNUPLOT_TYPE_PNG;
				if (!strcmp (optarg, "eps"))
					printGNUplot |= GNUPLOT_TYPE_EPS;
				if (!strcmp (optarg, "x11"))
					printGNUplot |= GNUPLOT_TYPE_X11;
			}
			else
			{
				printGNUplot |= GNUPLOT_TYPE_X11;
			}
			break;
		case 'b':
			printGNUplot |= GNUPLOT_BONUS_ONLY;
			break;
		case 'B':
			printGNUplot |= GNUPLOT_BONUS;
			break;
		case 'm':
			addMoon = false;
			break;
		case OPT_AIRMASS:
			airmd = atof (optarg);
		case 'c':
			printCalTargets = true;
			break;
		case 'o':
			printObservations = true;
			break;
		case 'i':
			printImages |= DISPLAY_ALL;
			break;
		case 'I':
			printImages |= DISPLAY_SUMMARY;
			break;
		case 'l':
			printImages |= DISPLAY_FILENAME;
			break;
		case 'p':
			if (optarg)
			{
				if (!strcmp (optarg, "txt"))
					printCounts = DISPLAY_SHORT;
				else
					printCounts |= DISPLAY_ALL;
			}
			else
				printCounts |= DISPLAY_ALL;
			break;
		case 'P':
			printCounts |= DISPLAY_SUMMARY;
			break;
		case OPT_FULL_DAY:
			printGNUplot |= GNUPLOT_FULL_DAY;
			break;
		case 'd':
			ret = parseDate (optarg, JD);
			if (ret)
				return ret;
			break;
		case '9':
			printDS9 = true;
			break;
		case 'N':
			std::cout << pureNumbers;
			break;
		case OPT_NAME:
			printExtended = -1;
			break;
		case OPT_PARSE_SCRIPT:
			printExtended = -2;
			scriptCameras.push_back (std::string (optarg));
			break;
		case OPT_CHECK_CONSTRAINTS:
			constraintFile = optarg;
			break;
		default:
			return Rts2AppDb::processOption (in_opt);
	}
	return 0;
}

void PrintTarget::printScripts (rts2db::Target *target, const char *pref)
{
	Rts2CamList::iterator cam_names;
	for (cam_names = cameras.begin (); cam_names != cameras.end (); cam_names++)
	{
		const char *cam_name = (*cam_names).c_str ();
		std::string script_buf;
		int failedCount;
		std::cout << pref << cam_name << ":'";
		try
		{
			target->getScript (cam_name, script_buf);
			std::cout << script_buf << '\'' << std::endl;
		}
		catch (rts2core::Error &er)
		{
			std::cout << " error : " << er << std::endl;
		}
		// try to parse it..
		rts2script::Script script = rts2script::Script ();
		script.setTarget (cam_name, target);
		failedCount = script.getFaultLocation ();
		if (failedCount != -1)
		{
			std::cout << "PARSING of script '" << script_buf << "' FAILED!!! AT " << failedCount << std::endl
				<< script.getWholeScript ().substr (0, failedCount + 1) << std::endl;
			for (; failedCount > 0; failedCount--)
				std::cout << " ";
			std::cout << "^ here" << std::endl;
		}
	}
}

void PrintTarget::printTarget (rts2db::Target *target)
{
	if (printDS9)
	{
		printTargetDS9 (target);
	}
	else if (printGNUplot)
	{
		if (!(printGNUplot & GNUPLOT_BONUS_ONLY))
		{
			printTargetGNUplot (target);
			std::cout << "e" << std::endl;
		}
		if ((printGNUplot & GNUPLOT_BONUS)
			|| (printGNUplot & GNUPLOT_BONUS_ONLY))
		{
			printTargetGNUBonus (target);
			std::cout << "e" << std::endl;
		}
	}
	else
	{
	  	// default print of the target
		if (!(printImages & DISPLAY_FILENAME))
		{
			if (printExtended == -2)
			{
				for (std::vector <std::string>::iterator iter = scriptCameras.begin (); iter != scriptCameras.end (); iter++)
				{
					std::string script_buf;
					// parse and pretty print script
					rts2script::Script script = rts2script::Script ();
					script.setTarget ((*iter).c_str (), target);
					script.prettyPrint (std::cout, rts2script::PRINT_XML);
				}
			}
			else if (printExtended == -1)
			{
				std::cout << target->getTargetName () << std::endl;

			}
			else if (printExtended == 0)
			{
				target->printShortInfo (std::cout, JD);
				std::cout << std::endl;
				printScripts (target, "       ");
			}
			else
			{
				Rts2InfoValOStream ivos (&std::cout);
				target->sendInfo (ivos, JD);
				// print scripts..
				if (printExtended > 1)
				{
					for (int i = 0; i < 10; i++)
					{
						JD += 10;
	
						std::cout << "==================================" << std::
							endl << "Date: " << LibnovaDate (JD) << std::endl;
						target->sendPositionInfo (ivos, JD);
					}
				}
				printScripts (target, "script for camera ");
			}
		}
		// test and print constraints
		if (constraintFile && constraints)
		{
			std::cout << "satisfy constraints in " << constraintFile << " " << constraints->satisfy (target, JD) << std::endl;
			std::list <std::string> vn;
			constraints->violated (target, JD, vn);
			std::cout << "number of violations " << vn.size ();
			if (vn.size () > 0)
			{
				std::cout << " - violated in ";
				for (std::list <std::string>::iterator iter = vn.begin (); iter != vn.end (); iter++)
				{
					if (iter != vn.begin ())
						std::cout << ", ";
					std::cout << *iter;
				}
			}
			std::cout << std::endl;
			
		}
		// print recomended calibrations targets
		if (printCalTargets)
		{
			rts2db::TargetSet *cal;
			cal = target->getCalTargets (JD, airmd);
			std::cout << "==================================" << std::endl <<
				"Calibration targets" << std::endl;
			cal->print (std::cout, JD);
			delete cal;
		}
		// print observations..
		if (printObservations)
		{
			rts2db::ObservationSet obsSet = rts2db::ObservationSet ();
			obsSet.loadTarget (target->getTargetID ());
			if (printImages)
				obsSet.printImages (printImages);
			if (printCounts)
				obsSet.printCounts (printCounts);
			std::cout << obsSet << std::endl;
		}
		else if (printImages || printCounts)
		{
			if (printImages)
			{
				rts2db::ImageSetTarget imgset = rts2db::ImageSetTarget (target->getTargetID ());
				imgset.load ();
				imgset.print (std::cout, printImages);
				imgset.clear ();
			}
		}
	}
}

void PrintTarget::printTargetGNUplot (rts2db::Target *target)
{
	for (double i = gbeg; i <= gend; i += step)
	{
		std::cout << i << " ";
		target->printAltTableSingleCol (std::cout, jd_start, i, step);
		std::cout << std::endl;
	}
}

void PrintTarget::printTargetGNUBonus (rts2db::Target *target)
{
	for (double i = gbeg; i <= gend; i += step)
	{
		std::cout << i << " " << target->getBonus (jd_start + i / 24.0) << std::endl;
	}
}

void PrintTarget::printTargetDS9 (rts2db::Target *target)
{
	target->printDS9Reg (std::cout, JD);
}

int PrintTarget::printTargets (rts2db::TargetSet & set)
{
	rts2db::TargetSet::iterator iter;
	struct ln_rst_time t_rst;
	struct ln_rst_time n_rst;

	// sun set and rise times (hours)
	double sset, rise;

	// night begin and ends (hours)
	double nbeg, nend;

	char old_fill;
	int old_p;
	std::ios_base::fmtflags old_settings;

	// if there is more then one target, we will not print horizont - it's useless
	if (set.size () > 1)
		addHorizon = false;

	if (printGNUplot)
	{
		ln_get_body_next_rst_horizon (JD, obs, ln_get_solar_equ_coords, LN_SOLAR_CIVIL_HORIZON, &t_rst);
		ln_get_body_next_rst_horizon (JD, obs, ln_get_solar_equ_coords, LN_SOLAR_NAUTIC_HORIZON, &n_rst);

		sset = get_norm_hour (t_rst.set);
		rise = get_norm_hour (t_rst.rise);
		nbeg = get_norm_hour (n_rst.set);
		nend = get_norm_hour (n_rst.rise);

		if (nbeg < sset || sset > rise)
		{
			sset -= 24.0;
			nbeg -= 24.0;
		}

		if (nbeg < sset)
		{
			nbeg += 24.0;
		}

		if (rise < nend)
		{
			rise += 24.0;
		}

		if (printGNUplot & GNUPLOT_FULL_DAY)
		{
			gbeg = get_norm_hour (t_rst.transit) - 12.0;
			gend = get_norm_hour (t_rst.transit) + 12.0;
		}
		else
		{
			gbeg = sset - 1.0;
			gend = rise + 1.0;
		}

		old_fill = std::cout.fill ('0');
		old_p = std::cout.precision (7);
		old_settings = std::cout.flags ();
		std::cout.setf (std::ios_base::fixed, std::ios_base::floatfield);

		std::cout
			<< "sset=" << sset << std::endl
			<< "rise=" << rise << std::endl
			<< "nend=" << nend << std::endl
			<< "nbeg=" << nbeg << std::endl
			<< "gbeg=" << gbeg << std::endl
			<< "gend=" << gend << std::endl
			<< "set xrange [gbeg:gend] noreverse" << std::endl
			<< "set xlabel \"Time UT [h]\"" << std::endl;
		if (printGNUplot & GNUPLOT_BONUS_ONLY)
		{
			std::cout << "set ylabel \"bonus\"" << std::endl
				<< "set yrange [0:]" << std::endl << "set ytics" << std::endl;
		}
		else
		{
			std::cout
				<< "set yrange [0:90] noreverse" << std::endl
				<< "set ylabel \"altitude\"" << std::endl;
		}
		if (printGNUplot & GNUPLOT_BONUS)
		{
			std::cout
				<< "set y2label \"bonus\"" << std::endl
				<< "set y2range [0:]" << std::endl << "set y2tics" << std::endl;
		}
		else if (!(printGNUplot & GNUPLOT_BONUS_ONLY))
		{
			std::cout
				<< "set y2label \"airmass\"" << std::endl
				<< "set y2tics ( \"1.00\" 90, \"1.05\" 72.25, \"1.10\" 65.38, \"1.20\" 56.44, \"1.30\" 50.28 , \"1.50\" 41.81, \"2.00\" 30, \"3.00\" 20, \"6.00\" 10)" << std::endl;
		}

		if (!(printGNUplot & GNUPLOT_BONUS_ONLY))
		{
			std::cout
				<< "set arrow from sset,10 to rise,10 nohead lt 0" << std::endl
				<< "set arrow from sset,20 to rise,20 nohead lt 0" << std::endl
				<< "set arrow from sset,30 to rise,30 nohead lt 0" << std::endl
				<< "set arrow from sset,41.81 to rise,41.81 nohead lt 0" << std::endl
				<< "set arrow from sset,50.28 to rise,50.28 nohead lt 0" << std::endl
				<< "set arrow from sset,56.44 to rise,56.44 nohead lt 0" << std::endl
				<< "set arrow from sset,65.38 to rise,65.38 nohead lt 0" << std::endl
				<< "set arrow from sset,72.25 to rise,72.25 nohead lt 0" << std::endl
				<< "set arrow from sset,81.93 to rise,81.93 nohead lt 0" << std::endl;
		}

		std::cout
			<< "set arrow from nbeg,graph 0 to nbeg,graph 1 nohead lt 0" << std::endl
			<< "set arrow from nend,graph 0 to nend,graph 1 nohead lt 0" << std::endl
			<< "set arrow from (nend/2+nbeg/2),graph 0 to (nend/2+nbeg/2),graph 1 nohead lt 0" << std::endl
			<< "set xtics ( ";

		for (int i = (int)floor (gbeg); i < (int) ceil (gend); i++)
		{
			if (i != (int) floor (gbeg))
				std::cout << ", ";
			std::cout << '"' << (i < 0 ? i + 24 : (i > 24) ? i - 24 : i) << "\" " << i;
		}
		std::cout << ')' << std::endl;

		switch (printGNUplot & GNUPLOT_TYPE_MASK)
		{
			case GNUPLOT_TYPE_PS:
				std::cout << "set terminal postscript color solid";
				break;
			case GNUPLOT_TYPE_PNG:
				std::cout << "set terminal png";
				break;
			case GNUPLOT_TYPE_EPS:
				std::cout << "set terminal postscript eps color solid";
				break;
			default:
				std::cout << "set terminal x11 persist";
		}
		std::cout << std::endl << "plot \\" << std::endl;

		if (addMoon)
		{
			std::
				cout << "     \"-\" u 1:2 smooth csplines lt 0 lw 3 t \"Moon\"";
		}
		if (addHorizon)
		{
			if (addMoon)
				std::cout << ", \\" << std::endl;
			std::
				cout <<
				"     \"-\" u 1:2 smooth csplines lt 2 lw 3 t \"Horizon\"";
		}

		rts2db::Target *target;

		// find and print calibration targets..
		if (printCalTargets)
		{
			rts2db::TargetSet calibSet = rts2db::TargetSet (obs);
			for (iter = set.begin (); iter != set.end (); iter++)
			{
				target = (*iter).second;
				rts2db::TargetSet *addS = target->getCalTargets ();
				calibSet.addSet (*addS);
				delete addS;
			}
			set.addSet (calibSet);
		}

		for (iter = set.begin (); iter != set.end (); iter++)
		{
			target = (*iter).second;
			if (!(printGNUplot & GNUPLOT_BONUS_ONLY))
			{
				if (iter != set.begin () || addMoon || addHorizon)
					std::cout << ", \\" << std::endl;

				std::cout
					<< "     \"-\" u 1:2 smooth csplines lw 2 t \""
					<< target->getTargetName ()
					<< " (" << target->getTargetID () << ")\"";
			}
			if ((printGNUplot & GNUPLOT_BONUS) || (printGNUplot & GNUPLOT_BONUS_ONLY))
			{
				if (iter != set.begin () || addMoon || addHorizon
					|| !(printGNUplot & GNUPLOT_BONUS_ONLY))
					std::cout << ", \\" << std::endl;
				std::cout
					<< "     \"-\" u 1:2 smooth csplines lw 2 t \"bonus for "
					<< target->getTargetName () << " (" << target->getTargetID ()
					<< ")\"";
				if (!(printGNUplot & GNUPLOT_BONUS_ONLY))
					std::cout << " axes x1y2";
			}
		}
		std::cout << std::endl;
	}

	jd_start = ((int) JD) - 0.5;
	step = 0.2;

	if (printDS9)
	{
		std::cout << "fk5" << std::endl;
	}
	else if (printGNUplot)
	{
		if (addMoon)
		{
			struct ln_hrz_posn moonHrz;
			struct ln_equ_posn moonEqu;
			for (double i = gbeg; i <= gend; i += step)
			{
				double jd = jd_start + i / 24.0;
				ln_get_lunar_equ_coords (jd, &moonEqu);
				ln_get_hrz_from_equ (&moonEqu, obs, jd, &moonHrz);
				std::cout << i << " " << moonHrz.alt << " " << moonHrz.az << std::endl;
			}
			std::cout << "e" << std::endl;
		}
		if (addHorizon)
		{
			struct ln_hrz_posn hor;
			for (double i = gbeg; i <= gend; i += step)
			{
				double jd = jd_start + i / 24.0;
				((*(set.begin ())).second)->getAltAz (&hor, jd);
				std::cout << i << " " << Rts2Config::instance ()->getObjectChecker ()->getHorizonHeight (&hor, 0) << " " << hor.az << std::endl;
			}
			std::cout << "e" << std::endl;
		}
	}

	for (iter = set.begin (); iter != set.end (); iter++)
	{
		rts2db::Target *target = (*iter).second;
		printTarget (target);
	}

	if (printGNUplot)
	{
		std::cout.setf (old_settings);
		std::cout.precision (old_p);
		std::cout.fill (old_fill);
	}

	return (set.size () == 0 ? -1 : 0);
}

int PrintTarget::init ()
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

	ret = cameras.load ();
	if (ret)
		return -1;

	if (constraintFile)
	{
		constraints = new rts2db::Constraints ();
		constraints->load (constraintFile);
	}

	return 0;
}
