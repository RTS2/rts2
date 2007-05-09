#include "imgdisplay.h"
#include "../utilsdb/rts2appdb.h"
#include "../utilsdb/rts2camlist.h"
#include "../utilsdb/target.h"
#include "../utilsdb/rts2obsset.h"
#include "../utils/rts2config.h"
#include "../utils/libnova_cpp.h"
#include "rts2script.h"

#include <iostream>
#include <iomanip>
#include <list>
#include <stdlib.h>

std::ostream & operator << (std::ostream & _os, struct ln_lnlat_posn *_pos)
{
  struct ln_dms dms;
  ln_deg_to_dms (_pos->lng, &dms);
  _os << std::setw (3) << std::setfill ('0') << dms.degrees << ":"
    << std::setw (2) << std::setfill ('0') << dms.minutes << ":"
    << std::setprecision (4) << dms.seconds;
  return _os;
}

double
get_norm_hour (double JD)
{
  struct ln_date tmp_date;

  ln_get_date (JD, &tmp_date);
  return
    tmp_date.hours + (double) tmp_date.minutes / 60.0 +
    (double) tmp_date.seconds / 3600.0;
}

class Rts2TargetInfo:public Rts2AppDb
{
private:
  std::list < int >targets;
  Rts2CamList cameras;
  Target *target;
  struct ln_lnlat_posn *obs;
  void printTargetInfo ();
  void printTargetInfoGNU (double jd_start, double pbeg, double pend,
			   double step);
  void printTargetInfoDS9 ();
  bool printSelectable;
  bool printExtendet;
  bool printCalTargets;
  bool printObservations;
  int printImages;
  int printCounts;
  int printGNU;
  bool printDS9;
  bool addMoon;
  char *targetType;
  virtual int printTargets (Rts2TargetSet & set);

  double JD;
public:
    Rts2TargetInfo (int argc, char **argv);
    virtual ~ Rts2TargetInfo (void);

  virtual int processOption (int in_opt);

  virtual int processArgs (const char *arg);
  virtual int init ();
  virtual int run ();
};

Rts2TargetInfo::Rts2TargetInfo (int in_argc, char **in_argv):
Rts2AppDb (in_argc, in_argv)
{
  obs = NULL;
  printSelectable = false;
  printExtendet = false;
  printCalTargets = false;
  printObservations = false;
  printImages = 0;
  printCounts = 0;
  printGNU = 0;
  printDS9 = false;
  addMoon = true;
  targetType = NULL;

  JD = ln_get_julian_from_sys ();

  addOption ('s', "selectable", 0, "print only selectable targets");
  addOption ('e', "extended", 2,
	     "print extended informations (visibility prediction,..)");
  addOption ('g', "gnuplot", 2,
	     "print in GNU plot format, optionaly followed by output type (x11 | ps | png)");
  addOption ('m', "moon", 0, "do not plot moon");
  addOption ('c', "calibartion", 0, "print recommended calibration targets");
  addOption ('o', "observations", 2,
	     "print observations (in given time range)");
  addOption ('i', "images", 2, "print images (in given time range)");
  addOption ('I', "images_summary", 0, "print image summary row");
  addOption ('l', "display filename", 0, "print image filename");
  addOption ('p', "photometer", 2, "print counts (in given format)");
  addOption ('P', "photometer_summary", 0, "print counts summary row");
  addOption ('t', "target_type", 1,
	     "search for target types, not for targets IDs");
  addOption ('d', "date", 1, "give informations for this data");
  addOption ('9', "ds9", 0, "print DS9 .reg file for target");
}

Rts2TargetInfo::~Rts2TargetInfo ()
{
  cameras.clear ();
}

int
Rts2TargetInfo::processOption (int in_opt)
{
  int ret;
  switch (in_opt)
    {
    case 's':
      printSelectable = true;
      break;
    case 'e':
      printExtendet = true;
      break;
    case 'g':
      printGNU = 1;
      if (optarg)
	{
	  if (!strcmp (optarg, "ps"))
	    printGNU = 2;
	  if (!strcmp (optarg, "png"))
	    printGNU = 3;
	  if (!strcmp (optarg, "eps"))
	    printGNU = 4;
	}
      break;
    case 'm':
      addMoon = false;
      break;
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
    case 't':
      targetType = optarg;
      break;
    case 'd':
      ret = parseDate (optarg, JD);
      if (ret)
	return ret;
      break;
    case '9':
      printDS9 = true;
      break;
    default:
      return Rts2AppDb::processOption (in_opt);
    }
  return 0;
}

