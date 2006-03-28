#include "model/telmodel.h"
#include "../utils/libnova_cpp.h"

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

class Rts2DevTelescopeModelTest:public Rts2DevTelescope
{
public:
  Rts2DevTelescopeModelTest ():Rts2DevTelescope (0, NULL)
  {
    telLongtitude = Rts2Config::instance ()->getObserver ()->lng;
    telLatitude = Rts2Config::instance ()->getObserver ()->lat;
  }

  void setObserverLat (double in_lat)
  {
    telLatitude = in_lat;
  }
};

class TelModelTest:public Rts2App
{
private:
  char *modelFile;
  Rts2TelModel *model;
    std::vector < std::string > runFiles;
  Rts2DevTelescopeModelTest *telescope;
  int verbose;

  void test (double ra, double dec);
  void runOnFile (std::string filename, std::ostream & os);
protected:
    virtual int processOption (int in_opt);
  virtual int processArgs (const char *arg);

public:
    TelModelTest (int in_argc, char **in_argv);
    virtual ~ TelModelTest (void);

  virtual int init ();
  virtual int run ();
};

TelModelTest::TelModelTest (int in_argc, char **in_argv):
Rts2App (in_argc, in_argv)
{
  Rts2Config *config;
  config = Rts2Config::instance ();
  modelFile = NULL;
  model = NULL;
  verbose = 0;
  addOption ('m', "model-file", 1, "Model file to use");
  addOption ('v', "verbose", 0, "Report model progress");
}

TelModelTest::~TelModelTest (void)
{
  delete telescope;
  runFiles.clear ();
}

int
TelModelTest::processOption (int in_opt)
{
  switch (in_opt)
    {
    case 'm':
      modelFile = optarg;
      break;
    case 'v':
      verbose++;
      break;
    default:
      return Rts2App::processOption (in_opt);
    }
  return 0;
}

int
TelModelTest::processArgs (const char *in_arg)
{
  runFiles.push_back (in_arg);
  return 0;
}

int
TelModelTest::init ()
{
  int ret;
  ret = Rts2App::init ();
  if (ret)
    return ret;

  telescope = new Rts2DevTelescopeModelTest ();

  model = new Rts2TelModel (telescope, modelFile);
  ret = model->load ();
  return ret;
}

void
TelModelTest::test (double ra, double dec)
{
  struct ln_equ_posn pos;
  pos.ra = ra;
  pos.dec = dec;
  model->apply (&pos);
  std::cout << "RA: " << ra << " DEC: " << dec << " -> " << pos.
    ra << " " << pos.dec << std::endl;
}

void
TelModelTest::runOnFile (std::string filename, std::ostream & os)
{
  char caption[81];
  double temp;
  double press;
  std::ifstream is (filename.c_str ());
  char firstChar;
  bool latLine = false;

  is.getline (caption, 80);
  os << caption << std::endl;
  while (!is.eof ())
    {
      // ignore
      firstChar = is.get ();
      if (firstChar == ':')
	{
	  is.getline (caption, 80);
	  os << firstChar << caption << std::endl;
	}
      else if (firstChar == 'E')
	{
	  std::string nd;
	  is >> nd;
	  if (nd == "ND")
	    {
	      os << "END" << std::endl;
	      return;
	    }
	  std::cerr << "Invalid end? E" << nd << std::endl;
	}
      // first line contains lat
      else if (!latLine)
	{
	  struct ln_dms lat;
	  struct ln_date date;
	  date.hours = 0;
	  date.minutes = 0;
	  date.seconds = 0;
	  // get sign
	  is >> firstChar >> lat.degrees >> lat.minutes >> lat.
	    seconds >> date.years >> date.months >> date.
	    days >> temp >> press;
	  if (is.fail ())
	    return;
	  is.ignore (2000, is.widen ('\n'));
	  if (firstChar == '-')
	    lat.neg = 1;
	  else
	    lat.neg = 0;
	  telescope->setObserverLat (ln_dms_to_deg (&lat));
	  latLine = true;
	  os << " " << LibnovaDeg (&lat) << " "
	    << date.years << " "
	    << date.months << " "
	    << date.days << " " << temp << " " << press << " " << std::endl;
	}
      // data lines
      else
	{
	  double aux1;
	  double aux2;
	  int m, d;
	  double epoch;

	  LibnovaRaDec _in;
	  LibnovaRaDec _out;
	  LibnovaHaM lst;

	  is >> _in >> m >> d >> epoch >> _out >> lst >> aux1 >> aux2;
	  is.ignore (2000, is.widen ('\n'));
	  if (is.fail ())
	    {
	      std::cerr << "Failed during file read" << std::endl;
	      return;
	    }
	  // calculate model position, output it,..
	  struct ln_equ_posn pos;
	  _out.getPos (&pos);
	  pos.ra = ln_range_degrees (lst.getRa () - pos.ra);
	  if (verbose > 0)
	    model->applyVerbose (&pos);
	  else
	    model->apply (&pos);
	  // print it out
	  pos.ra = ln_range_degrees (lst.getRa () - pos.ra);
	  std::cout.precision (1);
	  LibnovaRaDec _out_in (&pos);
	  std::cout << "  " << _out_in << " "
	    << m << " "
	    << d << " " << epoch << "   " << _out << "   " << lst << " ";
	  std::cout.precision (6);
	  std::cout << aux1 << " " << aux2 << std::endl;
	}
    }
}

int
TelModelTest::run ()
{
  if (!runFiles.empty ())
    {
      for (std::vector < std::string >::iterator iter = runFiles.begin ();
	   iter != runFiles.end (); iter++)
	runOnFile ((*iter), std::cout);
    }
  else
    {
      // some generic tests
      test (10, 20);
      test (30, 50);
    }
  return 0;
}

int
main (int argc, char **argv)
{
  int ret;
  TelModelTest app (argc, argv);
  ret = app.init ();
  if (ret)
    return ret;
  return app.run ();
}