int
Rts2TargetInfo::processArgs (const char *arg)
{
  // try to create that target..
  int tar_id;
  char *test;
  tar_id = strtol (arg, &test, 10);
  if (*test)
    {
      std::cerr << "Invalid target id: " << arg << std::endl;
      return -1;
    }
  targets.push_back (tar_id);
  return 0;
}

void
Rts2TargetInfo::printTargetInfo ()
{
  if (!(printImages & DISPLAY_FILENAME))
    {
      target->sendInfo (std::cout, JD);
      // print scripts..
      Rts2CamList::iterator cam_names;
      if (printExtendet)
	{
	  for (int i = 0; i < 10; i++)
	    {
	      JD += 10;

	      std::cout << "==================================" << std::
		endl << "Date: " << LibnovaDate (JD) << std::endl;
	      target->sendPositionInfo (std::cout, JD);
	    }
	}
      for (cam_names = cameras.begin (); cam_names != cameras.end ();
	   cam_names++)
	{
	  const char *cam_name = (*cam_names).c_str ();
	  int ret;
	  char script_buf[MAX_COMMAND_LENGTH];
	  int failedCount;
	  ret = target->getScript (cam_name, script_buf);
	  std::
	    cout << "Script for camera " << cam_name << ":'" << script_buf <<
	    "' ret (" << ret << ")" << std::endl;
	  // try to parse it..
	  Rts2Script script = Rts2Script (NULL, cam_name, target);
	  failedCount = script.getFaultLocation ();
	  if (failedCount != -1)
	    {
	      std::
		cout << "PARSING of script '" << script_buf <<
		"' FAILED!!! AT " << failedCount << std::endl;
	    }
	}
    }
  // print recomended calibrations targets
  if (printCalTargets)
    {
      Rts2TargetSet *cal;
      cal = target->getCalTargets (JD);
      std::cout << "==================================" << std::endl <<
	"Calibration targets" << std::endl;
      cal->print (std::cout, JD);
      delete cal;
    }
  // print observations..
  if (printObservations)
    {
      Rts2ObsSet obsSet = Rts2ObsSet (target->getTargetID ());
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
	  Rts2ImgSetTarget imgset = Rts2ImgSetTarget (target->getTargetID ());
	  imgset.load ();
	  imgset.print (std::cout, printImages);
	  imgset.clear ();
	}
    }
  return;
}

void
Rts2TargetInfo::printTargetInfoGNU (double jd_start, double pbeg, double pend,
				    double step)
{
  for (double i = pbeg; i <= pend; i += step)
    {
      std::cout << std::setw (10) << i << " ";
      target->printAltTableSingleCol (std::cout, jd_start, i, step);
      std::cout << std::endl;
    }
}

void
Rts2TargetInfo::printTargetInfoDS9 ()
{
  target->printDS9Reg (std::cout, JD);
}

int
Rts2TargetInfo::printTargets (Rts2TargetSet & set)
{
  Rts2TargetSet::iterator iter;
  struct ln_rst_time t_rst;
  struct ln_rst_time n_rst;

  double sset;
  double rise;
  double nbeg;
  double nend;

  char old_fill;
  int old_p;
  std::ios_base::fmtflags old_settings;

  if (printGNU)
    {
      ln_get_body_next_rst_horizon (JD, obs, ln_get_solar_equ_coords,
				    LN_SOLAR_CIVIL_HORIZON, &t_rst);
      ln_get_body_next_rst_horizon (JD, obs, ln_get_solar_equ_coords,
				    LN_SOLAR_NAUTIC_HORIZON, &n_rst);

      sset = get_norm_hour (t_rst.set);
      rise = get_norm_hour (t_rst.rise);
      nbeg = get_norm_hour (n_rst.set);
      nend = get_norm_hour (n_rst.rise);

      if (nbeg < sset || sset > rise)
	{
	  sset -= 24.0;
	  nbeg -= 24.0;
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
	<< "set yrange [0:90] noreverse" << std::endl
	<< "set xrange [sset:rise] noreverse" << std::endl
	<< "set xlabel \"Time UT [h]\"" << std::endl
	<< "set ylabel \"altitude\"" << std::
	endl << "set y2label \"airmass\"" << std::
	endl <<
	"set y2tics ( \"1.00\" 90, \"1.05\" 72.25, \"1.10\" 65.38, \"1.20\" 56.44, \"1.30\" 50.28 , \"1.50\" 41.81, \"2.00\" 30, \"3.00\" 20, \"6.00\" 10)"
	<< std::
	endl << "set arrow from sset,10 to rise,10 nohead lt 0" << std::
	endl << "set arrow from sset,20 to rise,20 nohead lt 0" << std::
	endl << "set arrow from sset,30 to rise,30 nohead lt 0" << std::
	endl << "set arrow from sset,41.81 to rise,41.81 nohead lt 0" << std::
	endl << "set arrow from sset,50.28 to rise,50.28 nohead lt 0" << std::
	endl << "set arrow from sset,56.44 to rise,56.44 nohead lt 0" << std::
	endl << "set arrow from sset,65.38 to rise,65.38 nohead lt 0" << std::
	endl << "set arrow from sset,72.25 to rise,72.25 nohead lt 0" << std::
	endl << "set arrow from sset,81.93 to rise,81.93 nohead lt 0" << std::
	endl << "set arrow from nbeg,0 to nbeg,90 nohead lt 0" << std::
	endl << "set arrow from nend,0 to nend,90 nohead lt 0" << std::
	endl <<
	"set arrow from (nend/2+nbeg/2),0 to (nend/2+nbeg/2),90 nohead lt 0"
	<< std::endl << "set xtics ( ";

      sset -= 1.0;
      rise += 1.0;

      for (int i = (int)floor (sset); i < (int) ceil (rise); i++)
	{
	  if (i != (int) floor (sset))
	    std::cout << ", ";
	  std::cout << '"' << (i < 0 ? i + 24 : i) << "\" " << i;
	}
      std::cout << ')' << std::endl;

      switch (printGNU)
	{
	case 2:
	  std::cout << "set terminal postscript color solid";
	  break;
	case 3:
	  std::cout << "set terminal png";
	  break;
	case 4:
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

      // find and print calibration targets..
      if (printCalTargets)
	{
	  Rts2TargetSet calibSet = Rts2TargetSet (obs, false);
	  for (iter = set.begin (); iter != set.end (); iter++)
	    {
	      target = *iter;
	      Rts2TargetSet *addS = target->getCalTargets ();
	      calibSet.merge (*addS);
	      addS->clear ();
	      delete addS;
	    }
	  set.merge (calibSet);
	}

      for (iter = set.begin (); iter != set.end (); iter++)
	{
	  target = *iter;
	  if (iter != set.begin () || addMoon)
	    std::cout << ", \\" << std::endl;
	  std::cout
	    << "     \"-\" u 1:2 smooth csplines lw 2 t \""
	    << target->getTargetName ()
	    << " (" << target->getTargetID () << ")\"";
	}
      std::cout << std::endl;
    }

  double jd_start = ((int) JD) - 0.5;
  double step = 0.2;

  if (printDS9)
    {
      std::cout << "fk5" << std::endl;
    }
  else if (printGNU && addMoon)
    {
      struct ln_hrz_posn moonHrz;
      struct ln_equ_posn moonEqu;
      for (double i = sset; i <= rise; i += step)
	{
	  double jd = jd_start + i / 24.0;
	  ln_get_lunar_equ_coords (jd, &moonEqu);
	  ln_get_hrz_from_equ (&moonEqu, obs, jd, &moonHrz);
	  std::cout
	    << i << " " << moonHrz.alt << " " << moonHrz.az << std::endl;
	}
      std::cout << "e" << std::endl;
    }

  for (iter = set.begin (); iter != set.end (); iter++)
    {
      target = *iter;
      if (printDS9)
	{
	  printTargetInfoDS9 ();
	}
      else if (printGNU)
	{
	  printTargetInfoGNU (jd_start, sset, rise, step);
	  std::cout << "e" << std::endl;
	}
      else
	{
	  printTargetInfo ();
	}
    }

  if (printGNU)
    {
      std::cout.setf (old_settings);
      std::cout.precision (old_p);
      std::cout.fill (old_fill);
    }

  return (set.size () == 0 ? -1 : 0);
}

int
Rts2TargetInfo::init ()
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

  return 0;
}

int
Rts2TargetInfo::run ()
{
  if (printSelectable)
    {
      if (targetType)
	{
	  Rts2TargetSetSelectable selSet =
	    Rts2TargetSetSelectable (targetType);
	  return printTargets (selSet);
	}
      else
	{
	  Rts2TargetSetSelectable selSet = Rts2TargetSetSelectable ();
	  return printTargets (selSet);
	}
    }
  if (targetType)
    {
      Rts2TargetSet typeSet = Rts2TargetSet (targetType);
      return printTargets (typeSet);
    }

  Rts2TargetSet tar_set = Rts2TargetSet (targets);
  return printTargets (tar_set);
}

int
main (int argc, char **argv)
{
  int ret;
  Rts2TargetInfo app = Rts2TargetInfo (argc, argv);
  ret = app.init ();
  if (ret)
    return ret;
  return app.run ();
}
